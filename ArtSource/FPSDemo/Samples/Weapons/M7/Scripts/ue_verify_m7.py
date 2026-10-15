"""Read-only validation of persisted, categorized M7 Unreal assets.

Run in an Unreal editor/Python commandlet after ue_import_m7.py finishes.
No asset is created, reimported, modified, saved, or force-reloaded here.
A process started after UnrealImportReport.json was written proves a fresh
process read; loading assets again in the import process does not prove it.
"""
import datetime
import hashlib
import json
import os
import re
import struct
import traceback
from pathlib import Path

import unreal

ROOT = Path(__file__).resolve().parents[1]
CORE_ROOT = '/Game/FPSDemo/Art/Weapons/Rifles/M7'
ATTACHMENT_ROOT = '/Game/FPSDemo/Art/Weapons/Attachments'
SLOT_FOLDERS = {'Stock': 'Stocks', 'Magazine': 'Magazines', 'RearGrip': 'RearGrips',
                'Optic': 'Optics', 'Foregrip': 'Foregrips', 'Muzzle': 'Muzzles',
                'Sights': 'Sights', 'Barrel': 'Barrels'}
OWNER_KEY, OWNER = 'm7_import_owner', 'M7UnrealImport'
CHANNELS = ('BaseColor', 'Normal', 'ORM')
LIB = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary


def digest(path):
    value = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            value.update(block)
    return value.hexdigest()


def asset_path(value):
    """Compare UE package and object paths without treating suffix as a folder."""
    if value is None:
        return None
    text = value.get_path_name() if hasattr(value, 'get_path_name') else str(value)
    return text.split('.', 1)[0]


def folder_for(row):
    if row['category'] == 'Core':
        return CORE_ROOT
    return ATTACHMENT_ROOT + '/' + SLOT_FOLDERS[row['category']] + '/M7'


def mesh_path(row):
    folder = CORE_ROOT + '/Presets' if 'parts' in row else folder_for(row) + '/Meshes'
    return folder + '/' + row['name']


def load_owned(path, expected_type):
    assert LIB.does_asset_exist(path), 'Asset package missing: ' + path
    asset = unreal.load_asset(path)
    assert isinstance(asset, expected_type), 'Wrong or unreadable asset class: ' + path
    assert str(LIB.get_metadata_tag(asset, OWNER_KEY)) == OWNER, 'Wrong ownership: ' + path
    assert asset_path(asset) == path, 'Redirector/unexpected asset destination: ' + path
    return asset


def maximum_error(actual, expected):
    assert len(actual) == len(expected), ('Vector length mismatch', actual, expected)
    return max((abs(float(a) - float(b)) for a, b in zip(actual, expected)), default=0.0)


def vector(value):
    return [value.x, value.y, value.z]


def png_size(path):
    with Path(path).open('rb') as stream:
        data = stream.read(24)
    assert data[:8] == b'\x89PNG\r\n\x1a\n' and data[12:16] == b'IHDR', 'Invalid PNG header: ' + str(path)
    return list(struct.unpack('>II', data[16:24]))


def socket_positions(row, manifest):
    if row.get('category') == 'Core':
        return {key: [p[0] * 100, -p[1] * 100, p[2] * 100]
                for key, p in manifest['mounts'].items()
                if key not in ('Core', 'Foregrip', 'Muzzle', 'FrontIron')}
    if row.get('category') == 'Barrel':
        return {key: [p[0] * 100, -p[1] * 100, p[2] * 100]
                for key, p in row['local_mounts_m'].items()}
    return {}


def source_slot(name, expected):
    if name in expected:
        return name
    sanitized = [key for key in expected if key.replace('.', '_') == name]
    if len(sanitized) == 1:
        return sanitized[0]
    canonical = re.sub(r'(?:\.\d{3})+$', '', name)
    assert canonical in expected, 'Unknown FBX material slot: ' + name
    return canonical


def material_plan(manifest):
    ordinary, plans = {}, {}
    for row in manifest['assets']:
        folder = folder_for(row)
        stem = row['name'].removeprefix('SM_')
        maps = row['texture_sets']
        unique = {tuple(channels[c] for c in CHANNELS) for channels in maps.values()}
        assert len(unique) == 1, 'Expected one atlas per component: ' + row['name']
        triple = next(iter(unique))
        instance = folder + '/Materials/MI_' + stem
        textures = {c: folder + '/Textures/T_' + stem + '_' + c for c in CHANNELS}
        plans[row['name']] = {'instance': instance, 'textures': textures,
                            'source_files': dict(zip(CHANNELS, triple)), 'row': row}
        for name in maps:
            assert name not in ordinary or ordinary[name] == instance, 'Ambiguous material slot: ' + name
            ordinary[name] = instance
    constants = set(manifest.get('preserved_constant_materials', ()))
    for row in manifest['assets']:
        constants.update(row.get('preserved_constant_materials', ()))
    for name in constants:
        assert name not in ordinary, 'Constant material was assigned an atlas: ' + name
        ordinary[name] = CORE_ROOT + '/Materials/M_' + name
    return ordinary, plans, constants


def verify_mesh(row, imported, slots, manifest):
    expected_path = mesh_path(row)
    assert imported['path'] == expected_path, ('Wrong weapon/attachment classification', row['name'], imported['path'], expected_path)
    assert imported['category'] == row.get('category', 'Preset'), 'Report category differs: ' + row['name']
    assert imported['class'] == 'StaticMesh' and imported['saved'], 'Report mesh was not saved: ' + row['name']
    mesh = load_owned(expected_path, unreal.StaticMesh)
    bounds = mesh.get_bounding_box()
    actual = {'min': vector(bounds.min), 'max': vector(bounds.max)}
    source = row.get('assembly_bounds_m', row.get('local_bounds_m'))
    expected = {key: [p * 100 for p in source[key]] for key in ('min', 'max')}
    expected['min'][1], expected['max'][1] = -expected['max'][1], -expected['min'][1]
    error = max(maximum_error(actual[k], expected[k]) for k in ('min', 'max'))
    assert error < .01, ('Axis/scale/pivot bounds mismatch', expected_path, actual, expected)
    assert all(maximum_error(actual[k], imported['bounds_cm'][k]) < .01 for k in ('min', 'max')), 'Bounds differ from import report: ' + expected_path
    assert all(maximum_error(expected[k], imported['expected_bounds_cm'][k]) < 1e-5 for k in ('min', 'max')), 'Import report expected bounds differ from manifest: ' + expected_path
    triangles = mesh.get_num_triangles(0)
    geometry = json.loads((ROOT / 'UEGeometryPreflight.json').read_text(encoding='utf-8'))
    assert geometry['success'], 'FBX degeneracy evidence failed'
    evidence = next(item for item in geometry['assets'] if item['name'] == row['name'])
    assert evidence['source_sha256'] == digest(ROOT / row['exports']['fbx']), 'FBX degeneracy evidence source changed'
    assert evidence['source_triangles'] == row['triangles'] == imported['source_triangles']
    budget = evidence['strong_degenerate_candidate_count']
    assert imported['degenerate_candidate_budget'] == budget
    needs_review = not 0 <= row['triangles'] - triangles <= budget
    assert imported['needs_surface_review'] == needs_review
    if needs_review:
        review = json.loads((ROOT / 'UESurfaceReview.json').read_text(encoding='utf-8'))
        assert review['success'], 'Unreal surface comparison failed'
        assert review['import_report_sha256'] == digest(ROOT / 'UnrealImportReport.json'), 'Surface evidence refers to another import'
        match = next(item for item in review['assets'] if item['name'] == row['name'])
        assert match['success'], 'Surface-area review failed: ' + row['name']
    assert triangles == imported['triangles'], ('Triangle count differs from saved import report', expected_path, triangles)
    assert row['triangles'] - triangles == imported['triangulation_triangle_delta']
    uvs = mesh.get_num_tex_coords(0)
    assert uvs >= 1 and uvs == imported['uv_channels'], ('Missing/changed UV channels', expected_path, uvs)
    assert mesh.get_num_lods() == imported['lod_count'], 'LOD count differs from import report: ' + expected_path
    assert imported['source_fbx'] == row['exports']['fbx'], 'Import report source differs: ' + expected_path
    assert str(LIB.get_metadata_tag(mesh, 'm7_source_fbx')) == row['exports']['fbx'], 'Mesh source metadata differs: ' + expected_path
    assert str(LIB.get_metadata_tag(mesh, 'm7_source_sha256')) == digest(ROOT / row['exports']['fbx']), 'Mesh source SHA differs: ' + expected_path
    assert str(LIB.get_metadata_tag(mesh, 'm7_category')) == row.get('category', 'AssembledPreset'), 'Mesh category metadata differs: ' + expected_path
    assert str(LIB.get_metadata_tag(mesh, 'm7_model')) == 'M7', 'Mesh model metadata differs: ' + expected_path
    assert str(LIB.get_metadata_tag(mesh, 'm7_units')) == 'centimetres', 'Mesh unit metadata differs: ' + expected_path
    assert str(LIB.get_metadata_tag(mesh, 'm7_forward_axis')) == '+X', 'Mesh forward-axis metadata differs: ' + expected_path
    observed_slots = mesh.get_editor_property('static_materials')
    assert len(observed_slots) == len(imported['materials']), 'Material slot count changed: ' + expected_path
    assigned = []
    for index, (slot, proof) in enumerate(zip(observed_slots, imported['materials'])):
        name = str(slot.get_editor_property('material_slot_name'))
        key = source_slot(name, slots)
        expected_material = slots[key]
        actual_material = asset_path(mesh.get_material(index))
        assert name == proof['slot'] and key == proof['source_slot'], ('Report material-slot identity differs', expected_path, index, name)
        assert actual_material == expected_material == asset_path(proof['material']), ('Material-slot assignment mismatch', expected_path, index, actual_material, expected_material)
        assigned.append({'index': index, 'slot': name, 'source_slot': key, 'material': actual_material})
    assert {s['source_slot'] for s in assigned} == set(row['used_materials']), 'Source material coverage differs: ' + expected_path
    positions = socket_positions(row, manifest)
    assert set(imported['sockets_cm']) == set(positions), 'Report socket names differ from manifest: ' + expected_path
    sockets = {}
    for name, location in positions.items():
        socket = mesh.find_socket(name)
        assert socket is not None, 'Socket missing: ' + expected_path + ':' + name
        observed = vector(socket.get_editor_property('relative_location'))
        assert maximum_error(observed, location) < 1e-4, ('Socket placement differs', expected_path, name, observed, location)
        assert maximum_error(observed, imported['sockets_cm'][name]) < 1e-4, 'Socket differs from import report: ' + expected_path + ':' + name
        rotation = socket.get_editor_property('relative_rotation')
        assert maximum_error([rotation.pitch, rotation.yaw, rotation.roll], [0, 0, 0]) < 1e-4, 'Socket rotation differs: ' + expected_path + ':' + name
        assert maximum_error(vector(socket.get_editor_property('relative_scale')), [1, 1, 1]) < 1e-5, 'Socket scale differs: ' + expected_path + ':' + name
        sockets[name] = observed
    if 'parts' in row:
        assert imported['parts'] == row['parts'], 'Preset component list differs: ' + expected_path
        assert set(imported['part_mounts_cm']) == set(row['part_mounts_m']), 'Preset mounting component names differ: ' + expected_path
        for name, point in row['part_mounts_m'].items():
            assert maximum_error(imported['part_mounts_cm'][name], [v * 100 for v in point]) < 1e-4, 'Preset resolved mount differs: ' + expected_path + ':' + name
    return {'path': expected_path, 'bounds_cm': actual, 'maximum_bound_error_cm': error,
            'triangles': triangles, 'uv_channels': uvs, 'lod_count': mesh.get_num_lods(),
            'materials': assigned, 'sockets_cm': sockets, 'pass': True}


def verify_texture(path, channel, source, proof):
    assert proof['path'] == path and proof['channel'] == channel and proof['saved'], 'Texture report identity differs: ' + path
    assert Path(proof['source']).resolve() == source.resolve(), 'Texture report source differs: ' + path
    size = png_size(source)
    assert size == proof['source_resolution'], 'Texture source resolution changed: ' + path
    texture = load_owned(path, unreal.Texture2D)
    srgb = bool(texture.get_editor_property('srgb'))
    flip = bool(texture.get_editor_property('flip_green_channel'))
    compression = texture.get_editor_property('compression_settings')
    expected_compression = {'BaseColor': unreal.TextureCompressionSettings.TC_DEFAULT,
                            'Normal': unreal.TextureCompressionSettings.TC_NORMALMAP,
                            'ORM': unreal.TextureCompressionSettings.TC_MASKS}[channel]
    assert srgb == (channel == 'BaseColor') == proof['srgb'], 'Texture sRGB differs: ' + path
    assert flip == (channel == 'Normal') == proof['flip_green_channel'], 'Texture normal convention differs: ' + path
    assert compression == expected_compression and str(compression) == proof['compression'], 'Texture compression differs: ' + path
    return {'path': path, 'channel': channel, 'source_resolution': size, 'srgb': srgb,
            'compression': str(compression), 'flip_green_channel': flip, 'pass': True}


def verify_instance(plan, proof):
    path = plan['instance']
    assert proof['type'] == 'baked_instance' and proof['saved'], 'Material instance report invalid: ' + path
    assert proof['source_asset'] == plan['row']['name'], 'Material instance source differs: ' + path
    expected_parent = CORE_ROOT + '/Materials/M_M7_BakedPBR'
    assert asset_path(proof['parent']) == expected_parent, 'Reported parent differs: ' + path
    instance = load_owned(path, unreal.MaterialInstanceConstant)
    parent = asset_path(instance.get_editor_property('parent'))
    assert parent == expected_parent, 'Material instance parent differs: ' + path
    assert set(proof['parameters']) == set(CHANNELS), 'Material instance report parameter names differ: ' + path
    assert set(proof['source_slots']) == set(plan['row']['texture_sets']), 'Material instance source-slot coverage differs: ' + path
    textures = {}
    for channel in CHANNELS:
        actual = asset_path(MEL.get_material_instance_texture_parameter_value(instance, channel))
        expected = plan['textures'][channel]
        assert actual == expected == asset_path(proof['parameters'][channel]), ('Material instance texture differs', path, channel, actual, expected)
        textures[channel] = actual
    return {'path': path, 'parent': parent, 'parameters': textures, 'pass': True}


def process_started_timestamp():
    """Use OS process creation time, not the time this Python module was loaded."""
    if os.name == 'nt':
        import ctypes
        from ctypes import wintypes
        kernel = ctypes.WinDLL('kernel32', use_last_error=True)
        kernel.GetCurrentProcess.restype = wintypes.HANDLE
        kernel.GetProcessTimes.argtypes = [wintypes.HANDLE] + [ctypes.POINTER(wintypes.FILETIME)] * 4
        kernel.GetProcessTimes.restype = wintypes.BOOL
        created, exited, system, user = (wintypes.FILETIME() for _ in range(4))
        if not kernel.GetProcessTimes(kernel.GetCurrentProcess(), ctypes.byref(created), ctypes.byref(exited), ctypes.byref(system), ctypes.byref(user)):
            raise ctypes.WinError(ctypes.get_last_error())
        ticks = (created.dwHighDateTime << 32) | created.dwLowDateTime
        return ticks / 10000000.0 - 11644473600.0
    return None


def main(require_fresh_process=False):
    destination = ROOT / 'UnrealVerification.json'
    report = {'generator': 'Scripts/ue_verify_m7.py',
              'generated_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'engine': unreal.SystemLibrary.get_engine_version(), 'success': False,
              'read_only': True, 'process_id': os.getpid(), 'require_fresh_process': require_fresh_process,
              'assets': [], 'textures': [], 'material_instances': [], 'other_materials': [], 'errors': []}

    def check(label, action, target=None):
        try:
            result = action()
            if target is not None:
                report[target].append(result)
        except Exception:
            error = {'check': label, 'traceback': traceback.format_exc()}
            report['errors'].append(error)
            unreal.log_error('M7_UE_VERIFY_FAILED ' + label + '\n' + error['traceback'])

    try:
        import_path = ROOT / 'UnrealImportReport.json'
        manifest = json.loads((ROOT / 'AssetManifest.json').read_text(encoding='utf-8'))
        imported = json.loads(import_path.read_text(encoding='utf-8'))
        report['source_reports'] = {'UnrealImportReport.json': digest(import_path), 'AssetManifest.json': digest(ROOT / 'AssetManifest.json')}
        started = process_started_timestamp()
        import_written = import_path.stat().st_mtime
        fresh = started is not None and started > import_written
        report['process_evidence'] = {'process_started_unix': started, 'import_report_written_unix': import_written,
                                      'fresh_process_since_import_report_write': fresh,
                                      'meaning': 'Fresh only when the OS process started after the completed import report was written. Loading an asset again in the same process is not fresh-process evidence.'}
        if require_fresh_process:
            assert fresh, 'Fresh-process verification requested but current OS process predates import report completion'
        assert imported['success'] and not imported['errors'], 'Importer did not report a complete successful import'
        assert len(manifest['assets']) == 19 and len(manifest['presets']) == 3, 'Unexpected source component/preset count'
        assert imported['core_root'] == CORE_ROOT and imported['attachment_root'] == ATTACHMENT_ROOT and imported['slot_folders'] == SLOT_FOLDERS, 'Import report classification roots differ'
        rows = manifest['assets'] + manifest['presets']
        mesh_reports = {r['name']: r for r in imported['assets']}
        assert len(imported['assets']) == len(mesh_reports) == 22 and set(mesh_reports) == {r['name'] for r in rows}, 'Expected exactly 22 distinct mesh import records'
        slots, plans, constants = material_plan(manifest)
        texture_reports = {r['path']: r for r in imported['textures']}
        expected_texture_paths = {p for plan in plans.values() for p in plan['textures'].values()}
        assert len(imported['textures']) == len(texture_reports) == 57 and set(texture_reports) == expected_texture_paths, 'Expected exactly 57 correctly classified texture import records'
        material_reports = {r['path']: r for r in imported['materials']}
        expected_material_paths = {p['instance'] for p in plans.values()} | {CORE_ROOT + '/Materials/M_M7_BakedPBR'} | {slots[n] for n in constants}
        assert len(material_reports) == len(imported['materials']) and set(material_reports) == expected_material_paths, 'Unexpected material record paths or duplicates'
        report['import_inputs_checked'] = 0
        for relative, expected_sha in imported['source_inputs'].items():
            check('source_input:' + relative, lambda p=relative, h=expected_sha: assert_digest(p, h))
            report['import_inputs_checked'] += 1
        for row in rows:
            check('mesh:' + row['name'], lambda r=row: verify_mesh(r, mesh_reports[r['name']], slots, manifest), 'assets')
        for plan in plans.values():
            for channel in CHANNELS:
                path = plan['textures'][channel]
                check('texture:' + path, lambda p=path, c=channel, q=plan: verify_texture(p, c, ROOT / q['source_files'][c], texture_reports[p]), 'textures')
            path = plan['instance']
            check('instance:' + path, lambda q=plan, p=path: verify_instance(q, material_reports[p]), 'material_instances')
        for path in sorted(expected_material_paths - {p['instance'] for p in plans.values()}):
            check('material:' + path, lambda p=path: verify_other_material(p, material_reports[p]), 'other_materials')
        socket_meshes = sum(bool(a['sockets_cm']) for a in report['assets'])
        report['summary'] = {'meshes_checked': len(report['assets']), 'textures_checked': len(report['textures']),
                             'material_instances_checked': len(report['material_instances']),
                             'other_materials_checked': len(report['other_materials']), 'socket_meshes_checked': socket_meshes,
                             'fresh_process_verified': fresh, 'error_count': len(report['errors'])}
        assert socket_meshes == 3, 'Expected Core and both Barrel meshes to have validated sockets'
        assert imported['summary']['meshes'] == 22 and imported['summary']['textures'] == 57 and imported['summary']['socket_meshes'] == 3, 'Import summary differs from expected asset totals'
        report['success'] = not report['errors'] and len(report['assets']) == 22 and len(report['textures']) == 57 and len(report['material_instances']) == 19
    except Exception:
        report['errors'].append({'check': 'verification_setup_or_totals', 'traceback': traceback.format_exc()})
    finally:
        report.setdefault('summary', {})['error_count'] = len(report['errors'])
        destination.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    if not report['success']:
        raise RuntimeError('M7 Unreal verification failed; see ' + str(destination) + '; errors=' + str(len(report['errors'])))
    unreal.log('M7_UE_VERIFICATION_COMPLETE ' + json.dumps(report['summary']))
    return report


def assert_digest(relative, expected):
    root = ROOT.resolve()
    path = (root / relative).resolve()
    assert path.is_relative_to(root), 'Input path escapes M7 source root: ' + relative
    assert path.is_file() and digest(path) == expected, 'Source input changed after import: ' + relative


def verify_other_material(path, proof):
    assert proof['saved'], 'Material not saved in import report: ' + path
    material = load_owned(path, unreal.Material)
    if proof['type'] == 'baked_master':
        assert proof['parameters'] == list(CHANNELS) and proof['orm_channels'] == {'R': 'AmbientOcclusion', 'G': 'Roughness', 'B': 'Metallic'}, 'Baked master channel report differs'
        assert material.get_editor_property('blend_mode') == unreal.BlendMode.BLEND_OPAQUE
        assert material.get_editor_property('shading_model') == unreal.MaterialShadingModel.MSM_DEFAULT_LIT
    else:
        assert proof['type'] == 'preserved_constant', 'Unknown material report type: ' + path
        glass = proof['transmission'] > 0
        assert material.get_editor_property('blend_mode') == (unreal.BlendMode.BLEND_TRANSLUCENT if glass else unreal.BlendMode.BLEND_OPAQUE), 'Constant material blend mode differs: ' + path
        assert material.get_editor_property('shading_model') == (unreal.MaterialShadingModel.MSM_THIN_TRANSLUCENT if glass else unreal.MaterialShadingModel.MSM_DEFAULT_LIT), 'Constant material shading differs: ' + path
        if glass:
            assert material.get_editor_property('translucency_lighting_mode') == unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING, 'Lens lighting mode differs: ' + path
    assert bool(material.get_editor_property('two_sided')), 'Expected double-sided M7 material: ' + path
    return {'path': path, 'type': proof['type'], 'pass': True}


if __name__ == '__main__':
    main(require_fresh_process='-M7RequireFreshVerification' in unreal.SystemLibrary.get_command_line())

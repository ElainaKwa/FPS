"""Independently reopen the M7 source and reimport its exports in background Blender.

Usage: blender --background --python ArtSource/M7Toon/Source/verify_m7.py
Writes Validation.json; never saves the source .blend or either export.
Material names and detachable GLB mesh count come from AssetManifest.json.
"""
import hashlib
import json
import math
import re
import traceback
from pathlib import Path

import bpy


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'M7_BLACK_Stylized.blend'
MANIFEST = ROOT / 'AssetManifest.json'
EXPORTS = {
    'fbx': ROOT / 'Exports/SM_TW_M7_BLACK.fbx',
    'glb': ROOT / 'Exports/SM_TW_M7_BLACK.glb',
}
COLLECTION = 'ASSET_M7_BLACK'
GEOMETRY_TYPES = {'MESH', 'FONT', 'CURVE', 'SURFACE', 'META'}
AXES = ('X', 'Y', 'Z')
TOLERANCE = 0.01


def canonical_material_name(name):
    # Blender resolves imported data-block name collisions with .001, .002, etc.
    return re.sub(r'\.\d{3,}$', '', name)


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def finite(values, label):
    require(all(math.isfinite(float(value)) for value in values),
            'Non-finite data: ' + label)


def file_fingerprint(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(block)
    return {'bytes': path.stat().st_size, 'sha256': digest.hexdigest()}


def check_mesh_data(mesh, label, require_uv=True):
    require(len(mesh.vertices) > 0 and len(mesh.polygons) > 0,
            label + ': empty mesh')
    require(all(material is not None for material in mesh.materials),
            label + ': empty material slot')
    if require_uv:
        require(len(mesh.uv_layers) > 0, label + ': missing UV layer')
    for vertex in mesh.vertices:
        finite(vertex.co, label + ' vertex coordinates')
        finite(vertex.normal, label + ' vertex normal')
    for layer in mesh.uv_layers:
        for loop in layer.data:
            finite(loop.uv, label + ' UV')
    for polygon in mesh.polygons:
        require(0 <= polygon.material_index < len(mesh.materials),
                label + ': invalid material index')
        finite(polygon.normal, label + ' polygon normal')
    # Modern Blender exposes split/corner normals independently of vertex normals.
    if hasattr(mesh, 'corner_normals'):
        for normal in mesh.corner_normals:
            finite(normal.vector, label + ' corner normal')
    mesh.calc_loop_triangles()
    for triangle in mesh.loop_triangles:
        finite(triangle.normal, label + ' triangle normal')


def summarize(objects, label, source=False):
    """Measure evaluated world geometry, including bevels and editable lettering."""
    objects = list(objects)
    require(objects, label + ': no objects')
    require(not any(obj.type in {'LIGHT', 'CAMERA'} for obj in objects),
            label + ': lights or cameras are inside the asset/export')
    bpy.context.view_layer.update()
    depsgraph = bpy.context.evaluated_depsgraph_get()
    mesh_objects = [obj for obj in objects if obj.type == 'MESH']
    geometry_objects = [obj for obj in objects if obj.type in GEOMETRY_TYPES]
    require(mesh_objects, label + ': no mesh objects')
    lo = [math.inf] * 3
    hi = [-math.inf] * 3
    triangles = 0
    vertices = 0
    raw_triangles = 0
    material_names = set()
    uv_layer_counts = []
    records = []
    for obj in objects:
        finite((value for row in obj.matrix_world for value in row),
               obj.name + ' matrix_world')
    for obj in geometry_objects:
        raw = obj.data
        materials = [material.name for material in raw.materials if material is not None]
        require(materials, label + ': missing material on ' + obj.name)
        material_names.update(materials)
        if obj.type == 'MESH':
            check_mesh_data(raw, label + ': ' + obj.name + ' source mesh')
            uv_layer_counts.append(len(raw.uv_layers))
            raw_triangles += len(raw.loop_triangles)
        evaluated = obj.evaluated_get(depsgraph)
        mesh = evaluated.to_mesh(preserve_all_data_layers=True, depsgraph=depsgraph)
        try:
            require(mesh is not None, label + ': no evaluated mesh for ' + obj.name)
            # Editable FONT geometry can lack UVs until the export's baked unwrap.
            check_mesh_data(mesh, label + ': ' + obj.name + ' evaluated mesh',
                            require_uv=not (source and obj.type != 'MESH'))
            count = len(mesh.loop_triangles)
            require(count > 0, label + ': evaluated mesh has no faces for ' + obj.name)
            triangles += count
            vertices += len(mesh.vertices)
            for vertex in mesh.vertices:
                position = evaluated.matrix_world @ vertex.co
                finite(position, obj.name + ' evaluated world vertex')
                for axis in range(3):
                    lo[axis] = min(lo[axis], float(position[axis]))
                    hi[axis] = max(hi[axis], float(position[axis]))
            records.append({
                'name': obj.name,
                'type': obj.type,
                'evaluated_triangles': count,
                'uv_layers': len(mesh.uv_layers),
                'materials': materials,
            })
        finally:
            evaluated.to_mesh_clear()
    finite(lo + hi, label + ' world bounds')
    dimensions = [high - low for low, high in zip(lo, hi)]
    require(all(size > 0 for size in dimensions), label + ': degenerate asset dimensions')
    require(triangles > 0, label + ': no evaluated triangles')
    if not source:
        require(len(mesh_objects) == len(geometry_objects),
                label + ': import contains unexpected non-mesh geometry')
    return {
        'objects': len(objects),
        'mesh_objects': len(mesh_objects),
        'geometry_objects': len(geometry_objects),
        'empty_objects': sum(obj.type == 'EMPTY' for obj in objects),
        'evaluated_vertices': vertices,
        'raw_triangles': raw_triangles,
        'evaluated_triangles': triangles,
        'mesh_uv_layers_min': min(uv_layer_counts),
        'mesh_uv_layers_max': max(uv_layer_counts),
        'finite_vertices_uvs_and_normals': True,
        'materials': sorted(material_names),
        'canonical_materials': sorted({canonical_material_name(name) for name in material_names}),
        'world_bbox_m': {'min': lo, 'max': hi, 'dimensions': dimensions},
        'axis_dimensions_m': dict(zip(AXES, dimensions)),
        'axes_longest_to_shortest': sorted(
            AXES, key=lambda axis: dimensions[AXES.index(axis)], reverse=True),
        'geometry': records,
    }


def compare_dimensions(actual, expected, label):
    observed = actual['world_bbox_m']['dimensions']
    reference = expected['world_bbox_m']['dimensions']
    errors = [abs(value - target) / abs(target) for value, target in zip(observed, reference)]
    result = {
        'comparison': label,
        'axis_comparison': 'strict XYZ; axis permutations are not accepted',
        'observed_axis_dimensions_m': actual['axis_dimensions_m'],
        'reference_axis_dimensions_m': expected['axis_dimensions_m'],
        'relative_dimension_errors': errors,
        'maximum_relative_error': max(errors),
        'tolerance': TOLERANCE,
        'passed': all(error < TOLERANCE for error in errors),
    }
    require(result['passed'], label + ': world dimensions differ by 1% or more: ' + str(result))
    return result


def compare_world_bounds(actual, expected, label):
    """Compare XYZ min/max placement, normalized by each source axis size."""
    observed = actual['world_bbox_m']
    reference = expected['world_bbox_m']
    errors = {
        edge: [abs(value - target) / size
               for value, target, size in zip(observed[edge], reference[edge], reference['dimensions'])]
        for edge in ('min', 'max')
    }
    maximum = max(error for values in errors.values() for error in values)
    result = {
        'comparison': label,
        'observed_world_bbox_m': observed,
        'reference_world_bbox_m': reference,
        'bounds_errors_relative_to_axis_size': errors,
        'maximum_relative_error': maximum,
        'tolerance': TOLERANCE,
        'passed': maximum < TOLERANCE,
    }
    require(result['passed'], label + ': world bounds differ by 1% or more: ' + str(result))
    return result


def compare_content(actual, expected, expected_materials, label):
    actual_materials = set(actual['canonical_materials'])
    reference_materials = set(expected['canonical_materials'])
    result = {
        'comparison': label,
        'observed_evaluated_triangles': actual['evaluated_triangles'],
        'reference_evaluated_triangles': expected['evaluated_triangles'],
        'triangles_equal': actual['evaluated_triangles'] == expected['evaluated_triangles'],
        'observed_materials': sorted(actual_materials),
        'reference_materials': sorted(reference_materials),
        'manifest_materials': sorted(expected_materials),
        'material_normalization': 'strip Blender numeric data-block suffix such as .001',
        'materials_equal': actual_materials == reference_materials == expected_materials,
    }
    result['passed'] = result['triangles_equal'] and result['materials_equal']
    require(result['passed'], label + ': triangle/material mismatch: ' + str(result))
    return result


def empty_import_scene():
    # This independent background process is reset between imports and never saves.
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.name = 'M7_Verification_Temporary'
    scene.unit_settings.scale_length = 1.0
    require(len(scene.objects) == 0, 'Temporary import scene is not empty')
    return scene


def main(report):
    require(bpy.app.background, 'Run this verifier in background Blender, not a live editing session')
    for path in (MANIFEST, SOURCE, *EXPORTS.values()):
        require(path.is_file() and path.stat().st_size > 0, 'Missing or empty asset file: ' + str(path))
        report['files'].append({'file': str(path.relative_to(ROOT)), **file_fingerprint(path)})
    manifest = json.loads(MANIFEST.read_text(encoding='utf-8'))
    material_names = manifest.get('materials')
    require(isinstance(material_names, list) and material_names
            and all(isinstance(name, str) and name for name in material_names),
            'Manifest materials must be a nonempty list of material names')
    expected_materials = {canonical_material_name(name) for name in material_names}
    require(len(expected_materials) == len(material_names), 'Manifest material names are duplicated')
    expected_parts = manifest.get('gltf_detachable_parts')
    require(type(expected_parts) is int and expected_parts > 0,
            'Manifest gltf_detachable_parts must be a positive integer')
    report['manifest_contract'] = {
        'materials': sorted(expected_materials),
        'gltf_detachable_parts': expected_parts,
    }

    bpy.ops.wm.open_mainfile(filepath=str(SOURCE))
    require(len(bpy.data.scenes) == 1,
            'Source must contain exactly one scene: ' + str([scene.name for scene in bpy.data.scenes]))
    old_names = [block.name for blocks in (bpy.data.scenes, bpy.data.objects, bpy.data.collections)
                 for block in blocks if 'TIDELINE' in block.name.upper()]
    require(not old_names, 'Source contains previous TIDELINE scene/object/collection data: ' + str(old_names))
    collection = bpy.data.collections.get(COLLECTION)
    require(collection is not None, 'Missing editable asset collection: ' + COLLECTION)
    require(collection.asset_data is not None, 'Collection is not marked as a Blender asset')
    require(bool(collection.asset_data.description.strip()), 'Asset metadata description is empty')
    objects = list(collection.all_objects)
    require(len(objects) > 100, 'Expected more than 100 editable asset objects')
    source_scene = bpy.data.scenes[0]
    require(all(source_scene.objects.get(obj.name) == obj for obj in objects),
            'Source asset objects are not all linked to the sole source scene')
    if bpy.context.scene != source_scene:
        require(bpy.context.window is not None, 'Cannot select the asset source scene')
        bpy.context.window.scene = source_scene
    report['source'] = summarize(objects, 'source', source=True)
    report['source'].update({
        'asset_collection': collection.name,
        'asset_marked': True,
        'asset_description': collection.asset_data.description,
        'asset_author': collection.asset_data.author,
        'asset_tags': [tag.name for tag in collection.asset_data.tags],
        'scene_count': len(bpy.data.scenes),
        'scene_name': source_scene.name,
        'previous_tideline_data_absent': True,
    })
    require(set(report['source']['canonical_materials']) == expected_materials,
            'Source materials differ from AssetManifest.json: ' + str(report['source']['canonical_materials']))
    report['checks'].append(
        'Source reopened independently; sole scene contains M7 asset with metadata and more than 100 editable objects; '
        'no prior TIDELINE scene/object/collection; finite coordinates, normals, materials and UVs checked')

    fbx_scene = empty_import_scene()
    bpy.ops.import_scene.fbx(filepath=str(EXPORTS['fbx']))
    report['fbx'] = summarize(fbx_scene.objects, 'FBX default import')
    report['fbx']['import_orientation'] = {
        'use_manual_orientation': False,
        'mode': 'Blender default import using FBX axis metadata',
        'source_axis_dimensions_m': report['source']['axis_dimensions_m'],
        'observed_axis_dimensions_m': report['fbx']['axis_dimensions_m'],
    }
    report['comparisons'].append(compare_dimensions(
        report['fbx'], report['source'], 'Default FBX XYZ dimensions versus source'))
    report['comparisons'].append(compare_world_bounds(
        report['fbx'], report['source'], 'Default FBX XYZ world bounds versus source'))
    report['comparisons'].append(compare_content(
        report['fbx'], report['source'], expected_materials, 'Default FBX triangles and manifest materials versus source'))
    report['checks'].append(
        'FBX imported independently with default orientation; strict XYZ dimensions and min/max bounds match source '
        'within 1%; evaluated triangle count and manifest material names match; UVs and normals are finite')

    glb_scene = empty_import_scene()
    bpy.ops.import_scene.gltf(filepath=str(EXPORTS['glb']))
    report['glb'] = summarize(glb_scene.objects, 'GLB')
    report['glb']['expected_mesh_objects_from_manifest'] = expected_parts
    require(report['glb']['mesh_objects'] == expected_parts,
            'GLB detachable mesh count differs from manifest: observed '
            + str(report['glb']['mesh_objects']) + ', expected ' + str(expected_parts))
    for other, name in ((report['source'], 'source'), (report['fbx'], 'FBX')):
        report['comparisons'].append(compare_dimensions(
            report['glb'], other, 'GLB XYZ dimensions versus ' + name))
        report['comparisons'].append(compare_world_bounds(
            report['glb'], other, 'GLB XYZ world bounds versus ' + name))
        report['comparisons'].append(compare_content(
            report['glb'], other, expected_materials, 'GLB triangles and manifest materials versus ' + name))
    report['checks'].append(
        'GLB imported independently; detachable mesh count matches manifest; strict XYZ dimensions and min/max bounds '
        'match source and FBX within 1%; evaluated triangle count and manifest materials match; UVs and normals are finite')

    for record in report['files']:
        require(file_fingerprint(ROOT / record['file']) == {key: record[key] for key in ('bytes', 'sha256')},
                'Asset file changed during verification: ' + record['file'])
    report['checks'].append('SHA-256 confirms the source, exports and manifest were not modified during verification')
    report['success'] = True


if __name__ == '__main__':
    report = {
        'asset': 'M7_BLACK',
        'blender': bpy.app.version_string,
        'success': False,
        'files': [],
        'checks': [],
        'comparisons': [],
        'errors': [],
        'source_units': 'metres; +X muzzle; +Z up',
        'fbx_export_contract': {
            'export_axis_forward': 'X',
            'axis_up': 'Z',
            'use_space_transform': True,
            'settings_source': 'Source/export_m7.py task contract; verified by default import geometry comparisons',
        },
        'source_or_exports_saved_by_verifier': False,
    }
    try:
        main(report)
    except Exception as exc:
        report['errors'].append(str(exc))
        report['traceback'] = traceback.format_exc()
        raise
    finally:
        (ROOT / 'Validation.json').write_text(
            json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
        print('M7_ASSET_VALIDATION ' + ('PASS' if report['success'] else 'FAIL'), flush=True)

"""Reopen and verify only the TIDELINE AR-01 source and exports in background Blender.

Usage: blender --background --python ArtSource/ToonRifle/Source/verify_asset.py
Writes Validation.json; never saves or changes the source .blend or either export.
"""
import json
import math
import re
import traceback
from pathlib import Path

import bpy


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'TIDELINE_AR01.blend'
EXPORTS = {
    'fbx': ROOT / 'Exports/SM_TW_TIDELINE_AR01.fbx',
    'glb': ROOT / 'Exports/SM_TW_TIDELINE_AR01.glb',
}
COLLECTION = 'ASSET_TIDELINE_AR01'
GEOMETRY_TYPES = {'MESH', 'FONT', 'CURVE', 'SURFACE', 'META'}
AXES = ('X', 'Y', 'Z')
TOLERANCE = 0.01
EXPECTED_MATERIALS = {
    'AR01_01_MidnightCeramic',
    'AR01_02_LagoonPolymer',
    'AR01_03_WarmPorcelain',
    'AR01_04_SignalAmber',
    'AR01_05_GripRubber',
    'AR01_06_SatinSteel',
    'AR01_07_RecessBlack',
    'AR01_08_OpticalGlass',
    'AR01_09_SightIndicator',
}


def canonical_material_name(name):
    # Blender resolves imported data-block name collisions with .001, .002, etc.
    return re.sub(r'\.\d{3,}$', '', name)


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def finite(values, label):
    require(all(math.isfinite(float(v)) for v in values),
            'Non-finite coordinates: ' + label)


def summarize(objects, label, require_source_mesh_uv=False):
    """Measure evaluated visible geometry, including bevels and font outlines."""
    objects = list(objects)
    require(objects, label + ': no objects')
    require(not any(o.type in {'LIGHT', 'CAMERA'} for o in objects),
            label + ': lights or cameras are inside the asset/export')
    bpy.context.view_layer.update()
    depsgraph = bpy.context.evaluated_depsgraph_get()
    mesh_objects = [o for o in objects if o.type == 'MESH']
    geometry_objects = [o for o in objects if o.type in GEOMETRY_TYPES]
    require(mesh_objects, label + ': no mesh objects')
    lo = [math.inf] * 3
    hi = [-math.inf] * 3
    triangles = 0
    vertices = 0
    material_names = set()
    uv_layer_counts = []
    raw_triangles = 0
    records = []
    for obj in objects:
        finite((v for row in obj.matrix_world for v in row), obj.name + ' matrix_world')
    for obj in geometry_objects:
        raw = obj.data
        materials = [m.name for m in raw.materials if m is not None]
        require(materials, label + ': missing material on ' + obj.name)
        material_names.update(materials)
        if obj.type == 'MESH':
            require(len(raw.vertices) > 0 and len(raw.polygons) > 0,
                    label + ': empty mesh ' + obj.name)
            require(all(slot.material is not None for slot in obj.material_slots),
                    label + ': empty material slot on ' + obj.name)
            require(len(raw.uv_layers) > 0,
                    label + ': missing UV layer on ' + obj.name)
            uv_layer_counts.append(len(raw.uv_layers))
            for vertex in raw.vertices:
                finite(vertex.co, obj.name + ' source vertex')
            for layer in raw.uv_layers:
                for loop in layer.data:
                    finite(loop.uv, obj.name + ' UV')
            for polygon in raw.polygons:
                require(polygon.material_index < len(raw.materials),
                        label + ': invalid material index on ' + obj.name)
            raw.calc_loop_triangles()
            raw_triangles += len(raw.loop_triangles)
        evaluated = obj.evaluated_get(depsgraph)
        mesh = evaluated.to_mesh(preserve_all_data_layers=True, depsgraph=depsgraph)
        try:
            require(mesh is not None and len(mesh.vertices) > 0,
                    label + ': evaluated mesh is empty for ' + obj.name)
            mesh.calc_loop_triangles()
            count = len(mesh.loop_triangles)
            require(count > 0, label + ': evaluated mesh has no faces for ' + obj.name)
            triangles += count
            vertices += len(mesh.vertices)
            for vertex in mesh.vertices:
                position = evaluated.matrix_world @ vertex.co
                finite(position, obj.name + ' evaluated world vertex')
                for i in range(3):
                    lo[i] = min(lo[i], float(position[i]))
                    hi[i] = max(hi[i], float(position[i]))
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
    dimensions = [b - a for a, b in zip(lo, hi)]
    require(all(d > 0 for d in dimensions), label + ': degenerate asset dimensions')
    require(triangles > 0, label + ': no evaluated triangles')
    # Only imported meshes must cover the whole exported geometry. Source FONT
    # objects are editable lettering; their converted export is checked separately.
    if not require_source_mesh_uv:
        require(len(mesh_objects) == len(geometry_objects),
                label + ': import contains unexpected non-mesh geometry')
    return {
        'objects': len(objects),
        'mesh_objects': len(mesh_objects),
        'geometry_objects': len(geometry_objects),
        'empty_objects': sum(o.type == 'EMPTY' for o in objects),
        'evaluated_vertices': vertices,
        'raw_triangles': raw_triangles,
        'evaluated_triangles': triangles,
        'mesh_uv_layers_min': min(uv_layer_counts),
        'mesh_uv_layers_max': max(uv_layer_counts),
        'materials': sorted(material_names),
        'canonical_materials': sorted({canonical_material_name(name) for name in material_names}),
        'world_bbox_m': {'min': lo, 'max': hi, 'dimensions': dimensions},
        'axis_dimensions_m': dict(zip(AXES, dimensions)),
        'axes_longest_to_shortest': sorted(AXES, key=lambda axis: dimensions[AXES.index(axis)], reverse=True),
        'geometry': records,
    }


def compare_dimensions(actual, expected, label):
    observed = actual['world_bbox_m']['dimensions']
    reference = expected['world_bbox_m']['dimensions']
    errors = [abs(x - y) / abs(y) for x, y in zip(observed, reference)]
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
    a = actual['world_bbox_m']
    b = expected['world_bbox_m']
    errors = {
        edge: [abs(x - y) / size for x, y, size in zip(a[edge], b[edge], b['dimensions'])]
        for edge in ('min', 'max')
    }
    maximum = max(error for edge in errors.values() for error in edge)
    result = {
        'comparison': label,
        'observed_world_bbox_m': a,
        'reference_world_bbox_m': b,
        'bounds_errors_relative_to_axis_size': errors,
        'maximum_relative_error': maximum,
        'tolerance': TOLERANCE,
        'passed': maximum < TOLERANCE,
    }
    require(result['passed'], label + ': world bounds differ by 1% or more: ' + str(result))
    return result


def compare_content(actual, expected, label):
    actual_materials = set(actual['canonical_materials'])
    expected_materials = set(expected['canonical_materials'])
    result = {
        'comparison': label,
        'observed_evaluated_triangles': actual['evaluated_triangles'],
        'reference_evaluated_triangles': expected['evaluated_triangles'],
        'triangles_equal': actual['evaluated_triangles'] == expected['evaluated_triangles'],
        'observed_materials': sorted(actual_materials),
        'reference_materials': sorted(expected_materials),
        'material_normalization': 'strip Blender numeric data-block suffix such as .001',
        'materials_equal': actual_materials == expected_materials == EXPECTED_MATERIALS,
    }
    result['passed'] = result['triangles_equal'] and result['materials_equal']
    require(result['passed'], label + ': triangle/material mismatch: ' + str(result))
    return result


def empty_import_scene():
    # This process is a background verification session. Resetting it isolates
    # each importer and never writes to the reopened authoring source.
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.name = 'TIDELINE_Verification_Temporary'
    scene.unit_settings.scale_length = 1.0
    require(len(scene.objects) == 0, 'Temporary import scene is not empty')
    return scene


def main(report):
    require(bpy.app.background, 'Run this verifier in background Blender, not a live editing session')
    for path in (SOURCE, *EXPORTS.values()):
        require(path.is_file() and path.stat().st_size > 0, 'Missing or empty asset file: ' + str(path))
        report['files'].append({'file': str(path.relative_to(ROOT)), 'bytes': path.stat().st_size})
    bpy.ops.wm.open_mainfile(filepath=str(SOURCE))
    collection = bpy.data.collections.get(COLLECTION)
    require(collection is not None, 'Missing editable asset collection: ' + COLLECTION)
    require(collection.asset_data is not None, 'Collection is not marked as a Blender asset')
    objects = list(collection.all_objects)
    require(len(objects) > 100, 'Expected more than 100 editable asset objects')
    scenes = [s for s in bpy.data.scenes if objects[0].name in s.objects]
    require(scenes, 'Asset collection is not linked to a scene')
    source_scene = bpy.context.scene if bpy.context.scene in scenes else scenes[0]
    if bpy.context.scene != source_scene:
        require(bpy.context.window is not None, 'Cannot select the asset source scene')
        bpy.context.window.scene = source_scene
    report['source'] = summarize(objects, 'source', require_source_mesh_uv=True)
    report['source']['asset_collection'] = collection.name
    report['source']['asset_marked'] = True
    report['source']['asset_description'] = collection.asset_data.description
    require(set(report['source']['canonical_materials']) == EXPECTED_MATERIALS,
            'Source must use exactly the nine TIDELINE materials: ' + str(report['source']['canonical_materials']))
    report['checks'].append('Source reopened independently; asset metadata, editable objects, finite coordinates, materials and UVs checked')
    fbx_scene = empty_import_scene()
    bpy.ops.import_scene.fbx(filepath=str(EXPORTS['fbx']))
    report['fbx'] = summarize(fbx_scene.objects, 'FBX default import')
    report['fbx']['import_orientation'] = {
        'use_manual_orientation': False,
        'mode': 'Blender default import using FBX axis metadata',
        'source_axis_dimensions_m': report['source']['axis_dimensions_m'],
        'observed_axis_dimensions_m': report['fbx']['axis_dimensions_m'],
    }
    report['comparisons'].append(compare_dimensions(report['fbx'], report['source'], 'Default FBX XYZ dimensions versus source'))
    report['comparisons'].append(compare_world_bounds(report['fbx'], report['source'], 'Default FBX XYZ world bounds versus source'))
    report['comparisons'].append(compare_content(report['fbx'], report['source'], 'Default FBX triangles and nine materials versus source'))
    report['checks'].append('FBX imported independently with default orientation; strict XYZ dimensions and min/max bounds match source within 1%; evaluated triangle count and all nine material names match')
    glb_scene = empty_import_scene()
    bpy.ops.import_scene.gltf(filepath=str(EXPORTS['glb']))
    report['glb'] = summarize(glb_scene.objects, 'GLB')
    report['comparisons'].append(compare_dimensions(report['glb'], report['source'], 'GLB XYZ dimensions versus source'))
    report['comparisons'].append(compare_world_bounds(report['glb'], report['source'], 'GLB XYZ world bounds versus source'))
    report['comparisons'].append(compare_content(report['glb'], report['source'], 'GLB triangles and nine materials versus source'))
    report['comparisons'].append(compare_dimensions(report['glb'], report['fbx'], 'GLB XYZ dimensions versus FBX'))
    report['comparisons'].append(compare_world_bounds(report['glb'], report['fbx'], 'GLB XYZ world bounds versus FBX'))
    report['comparisons'].append(compare_content(report['glb'], report['fbx'], 'GLB triangles and nine materials versus FBX'))
    report['checks'].append('GLB imported independently; strict XYZ dimensions and min/max bounds match source and FBX within 1%; evaluated triangle count and all nine material names match')
    report['success'] = True


if __name__ == '__main__':
    report = {
        'asset': 'TIDELINE_AR01',
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
            'settings_source': 'Source/export_rifle.py task contract; verified by default import geometry comparisons',
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
        ROOT.mkdir(parents=True, exist_ok=True)
        (ROOT / 'Validation.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
        print('TIDELINE_ASSET_VALIDATION ' + ('PASS' if report['success'] else 'FAIL'), flush=True)

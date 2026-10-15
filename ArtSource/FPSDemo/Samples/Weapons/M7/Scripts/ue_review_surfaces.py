"""Compare aggregate FBX surface area for UE rows flagged for geometry review.

Run through Blender --background --factory-startup --python <this script>.
Blender only tessellates temporary meshes from source FBX coordinate arrays;
this script does not load/save the source library or run/export Unreal assets.
Target FBXs must be UE's real rendered LOD0 exports (source mesh, collision,
other LODs and material baking disabled). Aggregate area and bounds checks do
not prove per-face topology, UV interpolation, winding, normals or appearance.
"""
import datetime
import hashlib
import json
import sys
import traceback
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT.parents[4]
sys.path.insert(0, str(ROOT / 'Scripts'))
from ue_check_export_geometry import BinaryFBX, triangles, area_and_edges

RELATIVE_AREA_TOLERANCE = 1e-4
BOUND_TOLERANCE_CM = .01


def digest(path):
    value = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            value.update(block)
    return value.hexdigest()


def source_path(relative):
    path = (ROOT / relative).resolve()
    assert path.is_relative_to(ROOT.resolve()) and path.is_file(), 'Missing/out-of-root source FBX: ' + str(relative)
    return path


def export_path(value):
    path = Path(value)
    if not path.is_absolute():
        # Export reports use project-relative Saved/... or an absolute path.
        path = PROJECT / path
    path = path.resolve()
    expected_root = (PROJECT / 'Saved/M7UnrealImport/GeometryReview').resolve()
    assert path.is_relative_to(expected_root) and path.is_file(), 'Missing/out-of-review-root UE geometry export: ' + str(value)
    assert path.suffix.lower() == '.fbx', 'Geometry review target is not FBX: ' + str(value)
    return path


def measure(path, coordinate_unit):
    parsed = BinaryFBX(path)
    pieces = []
    points = []
    total_area_raw = 0.0
    total_triangles = 0
    for name, vertices, indices in parsed.geometry():
        ids, polygon_ids, tessellation = triangles(indices, vertices)
        assert np.all(np.isfinite(vertices)), 'Nonfinite FBX geometry coordinates: ' + str(path)
        positions = vertices[ids]
        areas, edges = area_and_edges(positions)
        assert np.all(np.isfinite(areas)), 'Nonfinite FBX triangle area: ' + str(path)
        # Sum in float64 even when the original render vertex buffer is float32.
        area = float(np.sum(areas, dtype=np.float64))
        total_area_raw += area
        total_triangles += len(ids)
        points.append(vertices[np.unique(ids)])
        pieces.append({'name': name, 'triangles': len(ids), 'area_raw_units_squared': area,
                       'zero_area_triangles_exact': int(np.count_nonzero(areas == 0)),
                       'tessellation': tessellation})
    assert pieces and points, 'No renderable FBX geometry arrays: ' + str(path)
    all_points = np.concatenate(points)
    minimum = np.min(all_points, axis=0)
    maximum = np.max(all_points, axis=0)
    scale_to_m = 1.0 if coordinate_unit == 'metres' else .01
    return {'coordinate_unit': coordinate_unit, 'triangles': total_triangles,
            'surface_area_m2': total_area_raw * scale_to_m * scale_to_m,
            'raw_surface_area': total_area_raw,
            'bounds_raw_units': {'min': minimum.tolist(), 'max': maximum.tolist()},
            'dimensions_raw_units': (maximum - minimum).tolist(), 'geometry': pieces}


def maximum_sorted_dimension_error(observed, expected):
    """FBX axis conversions may permute/sign-flip coordinates, not dimensions."""
    return max(abs(a - b) for a, b in zip(sorted(observed), sorted(expected)))


def review(entry, source_row, imported):
    source = source_path(source_row['exports']['fbx'])
    assert entry['source_fbx'] == source_row['exports']['fbx'], 'Import row source differs: ' + entry['name']
    target = export_path(entry['geometry_review_export'])
    source_sha = digest(source)
    assert imported['source_inputs'][source_row['exports']['fbx']] == source_sha, 'Source FBX changed after import: ' + entry['name']
    target_sha = digest(target)
    src = measure(source, 'metres')
    dst = measure(target, 'centimetres')
    assert src['triangles'] == source_row['triangles'], 'Source FBX triangle count differs from manifest: ' + entry['name']
    assert dst['triangles'] == entry['triangles'], ('Export is not reported render LOD0 geometry', entry['name'], dst['triangles'], entry['triangles'])
    expected_dimensions_cm = [entry['bounds_cm']['max'][i] - entry['bounds_cm']['min'][i] for i in range(3)]
    src_error_cm = maximum_sorted_dimension_error([v * 100 for v in src['dimensions_raw_units']], expected_dimensions_cm)
    dst_error_cm = maximum_sorted_dimension_error(dst['dimensions_raw_units'], expected_dimensions_cm)
    assert src_error_cm < BOUND_TOLERANCE_CM, ('Source dimensions/units differ from imported bounds', entry['name'], src['dimensions_raw_units'], expected_dimensions_cm)
    assert dst_error_cm < BOUND_TOLERANCE_CM, ('UE export dimensions/units differ from imported bounds', entry['name'], dst['dimensions_raw_units'], expected_dimensions_cm)
    assert src['surface_area_m2'] > 0, 'Source surface area is not positive: ' + entry['name']
    relative_error = abs(dst['surface_area_m2'] - src['surface_area_m2']) / src['surface_area_m2']
    return {'name': entry['name'], 'unreal_path': entry['path'],
            'source_file': source_row['exports']['fbx'], 'source_sha256': source_sha,
            'geometry_review_export': str(target.relative_to(PROJECT)), 'geometry_review_export_sha256': target_sha,
            'source_triangles': src['triangles'], 'unreal_triangles': dst['triangles'],
            'triangulation_triangle_delta': src['triangles'] - dst['triangles'],
            'source_surface_area_m2': src['surface_area_m2'], 'unreal_surface_area_m2': dst['surface_area_m2'],
            'absolute_area_error_m2': abs(dst['surface_area_m2'] - src['surface_area_m2']),
            'relative_area_error': relative_error, 'relative_area_tolerance': RELATIVE_AREA_TOLERANCE,
            'source_maximum_dimension_error_cm': src_error_cm, 'unreal_maximum_dimension_error_cm': dst_error_cm,
            'bound_tolerance_cm': BOUND_TOLERANCE_CM, 'source_measurement': src, 'unreal_measurement': dst,
            'success': relative_error <= RELATIVE_AREA_TOLERANCE,
            'pass': relative_error <= RELATIVE_AREA_TOLERANCE}


def main():
    destination = ROOT / 'UESurfaceReview.json'
    import_path = ROOT / 'UnrealImportReport.json'
    manifest_path = ROOT / 'AssetManifest.json'
    report = {'schema_version': 1, 'generator': 'Scripts/ue_review_surfaces.py',
              'generated_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'read_only_assets': True, 'relative_area_tolerance': RELATIVE_AREA_TOLERANCE,
              'source_coordinate_unit': 'metres', 'unreal_export_coordinate_unit': 'centimetres',
              'limits': 'Only aggregate surface area and axis-independent dimensions are compared. This does not establish per-face correspondence, identical topology, normals, winding, UV interpolation, manifoldness or visual fidelity. Equal areas can hide compensating geometry changes; actual UE rendering and the independent import checks remain necessary.',
              'success': False, 'assets': [], 'errors': []}
    try:
        imported = json.loads(import_path.read_text(encoding='utf-8'))
        manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
        report['import_report_sha256'] = digest(import_path)
        report['manifest_sha256'] = digest(manifest_path)
        report['generator_sha256'] = digest(Path(__file__))
        assert imported['success'] and not imported['errors'], 'Complete successful Unreal import report required before surface review'
        source_rows = {r['name']: r for r in manifest['assets'] + manifest['presets']}
        entries = [e for e in imported['assets'] if e.get('needs_surface_review')]
        report['required_names'] = sorted(e['name'] for e in entries)
        assert len({e['name'] for e in entries}) == len(entries), 'Duplicate surface-review import rows'
        for entry in entries:
            try:
                result = review(entry, source_rows[entry['name']], imported)
                report['assets'].append(result)
                print('M7_SURFACE_REVIEW', json.dumps({key: result[key] for key in ('name', 'source_surface_area_m2', 'unreal_surface_area_m2', 'relative_area_error', 'pass')}), flush=True)
                if not result['pass']:
                    report['errors'].append({'name': entry['name'], 'error': 'Aggregate surface-area relative error exceeds ' + str(RELATIVE_AREA_TOLERANCE)})
            except Exception:
                report['errors'].append({'name': entry['name'], 'error': traceback.format_exc()})
        # A different import report cannot silently reuse this result.
        assert digest(import_path) == report['import_report_sha256'], 'Import report changed while surface review ran'
        assert digest(manifest_path) == report['manifest_sha256'], 'Manifest changed while surface review ran'
        report['success'] = not report['errors'] and len(report['assets']) == len(entries)
        report['summary'] = {'required': len(entries), 'checked': len(report['assets']),
                             'passed': sum(bool(a['pass']) for a in report['assets']), 'error_count': len(report['errors'])}
    except Exception:
        report['errors'].append({'name': 'setup_or_report_binding', 'error': traceback.format_exc()})
    finally:
        destination.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print('M7_SURFACE_REVIEW_REPORT', str(destination), flush=True)
    assert report['success'], 'M7 surface review failed; inspect ' + str(destination)
    return report


if __name__ == '__main__':
    main()

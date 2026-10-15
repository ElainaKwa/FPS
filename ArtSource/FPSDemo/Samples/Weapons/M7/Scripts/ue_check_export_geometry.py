"""Inspect exported FBX triangles for coincident points and zero-area faces.

Pure Python/NumPy; does not import Blender/Unreal or alter any asset. FBX
geometry positions are the authored metre-space coordinates in this library;
model scale is 100 and FBX unit is centimetres. Degeneracy is reported both
at float64 source precision and at float32 centimetre import precision.
Predictions identify geometry candidates, not a substitute for UE readback.
"""
import argparse
import hashlib
import json
import math
import struct
import sys
import zlib
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT.parents[4]


class BinaryFBX:
    def __init__(self, path):
        self.path = Path(path)
        self.data = self.path.read_bytes()
        assert self.data[:23] == b'Kaydara FBX Binary  \x00\x1a\x00', 'Not a binary FBX: ' + str(path)
        self.version = struct.unpack_from('<I', self.data, 23)[0]
        self.header = '<QQQB' if self.version >= 7500 else '<IIIB'
        self.header_size = struct.calcsize(self.header)

    def property(self, pos, retain_array=False):
        kind = chr(self.data[pos])
        pos += 1
        scalars = {'Y': 'h', 'C': '?', 'F': 'f', 'D': 'd', 'I': 'i', 'L': 'q'}
        if kind in scalars:
            fmt = '<' + scalars[kind]
            return struct.unpack_from(fmt, self.data, pos)[0], pos + struct.calcsize(fmt)
        if kind in 'SR':
            count = struct.unpack_from('<I', self.data, pos)[0]
            pos += 4
            value = self.data[pos:pos + count]
            return (value.decode('utf-8', 'replace') if kind == 'S' else '<binary>'), pos + count
        if kind in 'fdlibc':
            count, encoding, length = struct.unpack_from('<III', self.data, pos)
            pos += 12
            end = pos + length
            if not retain_array:
                return {'array_type': kind, 'count': count}, end
            raw = self.data[pos:end]
            if encoding:
                assert encoding == 1, 'Unsupported FBX array encoding'
                raw = zlib.decompress(raw)
            dtype = {'f': '<f4', 'd': '<f8', 'l': '<i8', 'i': '<i4', 'b': '?', 'c': 'i1'}[kind]
            result = np.frombuffer(raw, dtype=dtype, count=count)
            return result, end
        raise ValueError('Unknown FBX property: ' + kind)

    def node(self, pos):
        end, count, prop_length, name_length = struct.unpack_from(self.header, self.data, pos)
        if not end:
            return None, pos + self.header_size
        pos += self.header_size
        name = self.data[pos:pos + name_length].decode('utf-8')
        pos += name_length
        props = []
        retain = name in {'Vertices', 'PolygonVertexIndex'}
        for _ in range(count):
            value, pos = self.property(pos, retain)
            props.append(value)
        children = []
        while pos < end - self.header_size:
            child, pos = self.node(pos)
            if child is not None:
                children.append(child)
        return {'name': name, 'properties': props, 'children': children}, end

    def geometry(self):
        pos = 27
        found = []
        while pos < len(self.data):
            node, pos = self.node(pos)
            if node is None:
                break
            if node['name'] != 'Objects':
                continue
            for child in node['children']:
                if child['name'] != 'Geometry' or child['properties'][-1] != 'Mesh':
                    continue
                values = {n['name']: n['properties'] for n in child['children']}
                found.append((child['properties'][1], values['Vertices'][0].reshape(-1, 3), values['PolygonVertexIndex'][0]))
        return found


def triangles(indices, vertices):
    ends = np.flatnonzero(indices < 0)
    lengths = np.diff(np.r_[-1, ends])
    assert len(ends), 'Empty FBX polygon array'
    values = indices.astype(np.int64)
    values[values < 0] = -values[values < 0] - 1
    if np.all(lengths == 3):
        return values.reshape(-1, 3), np.arange(len(ends)), {'method': 'Already triangular', 'polygon_count': len(ends), 'polygon_sizes': [3]}
    try:
        import bpy
    except ImportError as exc:
        raise RuntimeError('FBX contains n-gons; run this script through Blender --background --factory-startup --python <script> -- <arguments>. Only temporary raw coordinate meshes are created; no asset file is saved.') from exc
    faces = [values[start:end + 1].tolist() for start, end in zip(np.r_[0, ends[:-1] + 1], ends)]
    mesh = bpy.data.meshes.new('M7_READONLY_PREFLIGHT_COORDINATES')
    try:
        mesh.from_pydata(vertices.tolist(), [], faces)
        mesh.calc_loop_triangles()
        result = np.empty(len(mesh.loop_triangles) * 3, dtype=np.int32)
        mesh.loop_triangles.foreach_get('vertices', result)
        polygons = np.empty(len(mesh.loop_triangles), dtype=np.int32)
        mesh.loop_triangles.foreach_get('polygon_index', polygons)
        return result.reshape(-1, 3), polygons, {'method': 'Blender calc_loop_triangles on temporary raw FBX coordinate mesh',
            'blender_version': bpy.app.version_string, 'polygon_count': len(ends), 'polygon_sizes': np.unique(lengths).tolist()}
    finally:
        bpy.data.meshes.remove(mesh)


def area_and_edges(points):
    ab, ac, bc = points[:, 1] - points[:, 0], points[:, 2] - points[:, 0], points[:, 2] - points[:, 1]
    areas = np.linalg.norm(np.cross(ab, ac), axis=1) * .5
    edges = np.stack([np.linalg.norm(ab, axis=1), np.linalg.norm(ac, axis=1), np.linalg.norm(bc, axis=1)], axis=1)
    return areas, edges


def geometry_metrics(name, vertices, indices):
    ids, source_polygons, triangulation = triangles(indices, vertices)
    assert np.all(np.isfinite(vertices)), 'Nonfinite FBX positions'
    assert np.min(ids) >= 0 and np.max(ids) < len(vertices), 'Invalid FBX polygon indices'
    pts = vertices[ids]
    areas, edges = area_and_edges(pts)
    # Float32 vertex import in cm may collapse extremely close metre positions.
    pts32cm = (pts * 100).astype(np.float32)
    areas32cm, edges32cm = area_and_edges(pts32cm)
    repeated_index = np.any(np.stack([ids[:, 0] == ids[:, 1], ids[:, 0] == ids[:, 2], ids[:, 1] == ids[:, 2]], axis=1), axis=1)
    repeated64 = np.min(edges, axis=1) == 0
    repeated32 = np.min(edges32cm, axis=1) == 0
    zero64 = areas == 0
    zero32 = areas32cm == 0
    strong = repeated_index | repeated64 | repeated32 | zero64 | zero32
    # Common UE point-comparison threshold is 2e-5 cm. This is deliberately a
    # separate candidate set: actual import flags/build implementation decide.
    near = np.min(edges32cm, axis=1) <= 2e-5
    candidates = np.flatnonzero(strong | near)
    smallest = np.argsort(areas)[:min(12, len(areas))]
    inspect = sorted(set(candidates.tolist() + smallest.tolist()))
    samples = []
    for index in inspect:
        samples.append({'triangle_index_zero_based': int(index), 'source_polygon_zero_based': int(source_polygons[index]), 'vertex_indices': ids[index].tolist(),
                        'vertices_m': pts[index].tolist(), 'vertices_cm_float32': pts32cm[index].tolist(),
                        'area_m2_float64': float(areas[index]), 'area_cm2_float32': float(areas32cm[index]),
                        'edge_lengths_m_float64': edges[index].tolist(), 'edge_lengths_cm_float32': edges32cm[index].tolist(),
                        'repeated_index': bool(repeated_index[index]), 'repeated_position_float64': bool(repeated64[index]),
                        'repeated_position_float32_cm': bool(repeated32[index]), 'zero_area_float64': bool(zero64[index]),
                        'zero_area_float32_cm': bool(zero32[index]), 'strong_degenerate_candidate': bool(strong[index]),
                        'near_point_threshold_candidate': bool(near[index])})
    return {'geometry_name': name, 'vertices': len(vertices), 'triangles': len(ids), 'triangulation': triangulation,
            'minimum_area_m2_float64': float(np.min(areas)), 'minimum_area_cm2_float32': float(np.min(areas32cm)),
            'minimum_edge_m_float64': float(np.min(edges)), 'minimum_edge_cm_float32': float(np.min(edges32cm)),
            'repeated_index_count': int(np.count_nonzero(repeated_index)), 'repeated_position_float64_count': int(np.count_nonzero(repeated64)),
            'repeated_position_float32_cm_count': int(np.count_nonzero(repeated32)), 'zero_area_float64_count': int(np.count_nonzero(zero64)),
            'zero_area_float32_cm_count': int(np.count_nonzero(zero32)), 'strong_degenerate_candidate_count': int(np.count_nonzero(strong)),
            'near_point_threshold_candidate_count': int(np.count_nonzero(near)),
            'zero_area_float64_triangle_indices': np.flatnonzero(zero64).tolist(),
            'strong_degenerate_candidate_indices': np.flatnonzero(strong).tolist(), 'inspected_triangles': samples}


def inspect(row):
    path = ROOT / row['exports']['fbx']
    parsed = BinaryFBX(path)
    pieces = [geometry_metrics(*geometry) for geometry in parsed.geometry()]
    source_count = sum(p['triangles'] for p in pieces)
    zero_area_exact = sum(p['zero_area_float64_count'] for p in pieces)
    strong_count = sum(p['strong_degenerate_candidate_count'] for p in pieces)
    near_count = sum(p['near_point_threshold_candidate_count'] for p in pieces)
    assert source_count == row['triangles'], (row['name'], 'FBX source triangle count differs from manifest', source_count, row['triangles'])
    return {'name': row['name'], 'file': row['exports']['fbx'], 'fbx_sha256': hashlib.sha256(parsed.data).hexdigest(),
            'source_file': row['exports']['fbx'], 'source_sha256': hashlib.sha256(parsed.data).hexdigest(),
            'source_triangles': source_count, 'zero_area_triangles_exact': zero_area_exact,
            'expected_after_exact_zero_filter': source_count - zero_area_exact,
            'manifest_triangles': row['triangles'], 'source_fbx_triangles': source_count,
            'strong_degenerate_candidate_count': strong_count, 'near_point_threshold_candidate_count': near_count,
            'predicted_triangles_after_strong_degenerate_filter': source_count - strong_count,
            'minimum_area_m2_float64': min(p['minimum_area_m2_float64'] for p in pieces),
            'minimum_area_cm2_float32': min(p['minimum_area_cm2_float32'] for p in pieces), 'geometry': pieces}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--asset', action='append', help='Limit to one or more exact SM_M7 asset names')
    parser.add_argument('--output', type=Path, default=ROOT / 'UEGeometryPreflight.json')
    argv = sys.argv[sys.argv.index('--') + 1:] if '--' in sys.argv else ([] if '--python' in sys.argv else None)
    args = parser.parse_args(argv)
    manifest = json.loads((ROOT / 'AssetManifest.json').read_text(encoding='utf-8'))
    rows = manifest['assets'] + manifest['presets']
    if args.asset:
        wanted = set(args.asset)
        rows = [row for row in rows if row['name'] in wanted]
        assert {r['name'] for r in rows} == wanted, 'Unknown requested asset name'
    report = {'schema_version': 1, 'generator': 'Scripts/ue_check_export_geometry.py',
              'generator_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              'manifest_sha256': hashlib.sha256((ROOT / 'AssetManifest.json').read_bytes()).hexdigest(),
              'read_only_assets': True,
              'limits': 'Pure FBX-coordinate preflight. Strong candidates have exact coincident vertices or zero area at source/import float precision. Near threshold candidates are reported separately. Unreal readback determines the actual filtered count; this report does not waive any mismatch.',
              'source_units': 'metres', 'fbx_model_scale': 100, 'import_units': 'centimetres',
              'exact_filter_definition': 'Only triangles whose float64 source-coordinate cross-product norm is exactly 0.0 have zero_area_triangles_exact=True/count. Near-point thresholds and float32-only collapses are diagnostic and do not alter expected_after_exact_zero_filter.',
              'float32_point_comparison_candidate_threshold_cm': 2e-5, 'assets': [], 'errors': []}
    for row in rows:
        try:
            result = inspect(row)
            report['assets'].append(result)
            print(json.dumps({k: result[k] for k in ('name', 'source_triangles', 'zero_area_triangles_exact', 'expected_after_exact_zero_filter', 'strong_degenerate_candidate_count', 'near_point_threshold_candidate_count', 'minimum_area_m2_float64')}, ensure_ascii=False), flush=True)
        except Exception as exc:
            report['errors'].append({'name': row['name'], 'error': str(exc)})
            print('GEOMETRY_CHECK_ERROR', row['name'], str(exc), flush=True)
    report['success'] = not report['errors'] and len(report['assets']) == len(rows)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
    print('GEOMETRY_PREFLIGHT_REPORT', str(args.output), flush=True)
    assert report['success'], 'FBX geometry preflight failed; inspect report errors'


if __name__ == '__main__':
    main()

"""Initialize from the geometry milestone or its verified standalone package.

Never overwrite the packaged baseline, an existing current source or manifest.
"""
import hashlib,json,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
PROJECT=ROOT.parents[4] if len(ROOT.parents)>4 else None
OLD=PROJECT/'ArtSource/M7ReplicaV2' if PROJECT is not None else None
def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def copy_missing(source,destination):
    """Copy with exclusive creation so an existing file cannot be replaced."""
    if source is None or not source.is_file():return False
    with source.open('rb') as incoming:
        try:outgoing=destination.open('xb')
        except FileExistsError:return False
        with outgoing:shutil.copyfileobj(incoming,outgoing)
    return True

def main():
    previous=OLD/'M7_Modular_Replica_V2.blend' if OLD is not None else None
    previous_available=previous is not None and previous.is_file()
    snapshot=ROOT/'Revisions/GeometryV2/Source/M7_PreMaterial.blend'
    manifest_file=ROOT/'AssetManifest.json'
    manifest=json.loads(manifest_file.read_text(encoding='utf-8')) if manifest_file.is_file() else None
    old_manifest=OLD/'AssetManifest.json' if OLD is not None else None
    if manifest is None and (old_manifest is None or not old_manifest.is_file()):
        raise FileNotFoundError('Keep the packaged AssetManifest.json and baseline, or provide the historical M7 manifest for first initialization.')
    if not snapshot.is_file() and not previous_available:
        raise FileNotFoundError('Neither the packaged M7_PreMaterial.blend nor the historical geometry source is available.')
    expected_hash=(manifest or {}).get('baseline',{}).get('sha256')
    if not previous_available and not expected_hash:
        raise ValueError('Standalone preparation requires baseline.sha256 in the packaged AssetManifest.json.')
    previous_hash=sha(previous) if previous_available else None
    if expected_hash and previous_hash and previous_hash!=expected_hash:
        raise ValueError('Historical source differs from the recorded baseline SHA-256; no baseline may be created or overwritten.')
    for name in ('Source','Scripts','Textures/Baked','Previews','Exports','Revisions/GeometryV2/Source','Reference'):
        (ROOT/name).mkdir(parents=True,exist_ok=True)
    if not snapshot.exists():copy_missing(previous,snapshot)
    baseline_hash=sha(snapshot)
    if expected_hash and baseline_hash!=expected_hash:
        raise ValueError('Packaged baseline differs from the recorded SHA-256; it must not be overwritten.')
    if previous_available and previous_hash!=baseline_hash:
        raise ValueError('Historical source and packaged baseline differ; neither file may be overwritten.')
    target=ROOT/'Source/M7_Material_Study.blend'
    if not target.exists():copy_missing(snapshot,target)
    if manifest is None:
        manifest=json.loads(old_manifest.read_text(encoding='utf-8'))
        manifest['source_file']='Source/M7_Material_Study.blend'
        manifest['library']='M7_Material_Study'
        manifest['material_report']='MaterialReport.json'
        manifest['baseline_files']=['Revisions/GeometryV2/Source/M7_PreMaterial.blend']
        manifest['baseline']={'historical_source':previous.relative_to(PROJECT).as_posix(),'sha256':baseline_hash,
                              'status':'Historical geometry and pre-material input; never edited by the material generator.'}
        manifest['appearance']='Black coated metal, restrained wear, fine grain, distinct polymer and rubber.'
        manifest['unreal_import_status']='not imported or tested in Unreal Editor'
        manifest['visual_acceptance']='pending user review'
        with manifest_file.open('x',encoding='utf-8') as output:
            output.write(json.dumps(manifest,ensure_ascii=False,indent=2))
    for name in ('Preview_Equipped_Tidal.png','Preview_Receiver_Magazine_Detail.png','Preview_ResonanceGen1_Detail.png','Preview_UpperRail_Detail.png'):
        before=ROOT/'Previews'/('Before_'+name.removeprefix('Preview_'))
        if not before.exists():copy_missing(OLD/name if OLD is not None else None,before)
    script=ROOT/'Scripts/switch_barrel.py'
    old_script=OLD/'Source/switch_barrel.py' if OLD is not None else None
    if not script.exists() and old_script is not None and old_script.is_file():
        body=old_script.read_text(encoding='utf-8')
        body=body.replace("Path(bpy.data.filepath).resolve().parent","Path(bpy.data.filepath).resolve().parents[1]")
        with script.open('x',encoding='utf-8') as output:output.write(body)
    reference=OLD/'Reference/ReferenceManifest.json' if OLD is not None else None
    if not (ROOT/'Reference/ReferenceManifest.json').exists():copy_missing(reference,ROOT/'Reference/ReferenceManifest.json')
    print(json.dumps({'sample':str(ROOT),'baseline_sha256':baseline_hash,
                      'baseline_mode':'historical source verified' if previous_available else 'verified packaged baseline',
                      'historical_source_available':previous_available,
                      'previous_unchanged':sha(previous)==baseline_hash if previous_available else None}))
if __name__=='__main__':main()

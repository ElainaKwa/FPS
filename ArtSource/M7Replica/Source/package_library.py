"""Package the verified model, exports, textures and previews."""
import hashlib,json,zipfile
from pathlib import Path

root=Path(__file__).resolve().parents[1]
validation=json.loads((root/'Validation.json').read_text(encoding='utf-8'))
assert validation['success'], 'Package only a passing asset library.'
source=root/'M7_Modular_Replica.blend'
assert hashlib.sha256(source.read_bytes()).hexdigest()==validation['source_sha256_after']
files=[source,root/'AssetManifest.json',root/'Validation.json',root/'ProfileComparison.json']
files += sorted((root/'Exports').glob('*.*'))
files += sorted((root/'Textures').glob('*.png'))
files += [root/name for name in ('Preview_Default.png','Preview_Suppressed.png',
          'Preview_Attachments.png','Preview_Core_Detail.png','Preview_Reference_Profile.png')]
files += sorted((root/'Source').glob('*.py'))
files += [root/'Reference/ReferenceManifest.json']
assert len(list((root/'Exports').glob('*.fbx')))==17
assert len(list((root/'Exports').glob('*.glb')))==17
target=root/'M7_Modular_Assets.zip'
with zipfile.ZipFile(target,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=6) as archive:
    for path in files:
        assert path.is_file() and path.resolve().is_relative_to(root)
        archive.write(path,path.relative_to(root).as_posix())
with zipfile.ZipFile(target) as archive:
    assert archive.testzip() is None
print(json.dumps({'archive':str(target),'files':len(files),'bytes':target.stat().st_size}))

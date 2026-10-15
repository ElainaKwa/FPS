"""Package the current verified M7 PBR sample, without game reference media."""

import hashlib
import importlib.util
import json
import re
import zipfile
from html.parser import HTMLParser
from pathlib import Path
from urllib.parse import unquote, urlsplit


ROOT = Path(__file__).resolve().parents[1]
BASELINE_SUFFIXES = {".blend", ".json", ".md", ".txt", ".csv", ".sha256", ".toml"}
CONFIG_SUFFIXES = {".json", ".toml", ".yaml", ".yml", ".ini", ".cfg", ".md"}
REQUIRED_ATTACHMENTS = {"SM_M7_Stock_ShadowRail", "SM_M7_Grip_Phantom"}


def digest(path):
    result = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            result.update(block)
    return result.hexdigest()


def inside(relative):
    path = (ROOT / relative).resolve()
    if not path.is_relative_to(ROOT.resolve()):
        raise ValueError("Package input lies outside this sample: " + str(path))
    return path


def compare_references(page, files):
    """Check both initial HTML images and JavaScript view-switch image paths."""
    class Links(HTMLParser):
        def __init__(self):
            super().__init__(); self.paths = set()

        def handle_starttag(self, tag, attrs):
            self.paths.update(value for key, value in attrs
                              if key in {"src", "href"} and value)

    source = page.read_text(encoding="utf-8")
    links = Links(); links.feed(source)
    links.paths.update(match[1] for match in
                       re.findall(r"\b(?:before|after)\s*:\s*(['\"])(.*?)\1", source))
    included = {path.resolve() for path in files}
    local = set()
    for value in links.paths:
        url = urlsplit(value)
        if url.scheme or url.netloc or not url.path:
            continue
        target = (page.parent / unquote(url.path)).resolve()
        if not target.is_relative_to(ROOT.resolve()):
            raise RuntimeError("Comparison page points outside the sample: " + value)
        if not target.is_file() or target not in included:
            raise RuntimeError("Comparison page dependency is absent from the package: " + value)
        local.add(target)
    return local


def main():
    manifest_path, validation_path = ROOT / "AssetManifest.json", ROOT / "Validation.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    validation = json.loads(validation_path.read_text(encoding="utf-8"))
    if not validation.get("success"):
        raise RuntimeError("Only package the successfully validated material study.")
    if len(manifest["assets"]) != 19 or len(manifest["presets"]) != 3:
        raise RuntimeError("Sample must contain nineteen assets and three assembly presets.")
    if not REQUIRED_ATTACHMENTS.issubset({asset["name"] for asset in manifest["assets"]}):
        raise RuntimeError("The sample must include independent ShadowRail stock and Phantom grip assets.")
    source = inside(manifest.get("source_file", "Source/M7_Material_Study.blend"))
    if digest(source) != validation["source_sha256_after"]:
        raise RuntimeError("Source .blend changed after validation; validate the final saved file again.")
    for relative, expected in validation.get("validated_files", {}).items():
        path = inside(relative)
        if not path.is_file() or digest(path) != expected:
            raise RuntimeError("Validated texture/export/manifest changed: " + relative)
    freeze_spec = importlib.util.spec_from_file_location(
        "m7_frozen_attachment_package", ROOT / "Scripts/freeze_tidal_attachment.py")
    freeze = importlib.util.module_from_spec(freeze_spec)
    freeze_spec.loader.exec_module(freeze)
    registry = freeze.check_frozen_files(ROOT, manifest)
    frozen = registry["assets"][freeze.ASSET_NAME]
    proof = validation.get("frozen_attachments", {}).get(freeze.ASSET_NAME, {})
    if (proof.get("source_sha256") != frozen["source_snapshot"]["sha256"]
            or proof.get("files_unchanged") is not True):
        raise RuntimeError("Current successful Validation must confirm the frozen Tidal source and files.")
    if validation.get("validated_files", {}).get(freeze.REGISTRY_NAME) != digest(ROOT / freeze.REGISTRY_NAME):
        raise RuntimeError("AttachmentRegistry.json is not bound to the current successful Validation.")
    report = inside(manifest.get("material_report", "MaterialReport.json"))
    if not report.is_file():
        raise RuntimeError("The current material baking/report artifact is missing.")
    files = {source, manifest_path.resolve(), validation_path.resolve(), report}
    exports = {inside(path) for row in manifest["assets"] + manifest["presets"]
               for kind, path in row["exports"].items() if kind in {"fbx", "glb"}}
    if len(exports) != 44 or any(not path.is_file() for path in exports):
        raise RuntimeError("Expected all forty-four declared FBX/GLB export files.")
    files.update(exports)
    maps = {inside(relative) for asset in manifest["assets"]
            for slots in asset.get("texture_sets", {}).values()
            for role, relative in slots.items() if role in {"BaseColor", "Normal", "ORM"}}
    if not maps or any(not path.is_file() for path in maps):
        raise RuntimeError("A declared baked material atlas is missing.")
    files.update((ROOT / "Textures/Baked").rglob("*.png"))
    files.update(maps)
    previews = list((ROOT / "Previews").glob("Preview_*.png"))
    if not previews:
        raise RuntimeError("Rendered material previews are missing.")
    files.update(previews)
    before_previews = list((ROOT / "Previews").glob("Before_*.png"))
    files.update(before_previews)
    compare = ROOT / "Previews/Compare.html"
    if not compare.is_file():
        raise RuntimeError("The rendered before/after comparison page is missing.")
    files.add(compare)
    scripts = list((ROOT / "Scripts").glob("*.py"))
    if not scripts:
        raise RuntimeError("Sample scripts are missing.")
    files.update(scripts)
    script_configs = list((ROOT / "Scripts").glob("*.json"))
    material_settings = ROOT / "Scripts/material_settings.json"
    if not material_settings.is_file():
        raise RuntimeError("The authoritative material_settings.json is missing.")
    files.update(script_configs)
    readme = ROOT / "README.md"
    if not readme.is_file():
        raise RuntimeError("The sample README.md is missing.")
    files.add(readme)
    # Include material/bake configuration and machine-readable reports only.
    # Reference/, images from game websites and source video are never scanned.
    files.update(path for path in ROOT.glob("*.json") if path.is_file())
    files.update(path for path in (ROOT / "Config").rglob("*")
                 if path.is_file() and path.suffix.lower() in CONFIG_SUFFIXES)
    baseline = set()
    entries = manifest.get("baseline_files", [])
    if isinstance(entries, str):
        entries = [entries]
    if not entries and (ROOT / "Baseline").is_dir():
        entries = ["Baseline"]
    if not entries:
        entries = [path.relative_to(ROOT).as_posix() for path in ROOT.glob("*Baseline*.json")]
    for entry in entries:
        path = inside(entry)
        candidates = [path] if path.is_file() else list(path.rglob("*"))
        baseline.update(file for file in candidates if file.is_file() and file.suffix.lower() in BASELINE_SUFFIXES)
    if not baseline:
        raise RuntimeError("Declare baseline_files or provide a read-only Baseline/ snapshot.")
    files.update(baseline)
    comparison_inputs = compare_references(compare, files)
    target = inside(manifest.get("package_filename", "M7_Material_Sample.zip"))
    if target.suffix != ".zip":
        raise RuntimeError("Package output must be a .zip inside the sample.")
    with zipfile.ZipFile(target, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for file in sorted({path.resolve() for path in files}):
            if not file.is_file() or not file.is_relative_to(ROOT.resolve()):
                raise RuntimeError("Invalid package input: " + str(file))
            relative = file.relative_to(ROOT.resolve()).as_posix()
            if "Reference" in Path(relative).parts:
                raise RuntimeError("Reference media directory is excluded from this sample package.")
            if file in {path.resolve() for path in baseline}:
                info = zipfile.ZipInfo.from_file(file, relative)
                info.compress_type = zipfile.ZIP_DEFLATED
                # Record a read-only baseline in Unix permissions and the DOS
                # read-only attribute while leaving original files untouched.
                mode = (file.stat().st_mode & ~0o222) & 0xffff
                info.external_attr = (mode << 16) | 1
                archive.writestr(info, file.read_bytes())
            else:
                archive.write(file, relative)
    with zipfile.ZipFile(target) as archive:
        if archive.testzip() is not None:
            raise RuntimeError("Material sample ZIP CRC check failed.")
        members = set(archive.namelist())
        required = exports | maps | set(script_configs) | set(before_previews) | comparison_inputs | {
            compare, ROOT / freeze.REGISTRY_NAME, ROOT / "Scripts/freeze_tidal_attachment.py"}
        if not all(path.resolve().relative_to(ROOT.resolve()).as_posix() in members for path in required):
            raise RuntimeError("An export, texture, configuration or comparison dependency is missing from the archive.")
    print(json.dumps({"archive": str(target), "files": len(files), "exports": len(exports),
                      "baked_maps": len(maps), "read_only_baseline_files": len(baseline),
                      "bytes": target.stat().st_size}))


if __name__ == "__main__":
    main()

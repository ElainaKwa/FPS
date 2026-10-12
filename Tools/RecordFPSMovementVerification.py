"""Publish a portable index from closed FPS movement harness evidence; never edit it by hand."""
import argparse
import hashlib
import json
import re
import sys
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def local(value):
    path = Path(value)
    return path if path.is_absolute() else ROOT / path


def relative(path):
    return path.resolve().relative_to(ROOT).as_posix()


def read_record(value):
    path = local(value)
    data = json.loads(path.read_text(encoding="utf-8-sig"))
    if data.get("result", data.get("status")) not in {"PASS", "PASS_ADMISSION_REJECTION"}:
        raise ValueError(f"Evidence has not passed: {path}")
    return path, data


def file_record(row):
    path = local(row.get("path", row.get("snapshot")))
    digest = sha(path)
    if digest != row["sha256"].lower():
        raise ValueError(f"Evidence file changed after its harness closed: {path}")
    return {"path": relative(path), "sha256": digest, "bytes": path.stat().st_size}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", action="append", required=True)
    parser.add_argument("--run", action="append", required=True, help="label=run.json")
    parser.add_argument("--assets", action="append", default=[])
    parser.add_argument("--output", default="Docs/FPS/Characters/Generated/MovementVerification.json")
    args = parser.parse_args()
    builds, modules = [], set()
    for value in args.build:
        path, data = read_record(value)
        modules.add(data["module"]["sha256"].lower())
        builds.append({"record": relative(path), "sha256": sha(path), "data": data})
    runs = []
    labels = set()
    for value in args.run:
        label, separator, value = value.partition("=")
        if not separator or label in labels:
            raise ValueError("Run arguments require unique label=record entries")
        labels.add(label)
        path, data = read_record(value)
        module = data.get("editor_module", data.get("module"))
        if not module or module["sha256"].lower() not in modules:
            raise ValueError(f"Run module has no supplied frozen build: {label}")
        if "VISUAL_INCOMPLETE" in data.get("acceptance_status", ""):
            raise ValueError(f"Incomplete visual record cannot be a passing UI entry: {label}")
        files, observations = [], []
        for row in data.get("logs", []) + data.get("captures", []):
            files.append(file_record(row))
            source = local(row.get("path", row.get("snapshot")))
            if source.suffix.lower() == ".log":
                lines = source.read_text(encoding="utf-8-sig", errors="replace").splitlines()
                if any(re.search(r"FPS_\w*SMOKE FAIL\b|FPS_MOVEMENT_UI_INPUT FAIL\b|Fatal error[:!]|Assertion failed:", line) for line in lines):
                    raise ValueError(f"Runtime failure remains in supplied passing record: {source}")
                observations.extend(line for line in lines if re.search(
                    r"FPS_MOVEMENT_SMOKE (?:MEASURE|PASS .*COMPLETE)|FPS_MOVEMENT_UI_INPUT|FPS_TEACHING_SMOKE.*COMPLETE|FPS_CONTENT (?:MISMATCH|REJECT|RETURN|RELEASE)", line))
        if not files:
            raise ValueError(f"No closed logs/captures in run: {label}")
        metadata = {key: value for key, value in data.items() if key not in {"logs", "captures"}}
        runs.append({"label": label, "record": relative(path), "sha256": sha(path),
                     "data": metadata, "files": files, "observations": observations})
    required = {"lan-zero", "lan-100ms-1percent", "training-visual", "teaching-movement", "teaching-gas", "movement-mismatch", "input-ui"}
    if not required.issubset(labels):
        raise ValueError(f"Missing required validation entries: {sorted(required - labels)}")
    assets = []
    for value in args.assets:
        path, data = read_record(value)
        assets.append({"record": relative(path), "sha256": sha(path), "data": data})
    result = {
        "purpose": "自动生成资料", "scope": "FPS movement and stamina verification",
        "status": "Automated server/client and rendering checks passed; manual experience remains unverified",
        "generated_utc": datetime.now(timezone.utc).isoformat(),
        "generator": relative(Path(__file__)), "generator_sha256": sha(Path(__file__)),
        "generation_arguments": sys.argv[1:],
        "maintenance": "Do not edit manually; regenerate from the named closed harness records.",
        "builds": builds, "runs": runs, "ui_assets": assets,
        "limits": ["Editor -game independent processes; no new Cook or standalone Server Target package",
                   "Native input and actual GAS with opt-in pose/resource/collision fixtures; no hardware keyboard claim",
                   "Screenshots prove rendered controls, not manual movement feel or every existing-map obstacle route",
                   "Local emulated network conditions; no real WAN, controller, stress or time-cheat penetration test",
                   "The workspace includes parallel attachment/art changes; this index validates the movement scope only"],
    }
    output = local(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"FPS_MOVEMENT_VERIFICATION PASS runs={len(runs)} output={relative(output)}")


if __name__ == "__main__":
    main()

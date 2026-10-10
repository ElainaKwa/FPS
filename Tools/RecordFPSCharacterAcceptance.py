"""Validate completed local acceptance records and export a portable evidence index."""

import argparse
import hashlib
import json
import re
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SUITES = {
    "Drops", "DropsLag", "DropsLifetime", "TrainingDrops", "Loadout",
    "TrainingLoadout", "Characters", "Maps", "Content", "Weapons",
    "Abilities", "Teaching", "Presentation", "Audio",
}
FAILURES = re.compile(r"FPS_[A-Z0-9_]+ FAIL|Fatal error:|Assertion failed:|TravelFailure:")


def sha(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def path(value):
    item = Path(value)
    return item if item.is_absolute() else ROOT / item


def portable(value):
    if isinstance(value, dict):
        return {key: portable(item) for key, item in value.items()}
    if isinstance(value, list):
        return [portable(item) for item in value]
    if isinstance(value, str):
        return value.replace(str(ROOT) + "\\", "").replace(
            str(ROOT).replace("\\", "/") + "/", "").replace("\\", "/")
    return value


def verify_hashes(value):
    """Check the evidence hashes recorded by each harness, including build inputs."""
    if isinstance(value, dict):
        location = value.get("snapshot", value.get("path"))
        if location and value.get("sha256"):
            target = path(location)
            if not target.is_file() or sha(target) != value["sha256"].lower():
                raise ValueError(f"Evidence changed or missing: {target}")
        for item in value.values():
            verify_hashes(item)
    elif isinstance(value, list):
        for item in value:
            verify_hashes(item)


def read_record(filename, results, verify=True):
    filename = path(filename).resolve()
    data = json.loads(filename.read_text(encoding="utf-8-sig"))
    if data.get("result") not in results:
        raise ValueError(f"Incomplete or unexpected result: {filename}: {data.get('result')}")
    if verify:
        verify_hashes(data)
    return {"record": portable(str(filename)), "sha256": sha(filename), "data": portable(data)}


def read_log(filename):
    data = path(filename).read_text(encoding="utf-8-sig", errors="replace")
    if FAILURES.search(data):
        raise ValueError(f"Failure marker in accepted log: {filename}")
    return data


def verify_dedicated(record, full):
    data = record["data"]
    expected = 10 if full else 2
    active = [item for item in data["verified_logs"] if item["active"]]
    if (data["expected_players"] != expected or len(active) != expected + 1
            or sum(item["role"] == "SERVER" for item in active) != 1
            or not all(item["fresh"] for item in data["verified_logs"])
            or data["expected_server_netmode"] != 1 or data["expected_client_netmode"] != 3
            or data["error"]):
        raise ValueError("Dedicated record has incomplete participants or wrong network mode")
    for item in data["verified_logs"]:
        log = read_log(item["path"])
        if not item["active"]:
            continue
        if full:
            if not item["complete"] or not re.search(
                    rf"FPS_ACCEPT PASS {item['role']} COMPLETE .*netmode={1 if item['role'] == 'SERVER' else 3}\b", log):
                raise ValueError(f"Active participant did not complete: {item['path']}")
            if item["role"] == "SERVER":
                completion = re.search(r"FPS_ACCEPT PASS SERVER COMPLETE .*observedWall=([\d.]+)", log)
                if not completion or float(completion[1]) < 300:
                    raise ValueError("Full round did not observe the actual 300 seconds")
        elif f"FPS_ACCEPT SAMPLE role={item['role']} netmode={1 if item['role'] == 'SERVER' else 3}" not in log:
            raise ValueError(f"Sample participant has no actual network observation: {item['path']}")
    if full:
        events = {item["event"] for item in data["reconnect_events"]}
        if (data["round_seconds"] != 300 or data["sample_seconds"] != 0 or not data["reconnect"]
                or not {"owned-client-stop", "real-client-rejoin"}.issubset(events)):
            raise ValueError("Full round requires real duration and disconnect/rejoin events")
    elif data["sample_seconds"] != 40:
        raise ValueError("Expected the distinct forty-second prerequisite sample")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", required=True)
    parser.add_argument("--sample", required=True)
    parser.add_argument("--round", required=True)
    parser.add_argument("--travel", action="append", required=True)
    parser.add_argument("--package-run", required=True)
    parser.add_argument("--failed-attempt", action="append", default=[])
    parser.add_argument("--output", default="Docs/FPS/Characters/Generated/AcceptanceSummary.json")
    args = parser.parse_args()
    build = read_record(args.build, {"PASS"})
    if (build["data"]["editor_exit_code"] != 0 or build["data"]["package_exit_code"] != 0
            or not build["data"]["input_files"] or not build["data"]["artifacts"]):
        raise ValueError("Build is incomplete or failed")
    artifacts = build["data"]["artifacts"]
    game = next(item for item in artifacts if item["path"].endswith("MCP_Test.exe"))
    module = next(item for item in artifacts if item["path"].endswith("UnrealEditor-MCP_Test.dll"))
    sample = read_record(args.sample, {"PASS_SAMPLE_ONLY"})
    round_record = read_record(args.round, {"PASS_FULL_ROUND"})
    verify_dedicated(sample, full=False)
    verify_dedicated(round_record, full=True)
    for run in (sample, round_record):
        data = run["data"]
        if data["client_binary"]["sha256"] != game["sha256"]:
            raise ValueError("Dedicated client version differs from the accepted build")
        if not data.get("editor_module") or data["editor_module"]["sha256"] != module["sha256"]:
            raise ValueError("Dedicated Editor module differs from the accepted build")
    travel = [read_record(item, {"passed"}) for item in args.travel]
    for item in travel:
        if not item["data"]["packaged"] or not any(
            binary["sha256"] == game["sha256"] for binary in item["data"]["binaries"]
        ):
            raise ValueError("Travel must use the newly accepted package")
        log = path(item["record"]).parent / "travel.log"
        if sha(log) != item["data"]["log_sha256"]:
            raise ValueError(f"Travel log changed: {log}")
        if "FPS_CHARACTER_TRAVEL PASS SERVER COMPLETE phases=5" not in read_log(log):
            raise ValueError("Travel did not complete all five actual world entries")
    packages = [read_record(item, {"PASS"}) for item in sorted(path(args.package_run).glob("*/run.json"))]
    if {item["data"]["suite"] for item in packages} != SUITES or len(packages) != len(SUITES):
        raise ValueError("Expected exactly the fourteen package suites")
    if any(item["data"]["executable_sha256"] != game["sha256"] for item in packages):
        raise ValueError("Package suite version differs from the accepted build")
    for item in packages:
        data = item["data"]
        if data["exit_code"] != 0 or not data["logs"] or len(data["completion_lines"]) != len(data["logs"]):
            raise ValueError(f"Package suite evidence is incomplete: {data['suite']}")
        for log, completion in zip(data["logs"], data["completion_lines"]):
            if not completion or completion not in read_log(log["path"]):
                raise ValueError(f"Package completion marker missing: {log['path']}")
    content = [{"path": portable(str(item)), "sha256": sha(item)} for folder in ("Modes", "Maps")
               for item in sorted((ROOT / "Content/FPSDemo" / folder).glob("*")) if item.is_file()]
    output = path(args.output).resolve()
    output.relative_to(ROOT / "Docs/FPS/Characters/Generated")
    summary = {
        "purpose": "自动生成资料", "scope": "FPS Characters expanded acceptance",
        "status": "Automated checks passed; manual experience and independent Server Target unverified",
        "generated_utc": datetime.now(timezone.utc).isoformat(),
        "generator": "Tools/RecordFPSCharacterAcceptance.py", "generator_sha256": sha(Path(__file__)),
        "maintenance": "Do not edit manually; regenerate from the named harness records.",
        "build": build, "content": content, "sample": sample, "full_round": round_record,
        "travel": travel, "package_suites": packages,
        "failed_attempts": [read_record(item, {"FAIL", "failed", "ABORTED"}, verify=False) for item in args.failed_attempt],
    }
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(summary, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"FPS_CHARACTER_EVIDENCE PASS suites={len(packages)} output={output}")


if __name__ == "__main__":
    main()

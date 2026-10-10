"""Analyze observed prediction kicks and authority aim; angular differences are measurements, not lag compensation."""
import argparse
import json
import math
import re
import statistics
from pathlib import Path


def samples(path, role):
    result = {}
    for line in Path(path).read_text(encoding="utf-8", errors="replace").splitlines():
        if f"FPS_RECOIL_SAMPLE {role} " in line:
            fields = {k: float(v) for k, v in re.findall(r"(\w+)=(-?\d+(?:\.\d+)?)", line)}
            result[int(fields["serial"])] = fields
    return result


def delta(a, b):
    return (a - b + 180) % 360 - 180


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("server")
    parser.add_argument("client")
    parser.add_argument("--output")
    parser.add_argument("--uncompensated", action="store_true")
    args = parser.parse_args()
    auth, pred = samples(args.server, "AUTH"), samples(args.client, "PRED")
    assert set(auth) == set(pred) == set(range(1, 31)), "Expected matching 30 authority/prediction serials"
    errors, angles, delays = [], [], []
    for serial, p in pred.items():
        errors.extend([abs(delta(p["pitch"], p["beforePitch"]) - p["kickPitch"]),
                       abs(delta(p["yaw"], p["beforeYaw"]) - p["kickYaw"])])
        a = auth[serial]
        angles.append(math.hypot(delta(a["pitch"], p["pitch"]), delta(a["yaw"], p["yaw"])))
        delays.append(a["time"] - p["time"])
    assert max(errors) < 0.0001, "Observed control rotation kick differs from curve increment"
    hits = sum(int(a["hitPawn"]) for a in auth.values())
    pitch_span = max(delta(a["pitch"], auth[1]["pitch"]) for a in auth.values())
    if args.uncompensated:
        assert pitch_span > 20, "Uncompensated recoil did not propagate to authority trace aim"
    else:
        assert hits > 0, "No actual moving target trace hits"
    report = {"shots": len(auth), "hits": hits, "kick_max_error_deg": max(errors),
              "authority_vs_predicted_after_kick_angle_deg": {"median": statistics.median(angles), "max": max(angles)},
              "authority_minus_predicted_server_time_seconds": {"median": statistics.median(delays), "min": min(delays), "max": max(delays)},
              "authority_pitch_span_deg": pitch_span,
              "scope": "Moving replicated target; " + ("client stops tracking when firing starts." if args.uncompensated else "client tracks each frame.") + " Differences include input replication, latency and recoil timing; no latency-compensation claim."}
    output = json.dumps(report, indent=2)
    print(output)
    if args.output:
        Path(args.output).write_text(output + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()

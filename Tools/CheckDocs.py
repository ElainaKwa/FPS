"""Dependency-free documentation checks. Run from any cwd; --root supports fixtures.

Supports inline Markdown links/images and reference definitions; fenced code and
inline code are ignored. External URLs, HTML links and heading anchors need review.
"""
import argparse
from datetime import date
import json
import os
from pathlib import Path
import re
import sys
from urllib.parse import unquote, urlsplit


def prose(text, strip_inline=True):
    lines, fence = [], None
    for line in text.splitlines():
        marker = re.match(r"^\s*(`{3,}|~{3,})", line)
        if marker:
            token = marker[1]
            if fence is None:
                fence = token
            elif token[0] == fence[0] and len(token) >= len(fence):
                fence = None
            continue
        if fence is None:
            lines.append(line)
    result = "\n".join(lines)
    return re.sub(r"(`+).*?\1", "", result) if strip_inline else result


def destinations(text):
    # Angle brackets allow spaces; bare URLs support one nested parenthesis pair.
    dest = r"(<[^>\n]+>|[^\s()]+(?:\([^()]*\)[^\s()]*)*)"
    yield from re.findall(r"\]\(\s*" + dest, text)
    yield from re.findall(r"(?m)^\s*\[[^]\n]+\]:\s*" + dest, text)


def check(root):
    root = root.resolve()
    docs = root / "Docs"
    findings = set()
    texts, edges = {}, {}
    def add(path, rule):
        findings.add((path, rule))
    for file in sorted(docs.rglob("*.md")):
        path = file.relative_to(root).as_posix()
        try:
            raw = file.read_text(encoding="utf-8-sig")
        except UnicodeDecodeError:
            add(path, "encoding")
            continue
        if "\ufffd" in raw:
            add(path, "encoding")
        text = prose(raw)
        texts[path] = text
        header = re.split(r"(?m)^##\s", prose(raw, strip_inline=False), maxsplit=1)[0]
        for field in ("用途", "适用范围", "状态", "最后更新"):
            value = re.search(r"(?m)^\s*(?:-\s*)?" + field + r"[：:]\s*(\S[^\n]*)$", header)
            if not value:
                add(path, "metadata:" + field)
            elif field == "最后更新":
                try:
                    date.fromisoformat(value[1].strip())
                except ValueError:
                    add(path, "metadata:date")
        parts = file.relative_to(docs).parts
        if any(not re.fullmatch(r"[A-Za-z0-9_.-]+", p) for p in parts):
            add(path, "naming")
        if re.search(r"(?:^|[_.-])(Final|Latest|New)(?:[_.-]|$)", file.stem, re.I):
            add(path, "naming")
        if len(parts) == 1 and file.name not in ("README.md", "DocumentationRules.md"):
            add(path, "root-placement")
        edges[path] = set()
        for dest in destinations(text):
            dest = dest.strip("<>")
            if re.match(r"^[A-Za-z]:[\\/]", dest) or dest.startswith(("/", "\\")):
                add(path, "absolute-link:" + dest)
                continue
            parsed = urlsplit(dest)
            if parsed.scheme or parsed.netloc or not parsed.path:
                continue
            target = Path(os.path.abspath(file.parent / unquote(parsed.path)))
            if not target.exists():
                add(path, "broken-link:" + dest)
                continue
            # Windows existence checks ignore case; inspect directory entries explicitly.
            cursor = target
            while cursor != cursor.parent:
                if cursor.name not in {p.name for p in cursor.parent.iterdir()}:
                    add(path, "link-case:" + dest)
                    break
                cursor = cursor.parent
            target = target.resolve()
            if target.is_dir():
                target = target / "README.md"
            try:
                edges[path].add(target.relative_to(root).as_posix())
            except ValueError:
                add(path, "outside-project-link:" + dest)
    seen, pending = set(), ["Docs/README.md"]
    if "Docs/README.md" not in texts:
        add("Docs/README.md", "missing-index")
    while pending:
        path = pending.pop()
        if path not in seen:
            seen.add(path)
            pending.extend(edges.get(path, ()))
    for path in texts.keys() - seen:
        add(path, "unreachable")
    baseline_file = root / "Tools/DocCheckBaseline.json"
    baseline = json.loads(baseline_file.read_text(encoding="utf-8")) if baseline_file.exists() else {}
    allowed = {(p, rule) for p, rules in baseline.get("exceptions", {}).items() for rule in rules}
    # Stale exceptions must be removed: prevents permanent broad exemptions.
    errors = sorted(findings - allowed)
    errors += sorted((p, "stale-exception:" + rule) for p, rule in allowed - findings)
    return errors, len(findings & allowed), len(texts)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()
    try:
        errors, exempt, count = check(args.root)
    except (OSError, ValueError) as exc:
        print(f"ERROR: {exc}")
        return 1
    for path, rule in errors:
        print(f"FAIL {path}: {rule}")
    print(f"Docs: {count}; errors: {len(errors)}; legacy exceptions: {exempt}")
    return int(bool(errors))


if __name__ == "__main__":
    sys.exit(main())

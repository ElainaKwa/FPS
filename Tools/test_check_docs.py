import json
from pathlib import Path
import shutil
import uuid
import unittest

from CheckDocs import check

HEADER = '# Test\n\n- 用途：当前使用说明\n- 适用范围：Tests\n- 状态：有效\n- 最后更新：2026-09-30\n\n'


class DocChecks(unittest.TestCase):
    def setUp(self):
        parent = Path(__file__).resolve().parents[1] / 'Saved' / 'DocCheckTests'
        self.root = parent / uuid.uuid4().hex
        self.root.mkdir(parents=True)
        def cleanup():
            assert self.root.resolve().parent == parent.resolve()
            shutil.rmtree(self.root)
        self.addCleanup(cleanup)
        self.write('Docs/README.md', HEADER + '[Usage](Topic/Usage.md)')
        self.write('Docs/Topic/Usage.md', HEADER)

    def write(self, name, text):
        target = self.root / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(text, encoding='utf-8')

    def rules(self):
        return [rule for _, rule in check(self.root)[0]]

    def test_valid_and_code_examples(self):
        self.write('Docs/Topic/Usage.md', HEADER + '\n```md\n[x](missing.md)\n```\n`[x](missing.md)`')
        self.assertEqual(self.rules(), [])

    def test_missing_metadata_and_orphan(self):
        self.write('Docs/Topic/Orphan.md', '# No metadata')
        self.assertIn('unreachable', self.rules())
        self.assertIn('metadata:用途', self.rules())

    def test_broken_image_and_reference(self):
        self.write('Docs/Topic/Usage.md', HEADER + '![x](missing.png)\n[x][ref]\n[ref]: absent.md')
        self.assertIn('broken-link:missing.png', self.rules())
        self.assertIn('broken-link:absent.md', self.rules())

    def test_case(self):
        self.write('Docs/README.md', HEADER + '[x](Topic/usage.md)')
        self.assertTrue(any(r.startswith(('link-case:', 'broken-link:')) for r in self.rules()))

    def test_naming_root_and_date(self):
        self.write('Docs/New.md', HEADER.replace('2026-09-30', '2026-02-30'))
        for rule in ('naming', 'root-placement', 'metadata:date'):
            self.assertIn(rule, self.rules())

    def test_baseline_is_exact_and_stale_is_error(self):
        self.write('Docs/Topic/Usage.md', '# Missing')
        baseline = {'exceptions': {'Docs/Topic/Usage.md': self.rules()}}
        self.write('Tools/DocCheckBaseline.json', json.dumps(baseline))
        self.assertEqual(self.rules(), [])
        self.write('Docs/Topic/NewFile.md', '# Missing')
        self.assertIn('metadata:用途', self.rules())
        self.write('Docs/Topic/Usage.md', HEADER)
        self.assertTrue(any(r.startswith('stale-exception:') for r in self.rules()))


if __name__ == '__main__':
    unittest.main()

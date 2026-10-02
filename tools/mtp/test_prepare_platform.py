#!/usr/bin/env python3
# SPDX-FileCopyrightText: The uwuAOSP Project
# SPDX-License-Identifier: Apache-2.0
"""Deployment regression tests; uses Git, never a compiler or USB device."""
import contextlib
import importlib.util
import io
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

SPEC = importlib.util.spec_from_file_location('prepare_mtp', Path(__file__).with_name('prepare_platform.py'))
HELPER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(HELPER)


def fixture(patch):
    """Materialize old hunk context only; unrelated lines are inert placeholders.

    This checks the real Git patch application, not native/AOSP compilation.
    """
    files = {}
    path = None
    index = None
    for line in patch.splitlines(True):
        if line.startswith('--- a/'):
            path = line[6:].strip()
            files[path] = {}
        elif line.startswith('@@'):
            index = int(re.match(r'@@ -(\d+)', line)[1]) - 1
        elif index is not None and line[:1] in (' ', '-'):
            previous = files[path].get(index)
            if previous is not None and previous != line[1:]:
                raise AssertionError('Overlapping source context differs')
            files[path][index] = line[1:]
            index += 1
        elif line.startswith('diff --git'):
            index = None
    return {p: ''.join(lines.get(i, f'// unrelated fixture line {i}\n')
                       for i in range(max(lines) + 1)) for p, lines in files.items()}


class Deployment(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name) / 'android'
        self.root.mkdir()
        for repository, patch in HELPER.PATCHES.items():
            target = self.root / repository
            target.mkdir(parents=True)
            subprocess.run(['git', 'init', '-q', str(target)], check=True)
            subprocess.run(['git', '-C', str(target), 'config', 'core.autocrlf', 'false'], check=True)
            for path, data in fixture((HELPER.PAYLOAD / patch).read_text()).items():
                file = target / path
                file.parent.mkdir(parents=True, exist_ok=True)
                file.write_text(data, encoding='utf-8', newline='\n')

    def deploy(self):
        with contextlib.redirect_stdout(io.StringIO()):
            HELPER.apply(self.root, HELPER.plan(self.root))

    def test_preview_apply_and_idempotence(self):
        before = {p: p.read_bytes() for p in self.root.rglob('*') if p.is_file() and '.git' not in p.parts}
        self.assertTrue(all(item[2] for item in HELPER.plan(self.root)))
        self.assertEqual(before, {p: p.read_bytes() for p in before})
        self.deploy()
        self.assertFalse(any(item[2] for item in HELPER.plan(self.root)))
        self.deploy()
        self.assertEqual(1, len(list((self.root.parent / 'android-backups').iterdir())))

    def test_conflict_preflights_every_project_before_mutation(self):
        policy = self.root / 'system/sepolicy/private/recovery.te'
        policy.write_text('unknown policy\n')
        original = (self.root / 'frameworks/av/media/mtp/Android.bp').read_bytes()
        with self.assertRaisesRegex(ValueError, 'conflicts'):
            self.deploy()
        self.assertEqual(original, (self.root / 'frameworks/av/media/mtp/Android.bp').read_bytes())
        self.assertFalse((self.root.parent / 'android-backups').exists())

    def test_modified_owned_block_cannot_be_duplicated(self):
        self.deploy()
        bp = self.root / 'frameworks/av/media/mtp/Android.bp'
        bp.write_text(bp.read_text().replace('"-DRECOVERY_MTP_READONLY"', '"-DRECOVERY_MTP_UNSAFE"'), encoding='utf-8', newline='\n')
        with self.assertRaisesRegex(ValueError, 'Partial/modified'):
            HELPER.plan(self.root)

    def test_unrelated_source_changes_survive(self):
        bp = self.root / 'frameworks/av/media/mtp/Android.bp'
        bp.write_text('// downstream customization\n' + bp.read_text(), encoding='utf-8', newline='\n')
        self.deploy()
        self.assertTrue(bp.read_text().startswith('// downstream customization\n'))
        self.assertFalse(any(item[2] for item in HELPER.plan(self.root)))

    def test_paths_cannot_escape_tree(self):
        with self.assertRaises(ValueError):
            HELPER.trusted_path(self.root, '../outside')


if __name__ == '__main__':
    unittest.main()

#!/usr/bin/env python3
"""Host deployment tests only; these do not compile or validate Android decryption."""
import hashlib
import json
from pathlib import Path
import tempfile
import unittest
import prepare_android17 as prepare


class PreparationTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.review = self.root / 'review.json'
        checked = self.root / 'system/vold/FsCrypt.cpp'
        checked.parent.mkdir(parents=True)
        checked.write_bytes(b'// synthetic reviewed source\n')
        self.review.write_text(json.dumps({'sources': {'system/vold/FsCrypt.cpp': hashlib.sha256(checked.read_bytes()).hexdigest()}}))
        for path, name in prepare.INTERFACES.items():
            file = self.root / f'hardware/interfaces/{path}/aidl/Android.bp'
            file.parent.mkdir(parents=True)
            file.write_text(f'aidl_interface {{\n    name: "{name}",\n    backend: {{ ndk: {{ enabled: true, }}, }},\n}}\n')
        self.sqlite = self.root / 'external/sqlite/dist/Android.bp'
        self.sqlite.parent.mkdir(parents=True)
        self.sqlite.write_text('cc_defaults { name: "sqlite-minimal-defaults", }\ncc_defaults { name: "release_package_libsqlite3_library_defaults", }\n')

    def tearDown(self):
        self.temp.cleanup()

    def test_preview_is_read_only_and_only_dependency_projects_change(self):
        originals = {p: p.read_bytes() for p in self.root.rglob('*') if p.is_file()}
        changes = prepare.plan(self.root, self.review)
        self.assertEqual(len(changes), 6)
        for path in changes:
            self.assertTrue(path.startswith(('hardware/interfaces/', 'external/sqlite/')))
        self.assertEqual(originals, {p: p.read_bytes() for p in originals})

    def test_applied_plan_is_idempotent(self):
        for path, (_, after) in prepare.plan(self.root, self.review).items():
            (self.root / path).write_text(after)
        self.assertEqual(prepare.plan(self.root, self.review), {})

    def test_unknown_platform_is_refused_before_any_changes(self):
        (self.root / 'system/vold/FsCrypt.cpp').write_text('changed')
        before = self.sqlite.read_bytes()
        with self.assertRaisesRegex(ValueError, 'needs review'):
            prepare.plan(self.root, self.review)
        self.assertEqual(self.sqlite.read_bytes(), before)

    def test_existing_disabled_variant_is_not_overridden(self):
        with self.assertRaises(ValueError):
            prepare.add_recovery_variant('aidl_interface { name: "example", recovery_available: false, }', 'example')

    def test_duplicate_interface_is_refused(self):
        block = 'aidl_interface { name: "example", }\n'
        with self.assertRaises(ValueError):
            prepare.add_recovery_variant(block * 2, 'example')

    def test_changed_sqlite_adapter_is_refused(self):
        self.sqlite.write_text('cc_library_static { name: "librecovery_crypto_sqlite", }')
        with self.assertRaisesRegex(ValueError, 'differs'):
            prepare.plan(self.root, self.review)


if __name__ == '__main__':
    unittest.main()

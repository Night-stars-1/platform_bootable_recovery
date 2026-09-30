#!/usr/bin/env python3
# SPDX-FileCopyrightText: The uwuAOSP Project
# SPDX-License-Identifier: Apache-2.0
"""Prepare reviewed Android 17 Recovery dependencies; never builds or touches a phone."""
import argparse
import difflib
import hashlib
import json
from pathlib import Path
import re
import time

ROOT = Path(__file__).resolve().parents[2]
INTERFACES = {
    'security/keymint': 'android.hardware.security.keymint',
    'security/secureclock': 'android.hardware.security.secureclock',
    'security/sharedsecret': 'android.hardware.security.sharedsecret',
    'gatekeeper': 'android.hardware.gatekeeper',
    'weaver': 'android.hardware.weaver',
}
SQLITE_BLOCK = '''\n// BEGIN recovery-crypto Android 17 SQLite (unlock-only backend)
cc_library_static {
    name: "librecovery_crypto_sqlite",
    defaults: ["sqlite-minimal-defaults", "release_package_libsqlite3_library_defaults"],
    recovery_available: true,
    host_supported: true,
    cflags: ["-DSQLITE_OMIT_LOAD_EXTENSION", "-DSQLITE_TEMP_STORE=3"],
    visibility: ["//visibility:public"],
}
// END recovery-crypto Android 17 SQLite
'''


def add_recovery_variant(source, name):
    pattern = r'(aidl_interface\s*\{\s*name:\s*"' + re.escape(name) + r'",)'
    matches = list(re.finditer(pattern, source))
    if len(matches) != 1:
        raise ValueError(f'Unknown interface layout: {name}')
    start = matches[0].end()
    # Interface may contain nested backend/version blocks; find the entire balanced block.
    begin = source.rfind('{', 0, start)
    depth, end, quoted, escaped, comment = 1, begin + 1, False, False, False
    while end < len(source) and depth:
        ch = source[end]
        if comment:
            if ch == '\n':
                comment = False
        elif quoted:
            if escaped:
                escaped = False
            elif ch == '\\':
                escaped = True
            elif ch == '"':
                quoted = False
        elif source[end:end+2] == '//':
            comment = True
        elif ch == '"':
            quoted = True
        elif ch == '{':
            depth += 1
        elif ch == '}':
            depth -= 1
        end += 1
    if depth:
        raise ValueError('Unbalanced interface')
    body = source[begin:end]
    existing = re.findall(r'\brecovery_available:\s*(true|false)', body)
    if existing:
        if existing != ['true']:
            raise ValueError(f'Refusing to override recovery_available in {name}')
        return source
    return source[:start] + '\n    recovery_available: true,' + source[start:]


def plan(source_root, review_path=None):
    review_path = review_path or ROOT / 'crypto/android17/source-review.json'
    review = json.loads(review_path.read_text(encoding='utf-8'))
    for path, expected in review['sources'].items():
        source = source_root / path
        if not source.is_file() or hashlib.sha256(source.read_bytes()).hexdigest() != expected:
            raise ValueError(f'Platform source needs review: {path}')
    changes = {}
    for path, name in INTERFACES.items():
        relative = f'hardware/interfaces/{path}/aidl/Android.bp'
        before = (source_root / relative).read_text(encoding='utf-8')
        after = add_recovery_variant(before, name)
        if after != before:
            changes[relative] = (before, after)
    relative = 'external/sqlite/dist/Android.bp'
    before = (source_root / relative).read_text(encoding='utf-8')
    if 'name: "librecovery_crypto_sqlite"' in before:
        if SQLITE_BLOCK not in before:
            raise ValueError('Existing SQLite adapter differs from the reviewed block')
    else:
        if not all(token in before for token in ('name: "sqlite-minimal-defaults"', 'name: "release_package_libsqlite3_library_defaults"')):
            raise ValueError('Unknown SQLite defaults')
        changes[relative] = (before, before + SQLITE_BLOCK)
    return changes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source_root', type=Path)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--apply', action='store_true')
    mode.add_argument('--check', action='store_true')
    args = parser.parse_args()
    source = args.source_root.resolve()
    changes = plan(source)
    if args.check:
        if changes:
            raise ValueError('Recovery variants not prepared: ' + ', '.join(changes))
        print('Reviewed platform formats and Recovery dependency declarations verified. No compilation or device validation.')
        return
    if not args.apply:
        for path, (before, after) in changes.items():
            print(''.join(difflib.unified_diff(before.splitlines(True), after.splitlines(True), fromfile=path, tofile=path)), end='')
        print('Preview only. No files changed, no backend enabled, no build ran.')
        return
    if changes:
        for path, (before, _) in changes.items():
            if (source / path).read_text(encoding='utf-8') != before:
                raise ValueError('Source changed during preflight: ' + path)
        backup = source.parent / (source.name + '-backups') / f'recovery-crypto-deps-{time.time_ns()}'
        for path in changes:
            target = backup / path
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes((source / path).read_bytes())
        # All preflight checks and backups finish before the first source mutation.
        for path, (_, after) in changes.items():
            target = source / path
            temp = target.with_name(target.name + '.recovery-crypto.tmp')
            temp.write_text(after, encoding='utf-8', newline='\n')
            temp.replace(target)
        print('Backup: ' + str(backup))
    print('Dependencies prepared. Backend remains opt-in; no compilation or phone operation ran.')


if __name__ == '__main__':
    try:
        main()
    except (ValueError, OSError, KeyError, json.JSONDecodeError) as error:
        raise SystemExit('ERROR: ' + str(error))

#!/usr/bin/env python3
# SPDX-FileCopyrightText: The uwuAOSP Project
# SPDX-License-Identifier: Apache-2.0
"""Preview/apply reviewed Recovery MTP transport patches. Never builds/flashes."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import time

PAYLOAD = Path(__file__).resolve().parents[2] / 'mtp/platform'
PATCHES = {'frameworks/av': 'frameworks-av.patch', 'system/sepolicy': 'sepolicy.patch'}


def trusted_path(root, relative):
    if Path(relative).is_absolute() or '..' in Path(relative).parts:
        raise ValueError('Invalid managed path: ' + relative)
    target = root / relative
    for part in (target, *target.parents):
        if part == root:
            break
        if part.is_symlink():
            raise ValueError('Symlink in managed path: ' + relative)
    if not target.resolve().is_relative_to(root.resolve()):
        raise ValueError('Path outside source tree: ' + relative)
    return target


def git(repository, *args):
    return subprocess.run(['git', '-C', str(repository), *args],
                          capture_output=True, text=True, timeout=30)


def plan(root):
    review = json.loads((PAYLOAD / 'source-review.json').read_text(encoding='utf-8'))
    result = []
    for relative, filename in PATCHES.items():
        repository = trusted_path(root, relative)
        if git(repository, 'rev-parse', '--show-toplevel').returncode:
            raise ValueError('Missing Git checkout: ' + relative)
        patch = PAYLOAD / filename
        # Validate every managed file, including already-applied deployments.
        for file in review[relative]['sha256']:
            path = trusted_path(root, relative + '/' + file)
            if not path.is_file():
                raise ValueError('Missing reviewed source: ' + str(path))
        if git(repository, 'apply', '--reverse', '--check', str(patch)).returncode == 0:
            result.append((relative, patch, False))
            continue
        if any(b'RECOVERY_MTP' in trusted_path(root, relative + '/' + file).read_bytes()
               for file in review[relative]['sha256']):
            raise ValueError('Partial/modified Recovery MTP deployment: ' + relative)
        check = git(repository, 'apply', '--check', str(patch))
        if check.returncode:
            raise ValueError('Recovery MTP patch conflicts in ' + relative + ': ' + check.stderr.strip())
        result.append((relative, patch, True))
    return result


def apply(root, changes):
    pending = [item for item in changes if item[2]]
    if not pending:
        return
    review = json.loads((PAYLOAD / 'source-review.json').read_text(encoding='utf-8'))
    backup = root.parent / (root.name + '-backups') / ('recovery-mtp-' + str(time.time_ns()))
    originals = {}
    for relative, _, _ in pending:
        for file in review[relative]['sha256']:
            key = relative + '/' + file
            originals[key] = trusted_path(root, key).read_bytes()
            target = backup / key
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(originals[key])
    (backup / 'source-hashes.json').write_text(json.dumps({
        p: hashlib.sha256(data).hexdigest() for p, data in originals.items()
    }, indent=2) + '\n', encoding='utf-8')
    # Recheck every repository before the first mutation; preserve all unrelated edits.
    for relative, patch, _ in pending:
        if git(root / relative, 'apply', '--check', str(patch)).returncode:
            raise ValueError('Source changed during preflight: ' + relative)
    for relative, patch, _ in pending:
        operation = git(root / relative, 'apply', str(patch))
        if operation.returncode:
            raise ValueError('Patch failed; originals preserved at ' + str(backup) + ': ' + operation.stderr)
    print('Backup: ' + str(backup))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source_root', type=Path)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--apply', action='store_true')
    mode.add_argument('--check', action='store_true')
    args = parser.parse_args()
    root = args.source_root.resolve()
    changes = plan(root)
    if args.check:
        pending = [relative for relative, _, needed in changes if needed]
        if pending:
            raise ValueError('Recovery MTP platform changes not installed: ' + ', '.join(pending))
        print('Verified read-only Recovery transport and USB policy inputs. No build/device validation.')
    elif args.apply:
        apply(root, changes)
        plan(root)
        print('Recovery MTP platform inputs prepared. Device opt-in still required. No build or phone operation ran.')
    else:
        for relative, patch, needed in changes:
            if needed:
                print('Repository: ' + relative)
                print(patch.read_text(encoding='utf-8'), end='')
        print('Preview only. No files changed; MTP remains opt-in.')


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, KeyError, subprocess.TimeoutExpired) as error:
        raise SystemExit('ERROR: ' + str(error))

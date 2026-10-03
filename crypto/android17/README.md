# Android 17 existing-key recovery backend

English | [简体中文](README.zh-CN.md)

This directory now contains an implementation, not the unsupported callback
skeleton in `../examples`. It implements the existing-key unlock chain for the
reviewed Android 17 platform formats. **It has not been compiled, linked, run on
Android, or used to decrypt a phone.** Do not advertise it as device-tested.
The maintainer has requested that only they perform builds.

The independent Python deployment tests and synthetic fixture generation do not
prove that native code or vendor security services work. The native test sources
are provided for the maintainer to compile/run.

## Implemented chain

1. Read a root-owned, non-writable ramdisk profile; connect to the selected AIDL
   KeyMint device. Validate its security level, negotiate the configured
   SharedSecret participants and connect to AIDL or HIDL Gatekeeper/Weaver.
   AIDL uses non-lazy `checkService`. The reviewed Recovery libhidl resolves
   HIDL through local passthrough implementations, not hwservicemanager.
   No KeyMint generation, enrollment or deletion runs.
   A concurrently running keystore2 daemon is refused to avoid replacing another
   negotiator's agreement. Device init/manifest must provide only reviewed HALs.
2. Read the reviewed crypto fstab, mount metadata if necessary and retrieve its
   **existing** vold v1 key directory. Decrypt through KeyMint with the original
   secdiscardable application ID. Convert wrapped keys for this boot and load a
   read-only `dm-default-key` mapping. Mount userdata read-only without fsck,
   journal replay, F2FS roll-forward or formatting.
3. Restore System DE from `/data/unencrypted/key`, then the selected user's DE
   key. Install them with `FS_IOC_ADD_ENCRYPTION_KEY`; compare their kernel key
   identifiers/descriptors to actual directory policies and verify key presence.
4. Read the current `sp-handle` from a private locksettings database snapshot,
   parse PasswordData/protector state and determine the real credential type.
   PIN/password bytes and Android pattern cell encoding are supported; current
   empty-LSKF protectors use the padded `default-password` path, old protectors
   retain their stored scrypt parameters. A missing `.pwd` alone is not accepted
   as proof of an empty LSKF; the explicit stored password quality must also be 0.
5. Verify the LSKF through Gatekeeper (including its fake user ID offset) or the
   protector's Weaver slot. Respect hardware errors/throttling. Do not re-enroll.
   Use the returned Gatekeeper auth token directly in KeyMint operations and use
   SecureClock when required. Read the existing locksettings protector key from
   the reviewed keystore2 database and unwrap SP in the correct v1/v2/v3 order.
   No keystore2 key import, namespace migration or database writes.
6. Preserve SP's ASCII-hex representation, derive the v1/v2 SHA-512 or v3
   SP800-108 HMAC-SHA256 `fbe-key`, and recover the existing CE key. Rotated `cx*`
   candidates are tried without renaming/fixating/deleting any key directory.
   Compare the installed key to `/data/media/<user>`'s policy. The separate
   Recovery framework still verifies mounted storage/key presence/readability
   before opening the ZIP browser.

SQLite databases and their WALs are read through non-symlink file descriptors.
Checksummed, committed WAL frames are applied to a private in-memory image;
uncommitted/incomplete frames are not published. SQLite uses `:memory:` and
read-only deserialization, never the original database or SHM. A nonempty hot
rollback journal fails the attempt instead of running database recovery. Userdata
must be read-only before these snapshots are read.

The backend contains no destructive fallback. An existing pending upgraded key,
`KEY_REQUIRES_UPGRADE`, missing key, malformed protector, unavailable service or
unsupported format returns an ABI error and preserves stored keys/credentials.
Successful or partial key installation remains in the kernel until reboot; the
worker releases its own secrets/handles without evicting those keys. A hung or
crashed worker is still bounded by the parent framework's deadline.

The private device-mapper table-load and activation requests use libdm's stable
`4.0.0` request ABI, rather than the newer version of the build's Linux headers.
This follows `DeviceMapper::InitIo` and works with older supported kernel minors.
The read-only mapping and secure, wiped ioctl buffer remain unchanged; neither
the table nor its key is sent through libdm's table-debug logging path.

## Opt-in device adaptation

The backend is **disabled by default at the Soong module level**. Its HAL/SQLite
dependencies are conditional too. Syncing this directory does not enable it or
add vendor services to unrelated Recovery products.

Keep the device adaptation outside `bootable/recovery`:

```text
device/<vendor>/<device>/recovery-crypto/
  Android.bp
  recovery.crypto.conf
  recovery.crypto.fstab
  init*.rc / VINTF / sepolicy and reviewed vendor dependency packaging
```

Use `device.example.bp` and `device.example.mk` as packaging templates. Replace
the example module names with device-specific names. Install exactly one backend
with stem `librecovery_crypto_backend`. The profile is installed as
`/system/etc/recovery.crypto.conf`, the crypto fstab as
`/system/etc/recovery.crypto.fstab` in the Recovery ramdisk.
Custom crypto fstab names must also stay under `/system/etc/`; profiles cannot
select a userdata/metadata-resident fstab.

Use the **normal system's real encryption options**, not an old Recovery fstab
with `fileencryption=ice,wrappedkey`. This separate fstab is used only by the
optional backend and does not replace Recovery's regular partition table. For
example, a device's real userdata options might include:

```text
fileencryption=aes-256-xts:aes-256-cts:v2+inlinecrypt_optimized+wrappedkey_v0
keydirectory=/metadata/vold/metadata_encryption
metadata_encryption=aes-256-xts:wrappedkey_v0
```

These are format examples, not device block paths. Copy only the device's actual
`/metadata` and `/data` entries; preserve its real filesystem/mount options.
Configure the exact KeyMint, Gatekeeper, Weaver, SecureClock and **all** relevant
SharedSecret participants. `sharedsecret_services` lists AIDL services;
optional `sharedsecret_hidl_instances=4.1/default,4.1/strongbox` selects the
highest advertised HIDL Keymaster version per security level. Do not configure
both 4.0 and 4.1 for the same instance. All configured participants receive the
same sorted parameter list and must return the same checksum. Devices without
that option keep the AIDL-only path. Set transport to `none` with an empty instance
only when that service is genuinely not needed by any protector on that device.
Gatekeeper can still be needed for the SP SID after Weaver verification.

The profile cannot run commands, pick arbitrary backend libraries, disable
authentication or choose a key blob from user input. It issues no arbitrary init
commands; HIDL requires the reviewed local passthrough implementation in Recovery.
Vendor HAL binaries/libraries, firmware availability, Binder
drivers, compatible OS/security-patch properties, VINTF and narrowly scoped
SELinux permissions must be supplied and reviewed by the device maintainer.
Keep SELinux enforcing. There is no universal vendor-security service bundle.
If a passthrough HAL requires vendor-internal properties, keep it in an appropriate
vendor HAL service domain and provide a reviewed device-owned AIDL protocol bridge.
Do not grant coredomain Recovery access that violates vendor property isolation.
Recovery's servicemanager uses VintfObjectRecovery, which merges fragments under
`/system/etc/vintf/manifest/`; add a unique Recovery fragment rather than replacing
existing health/fastboot declarations or the normal Android vendor manifest.

## Recovery dependency preparation

Some AIDL projects, their analyzer runtime and SQLite do not provide Recovery variants in this platform.
The helper adds only their build declarations, leaving normal-system code intact.
It verifies the reviewed source hashes before making any change, backs up original
files outside the source tree and refuses conflicting or unknown declarations.

Run from the Android source root; each command is separate:

```bash
python3 bootable/recovery/tools/crypto/prepare_android17.py /path/to/android
python3 bootable/recovery/tools/crypto/prepare_android17.py /path/to/android --apply
python3 bootable/recovery/tools/crypto/prepare_android17.py /path/to/android --check
```

None of these commands compiles, enables a device profile, accesses a phone, or
reads real user keys. No source hash manifest is required. When updating Android,
maintainers must check that the SP, vold and keystore2 formats still match the
backend. Dependency declarations and policy patch conflicts are checked separately.

The helper also adds `recovery_available: true` to the reviewed
`aidl-analyzer-main` static library in `system/tools/aidl/Android.bp`: AIDL
propagates interface Recovery availability to generated C++ analyzers, whose
static dependency needs the same image variant. This does not select an analyzer
for installation in the Recovery image or change normal-system code. A changed
analyzer runtime declaration is refused for review before any writes.

The dependency changes belong to `hardware/interfaces`, `system/tools/aidl`
and `external/sqlite`.
Commit/manifest-track them in a release fork, or reapply the helper after syncing
those projects. The device adaptation belongs to the device/vendor projects.
`repo sync -c bootable/recovery` updates the generic backend and helper without
editing either the device adaptation or those other projects. It does not itself
run this helper or sync the other projects.

The helper also checks/applies a complete explicitly reviewed patch from
`tools/crypto/patches/` to `system/sepolicy`. The AOSP variant is
`android17-recovery-key-access.patch`; the uwuAOSP variant is
`android17-uwu-recovery-key-access.patch`, reviewed against revision
`b41cbf3b46882139654574fe46ca5cf8175bf8a5`. The latter preserves uwuAOSP's
existing `apexd` metadata exceptions and all unrelated platform rules.
It uses Git against isolated copies to verify either the original or fully
patched state, preserves compatible unrelated edits, and refuses conflicts or
partial application. No regular expression generates or rewrites policy rules.
Review inputs and patch hashes are in the sibling JSON manifests. Exactly one
complete variant must match; the helper never combines hunks from different
variants. An unknown change to a key-isolation rule still requires review.

This patch alone grants no access. An adapted device must explicitly set the
following in its **BoardConfig**, in the same conditional as its crypto policy:

```make
BOARD_SEPOLICY_M4DEFS += recovery_crypto_android17=true
```

The exception expands only when both this flag and `target_recovery` are true.
Normal Android and non-opt-in Recovery retain the original key-isolation
semantics. Opt-in Recovery can read existing regular-file keys/databases and
add/query kernel fscrypt keys. Writes, execution, non-regular key-file access,
setting encryption policies and removing encryption keys remain forbidden.
Read-only access trusts Recovery code with encrypted key material; the patch
does not isolate the UI and worker into separate SELinux domains.

Keep this explicit patch in a platform SELinux fork for a maintained release,
or reapply after syncing `system/sepolicy`; device-specific permissions and the
flag stay in the device tree. Syncing `bootable/recovery` does not change either
project. The helper never compiles or flashes anything.

## Deliberate limits

- Requires an actual AIDL KeyMint implementation. HIDL-only Keymaster devices
  need a separate compatibility adapter; this code does not start keystore2's
  compatibility daemon or guess a bridge.
- HIDL Gatekeeper 1.0, Weaver 1.0 and Keymaster 4.0/4.1 SharedSecret negotiation
  are supported alongside AIDL. HIDL HALs need Recovery passthrough libraries.
  A vendor offering only a service/factory needs a reviewed device-tree bridge;
  a manifest alone does not supply a passthrough implementation.
- Uses the reviewed keystore2 live client-key schema with `blobentry.state`,
  SELinux domain 2, locksettings namespace 103 and known UUID encodings. Legacy
  APP-namespace keys, super-encrypted or boot-level-bound blobs and unknown blob
  metadata are refused. No migration or key upgrade runs in Recovery.
- Supports raw, KeyMint `wrappedkey_v0` and upstream block-crypto `wrappedkey`
  runtime conversion. Upstream wrapped keys require the real kernel ioctl.
  Legacy dm-default-key option format 1, `ice` fstab aliases, multi-device userdata
  and logical userdata require a separate adapter.
- Storage binding seed profiles are currently refused (`storage_binding=none`
  requires the maintainer to verify that the normal platform does not use one).
  Do not insert a guessed/public seed or bypass a seed check.
- Internal users only; UI currently exposes user 0. No work-profile challenge,
  adoptable storage, escrow-token unlock, grids outside 3x3..6x6 or non-ASCII
  on-screen password keyboard is provided by this change.
- Read-only mounting deliberately refuses filesystems needing recovery. It does
  not make a damaged/unclean filesystem readable by writing to it.
- Framework credential/IPC buffers are locked and excluded from dumps. Derived
  byte buffers and owned Binder copies are wiped; compiler/HAL/Binder/OpenSSL
  internal allocations are not a guarantee of complete memory locking. Worker
  core dumps are disabled. This is not certification against physical memory
  acquisition or buggy vendor logging.

## Validation to perform before shipping

### Locating an unlock failure

The Android 17 backend appends typed diagnostic checkpoints to the existing
`/tmp/recovery.log`. This works without logd/logcat and leaves worker stdout and
stderr redirected to `/dev/null`. Only a fixed checkpoint name, public result
code and numeric errno/Binder/HAL code are emitted. Credentials, pattern cells,
user IDs, file paths, key material, tokens and vendor error strings are excluded.
The logger refuses symlinks, non-regular/non-root-owned files and logs at least
4 MiB; it never creates a log. Logging failure preserves the operation result
and errno. These diagnostic changes still need Android compilation and device
validation by the maintainer.

After building/flashing a Recovery containing the diagnostics, make one unlock
attempt on the phone, then collect the log **before rebooting Recovery**:

```bash
adb -d pull /tmp/recovery.log recovery-decrypt.log
```

Each unlock session starts with the coarse `services`, `metadata`, `de_keys` and
`credential_type` results. CE results include the substeps below, followed by
`sp_unlock` and, if that succeeds, `ce_load`. A later summary failure may repeat
the result; use its preceding substep failures to find the cause. CE rotation
can examine more than one stored key candidate without retrying authentication,
so an earlier candidate failure does not imply final failure if `ce_load` succeeds.

| Checkpoint | Operation to investigate |
| --- | --- |
| `protector_key` | Reading the current SP protector key/security level from keystore |
| `credential_format`, `stretch` | Input encoding and stored scrypt parameters |
| `gatekeeper_input`, `gatekeeper_verify`, `gatekeeper_token`, `weaver_read` | Pre-call bounds, hardware verification, throttling and token format |
| `keymint_begin`, `keymint_finish`, `secureclock` | KeyMint key use, authenticated decrypt and timestamp generation |
| `sp_discardable`, `sp_software_decrypt`, `sp_format`, `sp_handle`, `sp_derive` | SP state, unwrap, main-user verification and FBE subkey derivation |
| `ce_key_directories`, `stored_key_read`, `stored_key_decrypt` | Existing CE key candidates and their software wrapping |
| `storage_export` | Hardware-wrapped storage key conversion |
| `fscrypt_policy`, `fscrypt_descriptor`, `fscrypt_identifier` | Kernel policy/key matching |
| `fscrypt_add_key`, `fscrypt_status` | Kernel key installation and presence |

`result=0` means success; 1 unsupported, 2 unavailable service, 3 missing existing
key/state, 4 required key upgrade, 5 credential rejected, 6 hardware throttled,
7 other I/O/format/cryptographic failure. `source=hal`, `binder` and `errno` identify
the numeric code's namespace; `source=none code=0` means there is no raw code,
**not** that the operation succeeded. For example, `keymint_begin result=7
source=hal code=...` locates a KeyMint rejection; it is not by itself evidence of
a wrong pattern. No diagnostic automatically retries, enrolls, upgrades or
rewrites keys.

Gatekeeper's existing handle may have legacy version 0. The reviewed
[`GateKeeper::Verify`](https://android.googlesource.com/platform/system/gatekeeper/+/5b5e75b5bda9fccbc3132e9624cb25286babdaac/gatekeeper.cpp)
rejects versions above its supported maximum, not version 0. The backend passes
that legacy handle to the configured HAL without reenrollment, retains its
size/upper-version bounds and validates the resulting hardware auth token and
SID as before. A valid input shape is not proof of successful authentication.

The pure formatter fixtures in `tests/native_test.cpp` check code/namespace
formatting, numeric bounds and invalid inputs. They do not write a log or call a
HAL. Passing source checks does not validate Recovery logging or decryption.

### Maintainer checks

The host deployment tests can be run without compiling:

```bash
python3 bootable/recovery/tools/crypto/test_prepare_android17.py
python3 bootable/recovery/tools/crypto/test_policy_patch.py
```

The policy tests apply the patch only to temporary public fixtures and use m4
to verify all four opt-in/Recovery combinations, unchanged normal isolation and
retained removal/write guards. They do not compile a SELinux binary policy.

The maintainer should compile/run `recovery_crypto_android17_test` in the normal
Android native-test environment, which tests
SP versioned KDFs, stored scrypt parameters, authenticated AES-GCM, truncated
PasswordData and committed/checksummed WAL handling using synthetic fixtures.
No native test calls a HAL or reads a real credential/key directory.
The test is deliberately not a Recovery image module: platform gtest has no
Recovery variants. It uses the same backend sources with normal Android
dependencies; the production backend remains `recovery: true`. These fixture
tests do not validate Recovery linking, HAL operation or device decryption.

ScreenRecoveryUI asks for patterns on a touchable 3-by-3 through 6-by-6 grid, with
selected dots, connecting strokes and explicit Unlock/Clear/Cancel controls.
Starting a new stroke on a dot clears the previous drawing; a pattern needs
at least four dots and finger-up before explicit submission. Crossing an
unvisited cells on rows, columns and 45-degree diagonals follows the reviewed
uwuAOSP LockPatternView rule. Volume keys navigate the same grid
and controls; Power selects the highlighted target when touch is unavailable.
Custom/stub UIs can decline the optional pattern API without affecting sideload.
On ordinary interactive Recovery entry, an installed backend now starts the
metadata/DE preparation and opens this credential UI before the home menu.
Cancel or failure returns to the home menu. Selecting an internal-storage ZIP
later reuses verified unlocked storage or lets the user try unlocking again.
Command-driven OTA/sideload, wipe, rescue, just-exit and headless boots do not
wait for credential input. Startup behavior still needs maintainer build and
device verification; this change does not itself fix a backend unlock failure.
The UI views the caller's locked credential memory and neither logs nor copies
the pattern into a text/menu string. Cancelling clears it and leaving the page
redraws both framebuffer pages. Ordinary menus retain their swipe scrolling.
Synthetic grid/midpoint/sparse-motion/layout fixtures are included in
`tests/unit/screen_ui_test.cpp`; no test attempts real credential verification.

The backend reads `lock_pattern_size` with the read-only locksettings snapshot
helper, after metadata and DE restoration.
An absent setting defaults to 3, as on standard AOSP installs. A failed database
read, duplicate/malformed value or unsupported size stops before credential
submission; there is no manual override or automatic trial of other sizes.
Each cell encodes as one byte `row * gridSize + column + '1'`, including indexes
above 8. The optional `recovery_crypto_get_pattern_size_v1` export leaves the
required v1 backend ABI structure intact. A custom backend lacking the export
retains its old 3x3 contract. The private worker protocol is v2, so Recovery and
its worker must be rebuilt together.

Grid-size, per-user/default/error and encoding fixtures are in
`crypto/android17/tests/native_test.cpp`; protocol bounds/duplicate fixtures are
in `crypto/tests/protocol_test.cpp`. These new native tests have not been run;
source parsing and layout previews do not verify authentication on a device.

For each enabled device, test empty LSKF, PIN, password, pattern, Weaver and
Gatekeeper paths as applicable, wrong credentials, hardware throttle, missing
services/keys, unsupported/upgrade-required blobs, failure followed by ordinary
ADB sideload, and returning to Android with unchanged credentials. Verify
fscrypt key/policy matches, file names and file contents after successful unlock.
Compilation alone is not evidence of decryption; source-only checks are less.

Primary design references:

- https://source.android.com/docs/security/features/encryption/file-based
- https://source.android.com/docs/security/features/encryption/metadata
- https://source.android.com/docs/security/features/encryption/hw-wrapped-keys

## Optional writable media export

A Recovery MTP `VID:PID:rw` profile changes the mount lifecycle; see
[the MTP guide](../../mtp/README.md). The default without this trusted profile
remains read-only. With the opt-in, userdata journal/roll-forward recovery is
allowed at its initial ro mount; only after CE key recovery succeeds is an rw
remount attempted. Keys and lock settings remain protected by their read-only
access rules. This path requires device filesystem and SELinux validation.

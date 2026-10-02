# Recovery MTP (optional, read-only)

Shares verified, unlocked user-0 media with a desktop MTP client, including
Windows File Explorer. Keeps ADB in the same USB configuration. It does not
enable MTP for an unadapted device, mount or format userdata, install keys,
change encryption, or grant the host write access.

The Android 17 crypto backend currently mounts userdata read-only. This feature
supports browsing and downloading files. Upload, delete, rename, move and edit
requests are rejected at both the database and the AOSP dispatch boundary;
the storage advertises read-only access. A writable MTP implementation requires
a separately reviewed writable crypto/mount lifecycle, not a UI switch.

## Device adaptation

1. Supply a working crypto adapter that installs CE keys for user 0. The daemon
   independently calls `VerifyUserStorage(0)`; a previous success property or
   visible directory alone is insufficient. Without keys, MTP stays disabled.
2. Prepare the owning platform projects from the synced Recovery fork:

   ```bash
   python3 bootable/recovery/tools/mtp/prepare_platform.py /path/to/android
   python3 bootable/recovery/tools/mtp/prepare_platform.py /path/to/android --apply
   python3 bootable/recovery/tools/mtp/prepare_platform.py /path/to/android --check
   ```

   The default command previews. Explicit reviewed Git patches add a private,
   opt-in transport in `frameworks/av/media/mtp`, preserving normal `libmtp`
   behavior. No AOSP transport source is copied into the Recovery fork.
   Narrow Recovery policy additions permit the existing USB configuration
   properties, stopping this service and the specific FunctionFS ioctls.
   No neverallow, key-isolation rule or userdata write restriction is relaxed.
   Conflicts and modified/partial deployments are refused; backups live outside
   the source tree. Reapply/check after syncing the owning platform projects.
3. In the device product:

   ```make
   SOONG_CONFIG_NAMESPACES += recovery_mtp
   SOONG_CONFIG_recovery_mtp += enabled
   SOONG_CONFIG_recovery_mtp_enabled := true
   PRODUCT_PACKAGES += recovery_mtp your_device_recovery_mtp_config
   ```

   Install a root-owned, non-writable regular file at
   `/system/etc/recovery.mtp.conf` via a Recovery-only `prebuilt_etc` module.
   Its entire content is `VID:PID` with four hexadecimal digits each, optionally
   followed by a single LF. Example for an appropriate Google VID device:
   `18D1:4EE2`. The VID must match `ro.recovery.usb.vid`; select a valid, distinct
   PID for the MTP+ADB interface composition for your device/vendor.
4. Verify configfs (`sys.usb.configfs=1`), FunctionFS, the USB controller property
   and working Recovery ADB. This implementation does not replace a device's
   legacy `android_usb` setup. It uses gadget `g1`, configuration `b.1` and
   `ffs.mtp` + `ffs.adb`, matching the existing Recovery layout. Devices with
   different gadget layouts need their own init adapter. Only one configuration
   is exposed; the MTP interface comes first for Windows OS descriptors.
5. Keep all profile files/product includes in the device tree. Neither the
   deployment helper nor the device adapter edits `bootable/recovery`, so
   `repo sync -c bootable/recovery` can update the generic implementation.

## Lifecycle and limits

- The home menu starts MTP only after actual storage verification and only when
  ADB was already enabled. It does not override sideload, rescue or fastboot.
- Menu navigation keeps MTP. Foreground actions stop the daemon and release
  media descriptors before installer/wipe/fastboot/unmount; returning to the
  menu restarts it after verification. All property waits are bounded.
- Descriptor/daemon/config failures restore ADB and do not block unlocking or
  normal Recovery use. A failed start is not retried on every menu tap.
- The index includes only directories and regular files on the media root's
  filesystem. Symlinks, special nodes, path traversal and replaced inodes are
  refused; opened files are pinned with read-only descriptors for transfer.
- The index is a snapshot for one connection, bounded to 100,000 entries and
  64 directory levels. Files larger than 4 GiB retain their 64-bit sizes.
  Only user 0 is exposed. No /data keys, metadata or other user roots are exported.

## Validation

`python3 bootable/recovery/tools/mtp/test_prepare_platform.py` exercises actual
Git patch application, idempotence, conflicts, partial deployments, preservation
of unrelated changes and path containment. It does not run a compiler.

`recovery_mtp_database_test` is a normal Android native test (not a Recovery/gtest
variant). Its source covers nested/Unicode enumeration, symlink/special-node
exclusion, replaced inodes, pinned transfers, 64-bit sizes, bounds and mutation
refusal. Maintainers must build/run it themselves.

Android compilation, SELinux policy compilation, USB enumeration, disconnect/
reconnect, sideload handover, Windows downloads and device decryption have not
been validated for this change. Source parsing and Python deployment tests are
not proof of those runtime properties. Check `sys.usb.config`, `sys.usb.state`,
`sys.usb.ffs.mtp.ready`, `init.svc.recovery-mtp`, Recovery logs and SELinux denials
on a real device without collecting lock credentials or keys.

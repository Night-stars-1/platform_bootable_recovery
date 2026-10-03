# Android 17 Recovery 解密技术参考

[返回设备适配指南](README.zh-CN.md) | [English](README.md) | [通用解密框架](../README.zh-CN.md)

本目录提供 Android 17 **已有密钥恢复**的后端实现，包含 metadata、DE、Synthetic
Password 和 CE 解锁链路。它不是 `../examples` 中只返回“不支持”的接口示例。
后端默认关闭，只有设备显式适配后才会启用。

**源码实现、部署检查和编译成功，都不能代替实际设备解密验证。** 发布镜像前，
维护者需要核对自己的平台版本、厂商安全服务与加密格式，并完成真机测试。
编译由维护者执行；Python 部署测试和合成测试数据只用于检查相应逻辑。

## 新设备从哪里开始

设备适配放在 `bootable/recovery` 之外：

```text
device/<厂商>/<设备>/recovery-crypto/
  Android.bp
  recovery.crypto.conf
  recovery.crypto.fstab
  init*.rc
  VINTF 片段
  sepolicy/
  厂商依赖的 Recovery 打包声明
```

建议按以下顺序接入：

1. 核对正常系统的 `/metadata`、`/data` fstab、内核加密能力，以及当前 ROM
   使用的密钥和 Synthetic Password 格式。
2. 核对实际 KeyMint、Gatekeeper、Weaver、SecureClock、SharedSecret 服务和
   固件依赖；确认它们能在 Recovery 环境运行。
3. 参考 [Android.bp 模板](device.example.bp)、[产品配置模板](device.example.mk)
   和 [后端配置示例](profile.example.conf)，在设备树创建自己的模块和配置。
4. 打包完整厂商库依赖，添加 init、Recovery VINTF、设备节点和 SELinux 规则。
5. 准备平台 Recovery 依赖和密钥读取策略，显式启用本设备的后端。
6. 检查实际镜像内容，编译后通过诊断日志逐阶段验证解密。

高通公共层可以共享认证桥源码及基础规则，但不能替代设备的库选择、服务实例、
fstab、固件路径和启动流程。已有的
[diting 适配说明](https://github.com/Night-stars-1/uwuaosp-diting/blob/main/cloud/DITING_RECOVERY_CRYPTO.md)
可作为实例，不能直接复制到所有高通设备。

## 已实现的解锁链路

术语：DE 是设备加密存储，CE 是凭据加密存储，SP 是 Synthetic Password
（合成密码），LSKF 是锁屏知识因子，即用户的 PIN、密码或图案。

1. 读取 ramdisk 中 root 拥有且不可写的配置，连接指定的 AIDL KeyMint，检查
   安全级别，协商配置中的 SharedSecret 参与者，再连接 AIDL 或 HIDL
   Gatekeeper/Weaver。AIDL 使用非 lazy 的 `checkService`；当前审查的 Recovery
   libhidl 通过本地 passthrough 实现解析 HIDL，不使用 hwservicemanager。
   不生成 KeyMint 密钥、不注册或删除凭据。若 keystore2 守护进程同时运行，
   后端拒绝继续，避免覆盖另一个协商者的共享密钥协议状态。
2. 读取已审查的 crypto fstab，按需挂载 metadata，读取**已有**的 vold v1 密钥
   目录。使用原始 secdiscardable application ID 经 KeyMint 解密，为本次启动
   转换硬件封装密钥，创建只读 `dm-default-key` 映射。默认只读挂载 userdata，
   不运行 fsck、日志重放、F2FS 前滚恢复或格式化；可写 MTP 的显式选项见文末。
3. 从 `/data/unencrypted/key` 恢复系统 DE 密钥，再恢复所选用户的 DE 密钥。
   使用 `FS_IOC_ADD_ENCRYPTION_KEY` 安装到内核，核对密钥标识/描述符与实际
   目录加密策略，并确认密钥存在。
4. 从私有 locksettings 数据库快照读取当前 `sp-handle`，解析 PasswordData
   和 protector 状态，确定真实凭据类型。支持 PIN、密码字节和 Android 图案
   单元编码。当前空 LSKF protector 使用填充后的 `default-password` 路径，
   旧 protector 保留存储的 scrypt 参数。缺少 `.pwd` 文件不能单独证明没有
   锁屏凭据，存储的 password quality 还必须明确为 0。
5. 经 Gatekeeper（包括其虚拟用户 ID 偏移）或 protector 的 Weaver 槽验证
   LSKF，保留硬件错误与限流，不重新注册凭据。将 Gatekeeper 返回的认证令牌
   用于 KeyMint，必要时使用 SecureClock。从已审查的 keystore2 数据库读取
   已有 SP protector 密钥，按正确的 v1/v2/v3 顺序解封 SP；不导入密钥、迁移
   namespace 或写数据库。
6. 保持 SP 的 ASCII 十六进制表示，使用 v1/v2 SHA-512 或 v3 SP800-108
   HMAC-SHA256 派生 `fbe-key`，恢复已有 CE 密钥。轮换的 `cx*` 候选不会被
   重命名、固定或删除。核对安装的密钥与 `/data/media/<user>` 的加密策略。
   通用框架还会独立检查挂载、内核密钥状态与目录可读性，随后才打开 ZIP 浏览器。

SQLite 数据库及 WAL 通过不跟随符号链接的文件描述符读取。只将已提交且校验和
有效的 WAL 帧应用到私有内存映像，不发布未提交或不完整的帧。SQLite 使用
`:memory:` 和只读反序列化，不操作原数据库或 SHM。发现非空的 hot rollback
journal 时停止，不执行数据库恢复；读取快照前 userdata 必须处于只读状态。

后端不包含破坏性回退。待升级密钥、`KEY_REQUIRES_UPGRADE`、缺失密钥、损坏的
protector、服务不可用或格式不支持，都会返回 ABI 错误并保留原有密钥与凭据。
已成功或部分安装的内核密钥保留到重启；worker 释放自己的秘密和句柄，不驱逐
这些密钥。worker 卡死或崩溃仍受父框架的截止时间约束。

device-mapper 的私有表加载与激活请求使用 libdm 稳定的 `4.0.0` 请求 ABI，
遵循 `DeviceMapper::InitIo`，不直接采用构建主机 Linux 头文件中的较新版本。
映射保持只读，ioctl 缓冲区会清除；映射表及密钥不会进入 libdm 的表调试日志。

## 配置与显式启用

后端及 HAL/SQLite 依赖在 Soong 模块层面默认关闭。同步本目录不会为其他产品
启用解密或增加厂商服务。

为设备模板中的模块选择唯一名称。每个产品只安装一个实际文件名为
`librecovery_crypto_backend` 的后端。配置与独立 fstab 安装到 Recovery ramdisk：

```text
/system/etc/recovery.crypto.conf
/system/etc/recovery.crypto.fstab
```

自定义 crypto fstab 也必须位于 `/system/etc/`，不能从 userdata 或 metadata
选择配置文件。独立 crypto fstab 仅供可选后端使用，不替换普通 Recovery 分区表。

**使用正常系统的真实加密选项，不能照搬旧 Recovery 的 `ice,wrappedkey`。**
以下只演示格式，不代表任何设备都应采用这些选项：

```text
fileencryption=aes-256-xts:aes-256-cts:v2+inlinecrypt_optimized+wrappedkey_v0
keydirectory=/metadata/vold/metadata_encryption
metadata_encryption=aes-256-xts:wrappedkey_v0
```

只复制设备真实的 `/metadata`、`/data` 条目，保留其分区路径、文件系统和挂载
参数。准确配置所有需要的 KeyMint、Gatekeeper、Weaver、SecureClock 和
SharedSecret 参与者：

- `sharedsecret_services` 列出 AIDL 服务。
- 可选 `sharedsecret_hidl_instances=4.1/default,4.1/strongbox` 按安全级别选择
  实际提供的最高 HIDL Keymaster 版本。同一实例不能同时配置 4.0 和 4.1。
- 所有参与者收到相同的排序参数列表，且必须返回相同校验和。未配置 HIDL
  参与者时保留 AIDL 路径。
- 只有确认本设备所有 protector 都不需要某服务，才能将 transport 设置为
  `none` 并将 instance 留空。使用 Weaver 验证后，SP SID 仍可能需要 Gatekeeper。

配置不能执行命令、任意选择后端库、关闭认证或根据用户输入选择密钥 blob，
也不发出任意 init 命令。HIDL 需要 Recovery 中已审查的本地 passthrough 实现。

厂商 HAL、完整库依赖、固件、Binder 驱动、匹配的系统版本/安全补丁属性、VINTF
及 SELinux 权限由设备维护者提供。**SELinux 保持 enforcing，没有适用于所有
设备的厂商安全服务包。** 若 passthrough HAL 需要 vendor 私有属性，应放入正确
的 vendor HAL 域，并由设备提供已审查的 AIDL 桥，不能给 coredomain Recovery
添加违反 vendor 属性隔离的访问权限。

Recovery servicemanager 使用 `VintfObjectRecovery`，合并
`/system/etc/vintf/manifest/` 下的片段。添加唯一命名的 Recovery 片段，不覆盖
已有 health/fastboot 声明或正常 Android 的 vendor manifest。

## 准备平台 Recovery 依赖

本平台部分 AIDL 接口、analyzer 运行库和 SQLite 没有 Recovery 变体。
`prepare_android17.py` 只补充所需构建声明，不修改正常系统代码；修改前核对
已审查的源码哈希，在源码树外备份原文件，遇到未知声明或冲突即停止。

在 Android 源码根目录执行，替换 `/path/to/android` 为自己的路径。
每条命令单独执行，失败后停止：

预览：

```bash
python3 bootable/recovery/tools/crypto/prepare_android17.py /path/to/android
```

应用：

```bash
python3 bootable/recovery/tools/crypto/prepare_android17.py /path/to/android --apply
```

检查：

```bash
python3 bootable/recovery/tools/crypto/prepare_android17.py /path/to/android --check
```

这些命令不编译、不启用设备配置、不访问手机，也不读取真实用户密钥。
已审查的平台版本与格式相关源码哈希记录在 [source-review.json](source-review.json)。
平台更新改变这些源码时，需要重新审查。

helper 还给 `system/tools/aidl/Android.bp` 中的 `aidl-analyzer-main` 静态库增加
`recovery_available: true`。AIDL 会向生成的 C++ analyzer 传播接口的 Recovery
可用性，因此其静态依赖也必须提供同一镜像变体。这不代表会将 analyzer 安装到
Recovery，也不改变正常系统代码；声明变化时会拒绝写入并要求审查。

依赖声明属于 `hardware/interfaces`、`system/tools/aidl` 和 `external/sqlite`。
维护发行版时应提交到相应 fork 并由 manifest 跟踪，或者同步这些项目后重新应用。
设备适配属于 device/vendor 项目。`repo sync -c bootable/recovery` 只更新通用
后端和 helper，不执行 helper，也不覆盖其他项目的适配。

## 平台 SELinux 密钥访问补丁

helper 对 `system/sepolicy` 检查并应用 `tools/crypto/patches/` 中完整的已审查补丁：

- AOSP：`android17-recovery-key-access.patch`。
- uwuAOSP：`android17-uwu-recovery-key-access.patch`，审查基线为
  `b41cbf3b46882139654574fe46ca5cf8175bf8a5`，保留已有 apexd metadata 例外和
  无关规则。

脚本在隔离副本上使用 Git 检查原始或完整已应用状态，保留兼容的无关修改，
拒绝冲突和部分应用状态。**不使用正则生成或重写策略规则。** 输入与补丁哈希
记录在补丁旁的 JSON 清单中；必须有且仅有一个完整变体匹配，不混用不同补丁
的 hunk。未知的密钥隔离规则改动仍需审查。

补丁本身不授予权限。设备必须在 **BoardConfig** 中，与解密策略采用相同条件
显式启用：

```make
BOARD_SEPOLICY_M4DEFS += recovery_crypto_android17=true
```

只有该开关和 `target_recovery` 同时为真时，例外才生效。普通 Android 与未接入
设备保持原来的密钥隔离规则。接入的 Recovery 可读取已有普通文件中的密钥和
数据库、添加/查询内核 fscrypt 密钥；写入、执行、访问非普通密钥文件、设置
加密策略和移除密钥仍被禁止。

这一权限依赖可信 Recovery 代码，不将 UI 与 worker 隔离到不同 SELinux 域。
维护发行版时应把明确补丁纳入平台 SELinux fork，或同步 `system/sepolicy` 后
重新应用。设备权限与开关仍留在设备树，同步 Recovery 不会修改它们。

## 当前支持范围与限制

- 要求真实的 AIDL KeyMint 实现。仅有 HIDL Keymaster 的设备需要另行实现兼容
  适配；本后端不启动 keystore2 兼容守护进程，也不猜测转换接口。
- 支持 HIDL Gatekeeper 1.0、Weaver 1.0，以及 Keymaster 4.0/4.1 的 SharedSecret
  协商。HIDL 仍需 Recovery passthrough 库；只有服务程序或 factory 的设备可能
  需要设备树中的桥接，manifest 不能代替实现。
- 使用已审查的 keystore2 live client-key schema，包括 `blobentry.state`、SELinux
  domain 2、locksettings namespace 103 和已知 UUID 编码。拒绝旧 APP namespace
  密钥、super-encrypted/boot-level-bound blob 与未知元数据，不迁移或升级密钥。
- 支持 raw、KeyMint `wrappedkey_v0` 和上游 block-crypto `wrappedkey` 的运行时
  转换。上游 wrapped key 需要真实内核 ioctl。旧 dm-default-key option format 1、
  `ice` 别名、多设备 userdata 和 logical userdata 需要额外适配。
- 当前拒绝 storage-binding seed 配置。`storage_binding=none` 必须基于正常平台
  确实未使用 seed 的核对，不能插入猜测或公开 seed，也不能绕过检查。
- 仅支持内部用户，UI 当前只展示用户 0。不支持工作资料挑战、可采纳存储、
  escrow-token 解锁、3×3～6×6 之外的图案网格或非 ASCII 屏幕密码键盘。
- 默认只读挂载拒绝需要恢复的文件系统，不通过写入修复损坏或未干净卸载的分区。
- 框架凭据和 IPC 缓冲区锁定内存且排除转储；派生字节和自身持有的 Binder 副本
  会清除，但不能保证编译器、HAL、Binder 或 OpenSSL 的内部副本全部锁定。
  worker 禁用 core dump；这不是对物理内存获取或厂商错误日志的安全认证。

## 解锁失败怎么定位

后端把固定诊断检查点追加到已有 `/tmp/recovery.log`，不依赖 logd/logcat。
worker 标准输出和错误仍重定向到 `/dev/null`。记录只含固定阶段名、公开结果码、
数字 errno/Binder/HAL 错误码，不含凭据、图案、用户 ID、文件路径、密钥、令牌
或厂商错误字符串。

诊断器拒绝符号链接、非普通文件、非 root 所有文件，以及达到 4 MiB 的日志，
不会自行创建日志。记录失败不改变解锁结果和 errno。

在含诊断代码的 Recovery 中尝试一次解锁，**重启 Recovery 前**提取：

```bash
adb -d pull /tmp/recovery.log recovery-decrypt.log
```

首先检查 `services`、`metadata`、`de_keys`、`credential_type`；随后检查 CE 子阶段、
`sp_unlock`，以及成功解锁 SP 后的 `ce_load`。汇总错误可能重复之前的错误，应
优先查看前面的具体子阶段。CE 密钥轮换可能尝试多个候选，不会重复认证；如果
最终 `ce_load` 成功，前一个候选失败不代表整体失败。

| 检查点 | 应核查的内容 |
| --- | --- |
| `protector_key` | 从 keystore 读取当前 SP protector 密钥及安全级别 |
| `credential_format`、`stretch` | 输入编码和存储的 scrypt 参数 |
| `gatekeeper_input`、`gatekeeper_verify`、`gatekeeper_token`、`weaver_read` | 输入边界、硬件验证、限流和令牌格式 |
| `keymint_begin`、`keymint_finish`、`secureclock` | KeyMint 密钥操作、认证解密和时间戳 |
| `sp_discardable`、`sp_software_decrypt`、`sp_format`、`sp_handle`、`sp_derive` | SP 状态、解封、主用户验证和 FBE 子密钥派生 |
| `ce_key_directories`、`stored_key_read`、`stored_key_decrypt` | 已有 CE 候选密钥及软件封装 |
| `storage_export` | 硬件封装存储密钥的转换 |
| `fscrypt_policy`、`fscrypt_descriptor`、`fscrypt_identifier` | 目录加密策略与密钥的匹配 |
| `fscrypt_add_key`、`fscrypt_status` | 内核密钥安装与存在状态 |

| `result` | 含义 |
| --- | --- |
| 0 | 成功 |
| 1 | 不支持 |
| 2 | 服务不可用 |
| 3 | 已有密钥或状态缺失 |
| 4 | 密钥需要升级 |
| 5 | 凭据被拒绝 |
| 6 | 硬件限流 |
| 7 | 其他 I/O、格式或密码学错误 |

`source=hal`、`binder`、`errno` 表示数字错误码来源。`source=none code=0` 仅表示
没有原始错误码，**不表示成功**。例如 `keymint_begin result=7 source=hal code=...`
定位到 KeyMint 拒绝，不能直接推断图案错误。诊断不会自动重试、注册、升级或
重写密钥。

Gatekeeper 已有 handle 可能是旧 version 0。已审查的
[GateKeeper::Verify](https://android.googlesource.com/platform/system/gatekeeper/+/5b5e75b5bda9fccbc3132e9624cb25286babdaac/gatekeeper.cpp)
拒绝超过支持上限的版本，不拒绝 version 0。后端保持大小和版本上限检查，将
旧 handle 交给配置的 HAL，不重新注册，并继续验证硬件认证令牌和 SID。
输入格式有效不能证明认证成功。

## 编译与真机验证

部署测试可在不编译 Android 的情况下运行。每条命令分别执行：

```bash
python3 bootable/recovery/tools/crypto/test_prepare_android17.py
```

```bash
python3 bootable/recovery/tools/crypto/test_policy_patch.py
```

策略测试仅在临时公开样本上应用补丁，并用 m4 验证四种 opt-in/Recovery 组合、
普通系统隔离与删除/写入限制，不编译 SELinux 二进制策略。

维护者应在普通 Android native-test 环境编译运行 `recovery_crypto_android17_test`。
它使用合成数据测试 SP 分版本 KDF、scrypt、认证 AES-GCM、截断 PasswordData
以及已提交且校验有效的 WAL，不调用 HAL 或读取真实凭据/密钥。该测试故意不
作为 Recovery 镜像模块，因为平台 gtest 没有 Recovery 变体；生产后端仍为
`recovery: true`。测试不能证明 Recovery 链接、HAL 可用或真机解密成功。

普通交互式进入 Recovery 时，已安装后端会先准备 metadata/DE，再显示解锁页。
取消或失败进入主界面；之后选择内部存储 ZIP 时，可以复用已验证的解锁状态或
再次尝试。命令驱动的 OTA/sideload、wipe、rescue、just-exit 和无界面启动不等待
输入凭据。启动交互流程不等于解决后端的解锁错误。

图案界面支持可触摸的 3×3～6×6 网格、选中圆点、连线及解锁/清除/取消按钮。
从圆点开始新笔画会清除之前的图案；至少四点且手指抬起后才能显式提交。跨过
同行、同列和 45 度对角线上的未访问单元时，遵循已审查的 uwuAOSP
LockPatternView 规则。音量键导航网格和按钮，电源键选择。自定义/简化 UI
可以拒绝可选图案接口，不影响 sideload。

UI 直接访问调用者锁定的凭据内存，不把图案复制到菜单文本或日志。取消会清除
凭据，离开时重绘两个 framebuffer 页面；普通菜单仍可滑动。合成布局和笔画
测试位于 `tests/unit/screen_ui_test.cpp`，不做真实凭据验证。

后端在 metadata/DE 恢复后从只读 locksettings 快照读取 `lock_pattern_size`。
缺少设置时默认 3；数据库失败、重复/损坏值或不支持的尺寸会停止，不提供手动
覆盖，也不自动尝试其他尺寸。单元编码为 `row * gridSize + column + '1'` 单字节，
包括索引大于 8 的单元。可选导出 `recovery_crypto_get_pattern_size_v1` 不改变
必需 v1 ABI 结构；没有此导出的自定义后端保持 3×3。私有 worker 协议为 v2，
Recovery 与 worker 必须一起重编译。相关格式与协议测试位于
`crypto/android17/tests/native_test.cpp` 和 `crypto/tests/protocol_test.cpp`。

每个启用设备都应验证空锁屏、PIN、密码、图案，以及适用的 Weaver/Gatekeeper
路径；还需测试错误凭据、硬件限流、服务/密钥缺失、不支持或需要升级的 blob。
成功后核对 fscrypt 密钥与目录策略、文件名和内容；失败后确认 ADB sideload
仍正常，重启 Android 后原有凭据仍有效。不能用格式化数据代替解密适配。

## 可选的可写 MTP 导出

Recovery MTP 的可信 `VID:PID:rw` 配置会改变挂载生命周期，详见
[MTP 文档](../../mtp/README.md)。没有该配置时默认只读。显式启用后，初始只读
挂载允许 userdata 日志/前滚恢复；只有 CE 密钥恢复成功后才尝试重新挂载为
可写。密钥与 locksettings 仍受只读访问规则保护。此路径必须验证设备文件系统
和 SELinux，不能把“只读挂载”理解为启用可写 MTP 后完全没有磁盘写入。

## 设计参考

- [AOSP 文件级加密](https://source.android.com/docs/security/features/encryption/file-based)
- [AOSP metadata 加密](https://source.android.com/docs/security/features/encryption/metadata)
- [AOSP 硬件封装密钥](https://source.android.com/docs/security/features/encryption/hw-wrapped-keys)
- [已审查平台源码与版本](source-review.json)

// Generated from translations.json. SPDX-License-Identifier: Apache-2.0
#pragma once
#include <atomic>
#include <string>
#include <string_view>
namespace recovery_m3e {
enum class Language { English, Chinese };
inline std::atomic<Language> ui_language{Language::English};
inline Language GetLanguage() { return ui_language.load(std::memory_order_relaxed); }
inline void SetLanguage(Language language) { ui_language.store(language, std::memory_order_relaxed); }
inline bool SupportedLocale(const std::string& locale) { return locale == "en-US" || locale == "zh-CN"; }
inline Language LanguageForLocale(const std::string& locale) { return locale.rfind("zh", 0) == 0 ? Language::Chinese : Language::English; }
struct Translation { std::string_view key, chinese; };
inline constexpr Translation kTranslations[] = {
  {"Recovery","恢复模式"},
  {"Install update","安装更新"},
  {"Choose an update method","选择更新方式"},
  {"Restart","重启"},
  {"Back to Android","返回 Android 系统"},
  {"Settings","设置"},
  {"Language","语言"},
  {"Language / 语言","语言 / Language"},
  {"Recovery preferences","语言、日志与维护"},
  {"Factory reset","恢复出厂设置"},
  {"Review erase options","查看擦除选项"},
  {"Update, recover, restart.","更新、恢复与重启。"},
  {"Apply update","安装更新"},
  {"Reboot system now","重启系统"},
  {"Advanced","高级工具"},
  {"Advanced options","高级工具"},
  {"Tools","工具"},
  {"Reboot options","重启选项"},
  {"Advanced tools","高级工具"},
  {"English","English"},
  {"简体中文","简体中文"},
  {"Current language","当前语言"},
  {"Tap to use this language","点按切换为此语言"},
  {"Choose your recovery language","选择恢复模式的显示语言"},
  {"System, bootloader and recovery","系统、引导程序与恢复模式"},
  {"Diagnostics and maintenance","诊断与维护工具"},
  {"Enter fastboot","进入 fastbootd"},
  {"Manage logical partitions","管理逻辑分区"},
  {"Reboot to bootloader","重启到引导程序"},
  {"Open the bootloader","进入 Bootloader"},
  {"Reboot to recovery","重启恢复模式"},
  {"Restart this recovery","重新启动 Recovery"},
  {"Mount/unmount system","挂载或卸载系统"},
  {"View recovery logs","查看恢复日志"},
  {"Read the complete output","阅读完整运行日志"},
  {"Enable ADB","启用 ADB"},
  {"Enable the debugging connection","开启调试连接"},
  {"Run graphics test","运行图形测试"},
  {"Run locale test","运行语言资源测试"},
  {"Enter rescue","进入救援模式"},
  {"Power off","关机"},
  {"Turn off the device","关闭设备电源"},
  {"Apply from ADB","通过 ADB 安装"},
  {"Apply from storage","从存储安装"},
  {"Send a package from your computer","从电脑发送更新包"},
  {"Cancel","取消"},
  {"No","否"},
  {"Yes","是"},
  {"Go back without applying this action","返回，不执行此操作"},
  {"Factory data reset","恢复出厂设置"},
  {"Wipe all user data?","要擦除所有用户数据吗？"},
  {"THIS CAN NOT BE UNDONE!","此操作无法撤销！"},
  {"This cannot be undone.","此操作无法撤销。"},
  {"Erase all user data?","要擦除所有用户数据吗？"},
  {"Format data/factory reset","格式化数据并恢复出厂设置"},
  {"Format cache partition","格式化缓存分区"},
  {"Format system partition","格式化系统分区"},
  {"Back","返回"},
  {"Confirm or select","确认或选择"},
  {"Select file to view","选择要查看的日志"},
  {"Tap to open  /  Swipe to scroll","点按进入  /  上下滑动滚动"},
  {"Volume: move  /  Power: select","音量键：移动  /  电源键：选择"},
  {"Press to move  /  Hold to select","短按移动  /  长按选择"},
  {"RECENT OUTPUT","最近输出"},
  {"Slot ","槽位 "},
  {"Language saved","语言已保存"},
  {"Language changed for this session","语言已在本次会话生效"},
  {"Language will be remembered when storage is writable","存储可写时记住语言选择"},
  {"Simplified Chinese","简体中文"},
  {"Format user data?","要格式化用户数据吗？"},
  {"This includes internal storage.","这也会擦除内部存储。"},
  {"THIS CANNOT BE UNDONE!","此操作无法撤销！"},
  {"Format data","格式化数据"},
  {"Format cache?","要格式化缓存吗？"},
  {"Format system?","要格式化系统吗？"},
  {"Signature verification failed","签名验证失败"},
  {"Install anyway?","仍要安装吗？"},
  {"This package will downgrade your system","此更新包会将系统降级"},
  {"To install additional packages, you need to reboot recovery first","安装其他更新包前，需要先重启恢复模式"},
  {"Do you want to reboot to recovery now?","现在重启恢复模式吗？"},
  {"WARNING: Security patch level downgrade detected. This may require formatting data. Device may brick if hardware rollback protection is enabled. ","警告：检测到安全补丁级别降级。可能需要格式化数据。如果启用了硬件防回滚保护，设备可能无法启动。"},
  {"Overwrite in-progress update?","要覆盖正在进行的更新吗？"},
  {"An update may already be in progress. If you proceed, the existing OS may not longer boot, and completing an update via ADB will be required.","可能已有更新正在进行。继续操作可能导致现有系统无法启动，需要通过 ADB 完成更新。"},
  {"Continue","继续"},
  {"Try again","重试"},
  {"Fastboot","Fastboot"},
  {"Reboot options and maintenance","重启选项与维护"},
  {"Language / Language","语言 / Language"},
  {"ADB Sideload","ADB 侧载"},
  {"Install result","安装结果"},
  {"Waiting for a package","等待更新包"},
  {"Verifying update","正在校验更新包"},
  {"Installing update","正在安装更新"},
  {"Installing security update","正在安装安全更新"},
  {"Installation complete","安装完成"},
  {"Installation failed","安装失败"},
  {"Installation not started","安装未开始"},
  {"Send the update package from your computer.","请从电脑发送更新包。"},
  {"Checking the package signature.","正在验证更新包签名。"},
  {"Keep the USB cable connected.","请保持 USB 连接。"},
  {"You can return to the menu or read the log.","可以返回菜单或查看日志。"},
  {"Open the recovery log for details.","请查看恢复日志了解原因。"},
  {"Cancelled, or no package was received.","操作已取消，或未收到更新包。"},
  {"Choose ZIP from internal storage","从内部存储选择 ZIP"},
  {"Unlock internal storage","解锁内部存储"},
  {"Unlock storage","解锁存储"},
  {"Delete last character","删除最后一个字符"},
  {"Clear input","清空输入"},
  {"Enter your lock-screen PIN","输入锁屏 PIN"},
  {"Enter your lock-screen password","输入锁屏密码"},
  {"Select pattern dots in order (1-9, top-left to bottom-right)","按顺序选择图案点（1 至 9，从左上到右下）"},
  {"Space","空格"},
  {"Entered characters: %d","已输入字符数：%d"},
  {"Retry after %d seconds","请等待 %d 秒后重试"},
  {"Starting storage security services","正在启动存储安全服务"},
  {"Unlocking metadata encryption","正在解锁元数据加密"},
  {"Loading device-encrypted keys","正在加载设备加密密钥"},
  {"Checking credential type","正在检查凭据类型"},
  {"Unlocking credential-encrypted storage","正在解锁凭据加密存储"},
  {"Checking internal storage access","正在检查内部存储访问"},
  {"Unlocking internal storage","正在解锁内部存储"},
  {"Internal storage is unlocked","内部存储已解锁"},
  {"Enter the Android lock-screen credential","输入 Android 锁屏凭据"},
  {"Storage decryption is not configured for this device","此设备尚未适配存储解密"},
  {"Storage security services are unavailable","存储安全服务不可用"},
  {"An existing storage key is missing","缺少现有存储密钥"},
  {"A storage key requires a platform upgrade","存储密钥需要平台升级适配"},
  {"The credential was not accepted","凭据未通过验证"},
  {"Too many attempts; wait before trying again","尝试次数过多，请等待后再重试"},
  {"Storage decryption could not complete","存储解密未能完成"},
  {"The storage decryption backend is incompatible","存储解密后端不兼容"},
  {"Storage decryption timed out","存储解密超时"},
  {"The storage decryption worker stopped","存储解密进程已停止"},
  {"Internal storage is still locked","内部存储仍未解锁"},
};
inline std::string Tr(const std::string& text) {
  if(GetLanguage()!=Language::Chinese) return text;
  for(const auto& entry:kTranslations) if(text==entry.key) return std::string(entry.chinese);
  for(const auto& entry:kTranslations) {
    size_t token=entry.key.find("%d");
    if(token==std::string_view::npos) continue;
    std::string prefix(entry.key.substr(0,token)),suffix(entry.key.substr(token+2));
    if(text.size()<=prefix.size()+suffix.size() || text.compare(0,prefix.size(),prefix)!=0 ||
        text.compare(text.size()-suffix.size(),suffix.size(),suffix)!=0) continue;
    std::string number=text.substr(prefix.size(),text.size()-prefix.size()-suffix.size());
    if(number.find_first_not_of("0123456789")!=std::string::npos) continue;
    std::string translated(entry.chinese);
    size_t placeholder=translated.find("%d");
    if(placeholder!=std::string::npos) translated.replace(placeholder,2,number);
    return translated;
  }
  size_t start=text.find_first_not_of(" "),end=text.find_last_not_of(" ");
  if(start!=std::string::npos) {
    std::string trimmed=text.substr(start,end-start+1);
    for(const auto& entry:kTranslations) if(trimmed==entry.key) return std::string(entry.chinese);
  }
  return text;
}
}

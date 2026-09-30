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
};
inline std::string Tr(const std::string& text) {
  if(GetLanguage()!=Language::Chinese) return text;
  for(const auto& entry:kTranslations) if(text==entry.key) return std::string(entry.chinese);
  size_t start=text.find_first_not_of(" "),end=text.find_last_not_of(" ");
  if(start!=std::string::npos) {
    std::string trimmed=text.substr(start,end-start+1);
    for(const auto& entry:kTranslations) if(trimmed==entry.key) return std::string(entry.chinese);
  }
  return text;
}
}

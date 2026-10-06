#!/usr/bin/env python3
# SPDX-FileCopyrightText: The uwuAOSP Project
# SPDX-License-Identifier: Apache-2.0
"""Compile native installation UI checks and render previews; requires C++17 and Pillow."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    body = source.index('{', start)
    depth = 1
    end = body + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def routing_test():
    """Exercise the actual routing/setter/result methods with host UI/menu substitutes."""
    screen = (ROOT / 'recovery_ui/screen_ui.cpp').read_text(encoding='utf-8')
    recovery = (ROOT / 'recovery.cpp').read_text(encoding='utf-8')
    definitions = '\n'.join(function(screen, signature) for signature in (
        'void ScreenRecoveryUI::SetInstallStage(',
        'bool ScreenRecoveryUI::IsInstallPageLocked() const',
        'bool ScreenRecoveryUI::IsDesignMenuLocked() const',
        'bool ScreenRecoveryUI::IsDesignAdbLocked() const',
        'bool ScreenRecoveryUI::ShouldHoldMenuFrameLocked() const',
        'void ScreenRecoveryUI::update_screen_locked()',
        'void ScreenRecoveryUI::update_progress_locked()'))
    result = function(recovery, 'static void ShowInstallResult(')
    update_menu = function(recovery, 'static InstallResult apply_update_menu(')
    return r'''
#include <cstdio>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include "recovery_ui/m3e_design.h"
struct TestMenu { std::string title; std::string PageTitle() const {return title;} bool DashboardCandidate() const {return false;} };
int frame_flips=0;void gr_flip(){++frame_flips;}
struct ScreenRecoveryUI {
  using InstallStage = recovery_ui::InstallStage;
  std::mutex updateMutex;std::unique_ptr<TestMenu> menu_;
  InstallStage m3e_install_stage_=InstallStage::NONE;
  std::vector<std::string> m3e_install_logs_;int redraws=0;
  bool menu_transition_=false,show_text=true,pagesIdentical=false;
  bool m3e_adb_sideload_=false,terminal_visible_=false;
  bool fastbootd_logo_enabled_=false;
  bool IsDesignMenuLocked() const;bool IsDesignAdbLocked() const;
  void* pattern_input_=nullptr;char** text_=nullptr;char** file_viewer_text_=nullptr;
  void draw_screen_locked(){++redraws;}void draw_foreground_locked(){++redraws;}
  void update_screen_locked();void update_progress_locked();
  bool ShouldHoldMenuFrameLocked() const;
  void SetInstallStage(InstallStage);bool IsInstallPageLocked() const;
};
''' + definitions + r'''
enum InstallResult {INSTALL_SUCCESS, INSTALL_ERROR, INSTALL_CORRUPT, INSTALL_NONE, INSTALL_KEY_INTERRUPTED};
struct RecoveryUI {
  using InstallStage = recovery_ui::InstallStage;
  enum class KeyError : int { TIMED_OUT = -1, INTERRUPTED = -2 };
  bool visible=true;int logs=0;InstallStage stage=InstallStage::NONE;
  bool interrupted=false;
  std::deque<size_t> selections;std::vector<InstallStage> stages;
  std::vector<std::string> titles;
  void SetInstallStage(InstallStage s){stage=s;stages.push_back(s);}
  void ClearText(){}void ShowText(bool value){visible=value;}
  bool IsKeyInterrupted(){return interrupted;}
  void Print(const char*,...){}
  bool IsTextVisible(){return visible;}
  size_t ShowMenu(const std::vector<std::string>& headers,const std::vector<std::string>& items,
      size_t initial,bool menu_only,const std::function<int(int,bool)>&,bool=false) {
    assert(!headers.empty());titles.push_back(headers.front());
    if(headers.front()=="Install result") {
      assert(items==std::vector<std::string>({"Continue","View recovery logs"}));
      assert(menu_only);
    }
    assert(initial==0 && !selections.empty());
    auto result=selections.front();selections.pop_front();return result;
  }
  void ShowFile(const std::string& path){assert(stage==InstallStage::NONE);assert(path=="/tmp/recovery.log");++logs;}
};
struct Device {
  enum BuiltinAction{NO_ACTION};
  static constexpr int kRefresh=-20,kGoBack=-21,kGoHome=-22;
  RecoveryUI ui;RecoveryUI* GetUI(){return &ui;}int HandleMenuKey(int key,bool){return key;}
};
struct Paths {
  static Paths Get(){return {};}std::string temporary_log_file(){return "/tmp/recovery.log";}
};
struct VolumeInfo {bool mMountable=true;std::string mLabel="USB";};
struct VolumeManager {
  static VolumeManager* Instance(){static VolumeManager manager;return &manager;}
  void getVolumeInfo(std::vector<VolumeInfo>& volumes){volumes={{}};}
};
namespace recovery_mtp {
int starts=0,stops=0;bool Start(){++starts;return true;}bool Stop(){++stops;return true;}
}
bool InitializeVirtiofs(){return false;}bool RecoveryCryptoAvailable(){return true;}
std::deque<InstallResult> install_results;
int image_operations=0;
void FlashPartitionImage(Device*);
InstallResult NextInstallResult(){assert(!install_results.empty());auto r=install_results.front();install_results.pop_front();return r;}
InstallResult ApplyFromAdb(Device*,bool,Device::BuiltinAction*){return NextInstallResult();}
InstallResult ApplyFromEncryptedStorage(Device*){return NextInstallResult();}
InstallResult ApplyFromVirtiofs(Device*){return NextInstallResult();}
InstallResult ApplyFromStorage(Device*,VolumeInfo&){return NextInstallResult();}
struct Target {std::string name;};
std::string GetProperty(const std::string&,const std::string&){return "_a";}
size_t Select(Device* d,const std::vector<std::string>& h,const std::vector<std::string>& i){
  return d->ui.ShowMenu(h,i,0,true,[](int k,bool){return k;});
}
const char* FlashBlocker(Device*){return nullptr;}
void Notice(Device*,const std::string&){}
bool UnlockRecoveryStorage(Device*){return true;}
namespace recovery_crypto {std::string UserStoragePath(unsigned){return "/data/media/0";}}
std::deque<std::string> image_paths;
std::string ChooseRecoveryStorageFile(Device* d,const std::string&,const std::string&,const std::string& title){
  d->ui.titles.push_back(title);assert(!image_paths.empty());
  auto path=image_paths.front();image_paths.pop_front();return path;
}
bool FlashImage(Device*,const std::string&,const std::string&){++image_operations;return true;}
#define LOG(...) std::cout
''' + result + '\n' + update_menu + r'''
void CheckNavigation() {
  for(size_t child:{0,1}) {
    Device d;Device::BuiltinAction reboot=Device::NO_ACTION;
    d.ui.selections={child,static_cast<size_t>(Device::kGoBack)};
    install_results={INSTALL_NONE};
    assert(apply_update_menu(&d,&reboot)==INSTALL_NONE);
    assert(d.ui.titles.front()=="Apply update" && d.ui.titles.back()=="Apply update");
    assert(std::count(d.ui.titles.begin(),d.ui.titles.end(),"Apply update")==2);
    assert(d.ui.selections.empty() && install_results.empty());
  }
  Device success;Device::BuiltinAction reboot=Device::NO_ACTION;
  success.ui.selections={0};install_results={INSTALL_SUCCESS};
  assert(apply_update_menu(&success,&reboot)==INSTALL_SUCCESS);
  assert(success.ui.titles.size()==1);
  Device interrupted;interrupted.ui.selections={static_cast<size_t>(RecoveryUI::KeyError::INTERRUPTED)};
  assert(apply_update_menu(&interrupted,&reboot)==INSTALL_KEY_INTERRUPTED);

}
void CheckRouting() {
  CheckNavigation();
  ScreenRecoveryUI design;
  for(const auto& title:{"Settings","Language","Advanced tools","Install result","Flash result","Flash partition image","Confirm or select"}) {
    design.menu_=std::make_unique<TestMenu>(TestMenu{title});assert(!design.IsDesignMenuLocked());
  }
  for(const auto& title:{"Reboot options","Install update"}) {
    design.menu_=std::make_unique<TestMenu>(TestMenu{title});assert(design.IsDesignMenuLocked());
    design.fastbootd_logo_enabled_=true;assert(!design.IsDesignMenuLocked());design.fastbootd_logo_enabled_=false;
  }
  design.menu_=std::make_unique<TestMenu>(TestMenu{"FastbootD"});
  assert(!design.IsDesignMenuLocked());design.fastbootd_logo_enabled_=true;
  assert(design.IsDesignMenuLocked());design.pattern_input_=reinterpret_cast<void*>(1);
  assert(!design.IsDesignMenuLocked());design.pattern_input_=nullptr;
  design.fastbootd_logo_enabled_=false;
  design.menu_.reset();design.SetInstallStage(InstallStage::INSTALLING);assert(!design.IsDesignAdbLocked());
  design.SetInstallStage(InstallStage::WAITING);assert(design.IsDesignAdbLocked());
  design.SetInstallStage(InstallStage::VERIFYING);assert(!design.IsDesignAdbLocked());
  design.SetInstallStage(InstallStage::INSTALLING);assert(design.IsDesignAdbLocked());
  design.menu_=std::make_unique<TestMenu>(TestMenu{"Confirm or select"});assert(!design.IsDesignAdbLocked());
  design.menu_.reset();design.SetInstallStage(InstallStage::SUCCESS);assert(!design.IsDesignAdbLocked());
  ScreenRecoveryUI transition;transition.menu_transition_=true;
  auto flips=frame_flips;
  transition.update_screen_locked();transition.update_progress_locked();
  assert(transition.redraws==0 && frame_flips==flips);
  transition.SetInstallStage(InstallStage::WAITING);
  assert(!transition.ShouldHoldMenuFrameLocked() && transition.redraws==1);
  transition.m3e_install_stage_=InstallStage::NONE;transition.menu_transition_=true;
  char* viewer=nullptr;transition.file_viewer_text_=&viewer;transition.text_=&viewer;
  assert(!transition.ShouldHoldMenuFrameLocked());
  transition.text_=nullptr;transition.pattern_input_=&viewer;
  assert(!transition.ShouldHoldMenuFrameLocked());
  transition.pattern_input_=nullptr;transition.show_text=false;
  assert(!transition.ShouldHoldMenuFrameLocked());
  transition.show_text=true;transition.menu_=std::make_unique<TestMenu>(TestMenu{"Apply update"});
  assert(!transition.ShouldHoldMenuFrameLocked());
  ScreenRecoveryUI ui;assert(!ui.IsInstallPageLocked());
  ui.SetInstallStage(InstallStage::WAITING);assert(ui.IsInstallPageLocked());
  ui.menu_=std::make_unique<TestMenu>(TestMenu{"ADB Sideload"});assert(ui.IsInstallPageLocked());
  ui.SetInstallStage(InstallStage::VERIFYING);
  ui.menu_->title="Confirm or select";assert(!ui.IsInstallPageLocked());
  ui.menu_.reset();assert(ui.IsInstallPageLocked());
  ui.SetInstallStage(InstallStage::INSTALLING);assert(ui.IsInstallPageLocked());
  ui.menu_=std::make_unique<TestMenu>(TestMenu{"Factory reset"});assert(!ui.IsInstallPageLocked());
  ui.menu_->title="Install result";ui.SetInstallStage(InstallStage::ERROR);assert(ui.IsInstallPageLocked());
  ui.menu_->title="Flash result";ui.SetInstallStage(InstallStage::FLASH_ERROR);assert(ui.IsInstallPageLocked());
  ui.menu_->title="Confirm or select";assert(!ui.IsInstallPageLocked());
  ui.m3e_install_logs_={"error detail"};ui.SetInstallStage(InstallStage::NONE);assert(!ui.IsInstallPageLocked());
  ui.SetInstallStage(InstallStage::ERROR);assert(ui.m3e_install_logs_.empty());
  ui.SetInstallStage(InstallStage::WAITING);assert(ui.m3e_install_logs_.empty());
  Device cancelled;cancelled.ui.stage=InstallStage::WAITING;
  ShowInstallResult(&cancelled,INSTALL_NONE);
  assert(cancelled.ui.stage==InstallStage::NONE && cancelled.ui.logs==0);
  assert(cancelled.ui.stages==std::vector<InstallStage>{InstallStage::NONE});
  for(auto result:{INSTALL_SUCCESS,INSTALL_ERROR,INSTALL_CORRUPT}) {
    auto expected=result==INSTALL_SUCCESS?InstallStage::SUCCESS:InstallStage::ERROR;
    for(bool logs:{false,true}) {
      Device device;device.ui.selections=logs?std::deque<size_t>{1,0}:std::deque<size_t>{0};
      ShowInstallResult(&device,result);assert(device.ui.logs==(logs?1:0));
      assert(device.ui.stages.front()==expected && device.ui.stage==InstallStage::NONE);
      assert(device.ui.selections.empty());
    }
  }
  Device hidden;hidden.ui.visible=false;ShowInstallResult(&hidden,INSTALL_SUCCESS);
  assert(hidden.ui.stage==InstallStage::NONE && hidden.ui.logs==0);
  std::cout<<"PASS: actual screen routing, confirmation precedence, result choices and log return\n";
}
'''


def fastboot_flow():
    """Run actual fastboot entry and TextMenu render/navigation with host I/O substitutes."""
    screen = (ROOT / 'recovery_ui/screen_ui.cpp').read_text(encoding='utf-8')
    header = (ROOT / 'recovery_ui/include/recovery_ui/screen_ui.h').read_text(encoding='utf-8')
    fastboot = (ROOT / 'fastboot/fastboot.cpp').read_text(encoding='utf-8')
    menu_definitions = '\n'.join(function(screen, signature) for signature in (
        'Menu::Menu(', 'int Menu::selection() const', 'TextMenu::TextMenu(',
        'const std::vector<std::string>& TextMenu::text_headers() const',
        'std::string TextMenu::TextItem(', 'size_t TextMenu::MenuStart() const',
        'size_t TextMenu::MenuEnd() const', 'size_t TextMenu::ItemsCount() const',
        'bool TextMenu::ItemsOverflow(', 'int TextMenu::Select(', 'int TextMenu::SelectVisible(',
        'int TextMenu::Scroll(', 'bool TextMenu::DashboardCandidate() const',
        'std::string TextMenu::PageTitle() const', 'int TextMenu::DrawHeader(',
        'void TextMenu::SetViewport(', 'void TextMenu::SetMenuHeight(',
        'int TextMenu::DrawItems(', 'int TextMenu::HitTest('))
    actions = fastboot[fastboot.index('static const std::vector'):fastboot.index('void FillDefaultFastbootLines')]
    return r'''
#include <map>
#include <cstdarg>
namespace FastbootFlow {
namespace android::base {
std::map<std::string,std::string> properties;
std::string GetProperty(const std::string& key,const std::string& fallback) {
  auto found=properties.find(key);return found==properties.end()?fallback:found->second;
}
bool EqualsIgnoreCase(std::string a,std::string b) {
  std::transform(a.begin(),a.end(),a.begin(),::tolower);
  std::transform(b.begin(),b.end(),b.begin(),::tolower);return a==b;
}
std::string StringPrintf(const char* format,...) {
  char result[256];va_list args;va_start(args,format);vsnprintf(result,sizeof(result),format,args);
  va_end(args);return result;
}
}
struct RecoveryUI {
  enum class KeyError : int {TIMED_OUT=-1,INTERRUPTED=-2};
  bool wearable=false,logo=false,reset=false,visible=false;
  size_t chosen=0;std::vector<std::string> details,items;
  bool IsWearable(){return wearable;}void SetEnableFastbootdLogo(bool value){logo=value;}
  void ResetKeyInterruptStatus(){reset=true;}void SetTitle(const std::vector<std::string>& value){details=value;}
  void ShowText(bool value){visible=value;}
  size_t ShowMenu(const std::vector<std::string>& headers,const std::vector<std::string>& value,
      size_t initial,bool menu_only,const std::function<int(int,bool)>& handler) {
    assert(headers.empty() && initial==0 && !menu_only && handler(7,true)==7);
    items=value;return chosen;
  }
};
struct Device {
  enum BuiltinAction {NO_ACTION,REBOOT_FROM_FASTBOOT,ENTER_RECOVERY,REBOOT_BOOTLOADER,
                     SHUTDOWN_FROM_FASTBOOT,KEY_INTERRUPTED};
  RecoveryUI ui;bool started=false;RecoveryUI* GetUI(){return &ui;}
  void StartFastboot(){started=true;}int HandleMenuKey(int key,bool){return key;}
};
int bcb_clears=0;bool clear_bootloader_message(std::string*){++bcb_clears;return true;}
#define LOG(...) std::cout
''' + actions + '\n' + '\n'.join(function(fastboot, signature) for signature in (
        'void FillDefaultFastbootLines(', 'void FillWearableFastbootLines(',
        'Device::BuiltinAction StartFastboot(')) + r'''
#undef LOG
enum class UIElement {SCROLLBAR};
class DrawInterface {
 public:
  virtual ~DrawInterface()=default;
  virtual int MenuItemHeight() const=0;virtual int MenuItemSpacing() const=0;
  virtual int DrawMenuPrompt(int,int,const std::vector<std::string>&) const=0;
  virtual int DrawDashboard(int,int,int,int,bool) const=0;
  virtual void DrawMenuCard(int,int,const std::string&,bool,bool,bool,bool) const=0;
  virtual void SetColor(UIElement) const=0;virtual void DrawScrollBar(int,int) const=0;
};
''' + function(header, 'class Menu {') + ';\n' + function(header, 'class TextMenu :') + r''';
#define CHECK(condition) assert(condition)
#define CHECK_LT(a,b) assert((a)<(b))
#define CHECK_LE(a,b) assert((a)<=(b))
// Inherited TextMenu selection/scroll code uses Android's signedness warning policy.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
''' + menu_definitions + r'''
#pragma GCC diagnostic pop
#undef CHECK
#undef CHECK_LT
#undef CHECK_LE
struct Drawing : DrawInterface {
  PixelCanvas& canvas;int width;mutable std::vector<Rect> cards;
  Drawing(PixelCanvas& c,int w):canvas(c),width(w){}
  int MenuItemHeight() const override{return design::LayoutMetrics(width).row_height;}
  int MenuItemSpacing() const override{return design::LayoutMetrics(width).gap;}
  int DrawMenuPrompt(int,int,const std::vector<std::string>&) const override{return 0;}
  int DrawDashboard(int,int,int,int,bool) const override{return 0;}
  void SetColor(UIElement) const override{}void DrawScrollBar(int,int) const override{}
  void DrawMenuCard(int y,int w,const std::string& item,bool selected,bool active,bool first,bool last) const override {
    cards.push_back(design::LayoutMetrics(w).Card(y));
    design::Card(canvas,w,y,item,selected,active,first,last);
  }
};
void EntryFlow() {
  static bool checked=false;if(checked)return;checked=true;
  android::base::properties={{"ro.product.device","astonc"},{"ro.uwu.release","17.0.130"},
    {"ro.serialno","host-example"},{"ro.secure","1"},{"ro.bootloader","sample-bootloader"}};
  const std::vector<Device::BuiltinAction> normal{Device::REBOOT_FROM_FASTBOOT,Device::REBOOT_BOOTLOADER,
      Device::ENTER_RECOVERY,Device::SHUTDOWN_FROM_FASTBOOT};
  const std::vector<Device::BuiltinAction> wearable{Device::REBOOT_FROM_FASTBOOT,Device::ENTER_RECOVERY,
      Device::REBOOT_BOOTLOADER,Device::SHUTDOWN_FROM_FASTBOOT};
  for(bool wear:{false,true})for(size_t index=0;index<4;++index) {
    Device d;d.ui.wearable=wear;d.ui.chosen=index;
    assert(StartFastboot(&d,{})==(wear?wearable:normal)[index]);
    assert(d.started && d.ui.reset && d.ui.visible && d.ui.logo==!wear);
    if(!wear) {
      assert(d.ui.items==std::vector<std::string>({"Reboot system now","Reboot to bootloader","Enter recovery","Power off"}));
      auto info=ReadDeviceInfo(d.ui.details);assert(info.product=="astonc" && info.version=="17.0.130");
      assert(std::find(d.ui.details.begin(),d.ui.details.end(),"Serial number - host-example")!=d.ui.details.end());
      assert(std::find(d.ui.details.begin(),d.ui.details.end(),"Secure boot - yes")!=d.ui.details.end());
    } else assert(d.ui.items[1]=="Enter recovery" && d.ui.details.front()=="Android Fastboot");
  }
  for(auto [key,action]:std::vector<std::pair<RecoveryUI::KeyError,Device::BuiltinAction>>{
      {RecoveryUI::KeyError::INTERRUPTED,Device::KEY_INTERRUPTED},{RecoveryUI::KeyError::TIMED_OUT,Device::NO_ACTION}}) {
    Device d;d.ui.chosen=static_cast<size_t>(key);assert(StartFastboot(&d,{})==action);
  }
  for(const char* key:{"ro.uwu.release","ro.uwu.build.version","ro.lineage.build.version","ro.build.version.incremental"}) {
    android::base::properties.erase("ro.uwu.release");android::base::properties.erase("ro.uwu.build.version");
    android::base::properties.erase("ro.lineage.build.version");android::base::properties.erase("ro.build.version.incremental");
    android::base::properties[key]="fallback-version";std::vector<std::string> details;FillDefaultFastbootLines(details);
    assert(ReadDeviceInfo(details).version=="fallback-version");
  }
  android::base::properties["ro.uwu.release"]="17.0.130";
  std::cout<<"PASS: actual fastboot entry, live metadata mapping, wearable order, four actions, timeout and interrupt\n";
}
void Render(PixelCanvas& canvas,int w,int h) {
  EntryFlow();Device d;StartFastboot(&d,{});
  design::Header(canvas,w,h,design::Page::Fastboot);design::FastbootStatus(canvas,w,h);
  Drawing drawing(canvas,w);TextMenu menu(true,0,{},d.ui.items,0,0,drawing);
  assert(menu.PageTitle()=="FastbootD" && menu.IsMain());
  int top=design::MenuTop(w,h,design::Page::Fastboot),available=design::FooterTop(w,h)-Dp(w,10)-top;
  menu.SetViewport(w,available);assert(menu.MenuEnd()-menu.MenuStart()>=2);
  assert(menu.DrawItems(0,top,w,false)<=available);
  design::Footer(canvas,w,h,d.ui.details);
  assert(canvas.Has(Tr("Ready for operation")) && canvas.Has("17.0.130"));
  // Walk every real menu action, including scrolling to the last row on small screens.
  PixelCanvas navigation(w,h);Drawing nav_drawing(navigation,w);
  TextMenu nav(true,0,{},d.ui.items,0,0,nav_drawing);nav.SetViewport(w,available);
  for(int index=0;index<4;++index) {
    assert(nav.Select(index)==index);nav_drawing.cards.clear();
    assert(nav.DrawItems(0,top,w,true)<=available);
    for(size_t row=0;row<nav_drawing.cards.size();++row) {
      auto b=nav_drawing.cards[row];int hit=nav.HitTest(b.x+b.w/2,b.y-top+b.h/2,w);
      assert(hit==static_cast<int>(row));
      int logical=nav.SelectVisible(hit);assert(logical==static_cast<int>(nav.MenuStart()+row));
      assert(d.ui.items[logical]==nav.TextItem(logical));
      assert(nav.HitTest(b.x+b.w/2,b.y-top-1,w)==-1);
      assert(nav.HitTest(b.x,b.y-top,w)==-1);
    }
  }
  nav.Select(0);nav.Scroll(1);nav_drawing.cards.clear();
  assert(nav.DrawItems(0,top,w,false)<=available);
  assert(nav.selection()>=static_cast<int>(nav.MenuStart()) && nav.selection()<static_cast<int>(nav.MenuEnd()));
  nav.Scroll(-1);assert(nav.MenuStart()==0);
  assert(nav.Select(4)==0 && nav.Select(-1)==3);
  TextMenu confirmation(true,0,{"Confirm"},d.ui.items,0,0,nav_drawing);
  assert(confirmation.PageTitle()=="Confirm or select");
}
} // namespace FastbootFlow
'''


def compile_android(clang, build):
    source = (ROOT / 'recovery_ui/screen_ui.cpp').read_text(encoding='utf-8')
    unit = '''#include "recovery_ui/screen_ui.h"
#include "recovery_ui/m3e_install.h"
#include "recovery_ui/m3e_design.h"
#include "recovery_ui/m3e_terminal.h"
#include "recovery_ui/terminal_session.h"
#include <cstring>
unsigned int gr_get_width(const GRSurface*);
unsigned int gr_get_height(const GRSurface*);
void gr_clear();
int gr_fb_width();
int gr_fb_height();
void gr_color(unsigned char,unsigned char,unsigned char,unsigned char);
void gr_fill(int,int,int,int);
void gr_texticon(int,int,const GRSurface*);
// Compile the actual canvas against minui API declarations; no display backend is linked.
class GRSurface {
 public:
  static std::unique_ptr<GRSurface> Create(size_t,size_t,size_t,size_t);
  uint8_t* data();
  size_t row_bytes;
};
static void M3eSetColor(recovery_m3e::Color) {}
'''
    unit += function(source, 'class M3eCanvas :') + ';\n'
    for signature in ('void ScreenRecoveryUI::SetInstallStage(',
                      'bool ScreenRecoveryUI::IsDesignMenuLocked() const',
                      'int ScreenRecoveryUI::DrawDashboard(',
                      'void ScreenRecoveryUI::DrawMenuCard(',
                      'void ScreenRecoveryUI::draw_screen_locked()',
                      'void ScreenRecoveryUI::draw_menu_and_text_buffer_locked(',
                      'void ScreenRecoveryUI::draw_battery_capacity_locked()',
                      'bool TextMenu::DashboardCandidate() const',
                      'std::string TextMenu::PageTitle() const',
                      'void TextMenu::SetMenuHeight(',
                      'int TextMenu::DrawItems(',
                      'int TextMenu::HitTest(',
                      'int ScreenRecoveryUI::SelectMenu(int sel)',
                      'bool ScreenRecoveryUI::IsInstallPageLocked() const',
                      'bool ScreenRecoveryUI::IsDesignAdbLocked() const',
                      'void ScreenRecoveryUI::DrawTerminalLocked()',
                      'void ScreenRecoveryUI::ShowTerminal()',
                      'void ScreenRecoveryUI::DrawInstallPageLocked()'):
        unit += '\n' + function(source, signature) + '\n'
    path = build / 'android-install-ui.cpp'
    path.write_text(unit, encoding='utf-8')
    flags = [clang, '--target=aarch64-linux-android35', '-std=c++17', '-Wall', '-Wextra',
             '-Werror', '-I' + str(ROOT / 'recovery_ui/include'), '-c']
    subprocess.run(flags + [str(path), '-o', str(build / 'android-install-ui.o')], check=True)
    subprocess.run(flags + [str(ROOT / 'tools/m3e/test_install_ui.cpp'), '-o',
                            str(build / 'android-install-renderer.o')], check=True)
    fastboot = (ROOT / 'fastboot/fastboot.cpp').read_text(encoding='utf-8')
    entry = '''#include "recovery_ui/device.h"
#include <algorithm>
#include <functional>
#include <iostream>
namespace android::base {
std::string GetProperty(const std::string&,const std::string&);
bool EqualsIgnoreCase(const std::string&,const std::string&);
}
bool clear_bootloader_message(std::string*);
#define LOG(...) std::cout
'''
    entry += fastboot[fastboot.index('static const std::vector'):fastboot.index('void FillDefaultFastbootLines')]
    entry += '\n'.join(function(fastboot, signature) for signature in (
        'void FillDefaultFastbootLines(', 'void FillWearableFastbootLines(',
        'Device::BuiltinAction StartFastboot('))
    entry_path = build / 'android-fastboot-entry.cpp'
    entry_path.write_text(entry, encoding='utf-8')
    subprocess.run(flags + [str(entry_path), '-o', str(build / 'android-fastboot-entry.o')], check=True)
    print('PASS: Android arm64 object compilation; real UI class headers and canvas, minui API declarations')


def run(cxx, out, ndk_clang=None):
    out.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='m3e-install-') as temp:
        build = Path(temp)
        (build / 'install_routing.inc').write_text(routing_test(), encoding='utf-8')
        (build / 'fastboot_flow.inc').write_text(fastboot_flow(), encoding='utf-8')
        exe = build / ('test.exe' if os.name == 'nt' else 'test')
        subprocess.run([cxx, '-std=c++17', '-O1', '-Wall', '-Wextra', '-Werror',
                        '-DM3E_INSTALL_ROUTING_TEST', '-I' + str(ROOT / 'recovery_ui/include'),
                        '-I' + str(build), str(ROOT / 'tools/m3e/test_install_ui.cpp'),
                        '-o', str(exe)], check=True)
        env = os.environ.copy()
        if Path(cxx).is_absolute():
            env['PATH'] = str(Path(cxx).parent) + os.pathsep + env.get('PATH', '')
        with Image.open(ROOT / 'res-xxxhdpi/images/uwu_recovery_m3e.png') as logo:
            logo.convert('RGB').save(build / 'logo.ppm')
        subprocess.run([str(exe), str(build)], check=True, env=env)
        if ndk_clang:
            compile_android(ndk_clang, build)
        # Cancellation is checked above but returns to the menu, without a result page.
        names = ('waiting', 'verifying', 'installing', 'success', 'error')
        for locale in ('zh', 'en'):
            tile_w, tile_h, gap = 300, 667, 20
            sheet = Image.new('RGB', (3 * (tile_w + gap) + gap, 2 * (tile_h + 42) + 96), '#f1eef8')
            draw = ImageDraw.Draw(sheet)
            draw.text((20, 16), 'M3E Recovery | installation flow | ' + locale,
                      font=ImageFont.load_default(size=22), fill='#272034')
            draw.text((20, 48), 'Native C++ host preview; sample progress and logs, not a device screenshot.',
                      font=ImageFont.load_default(size=15), fill='#574e68')
            for index, name in enumerate(names):
                with Image.open(build / (name + '-' + locale + '.ppm')) as frame:
                    frame.save(out / (name + '-' + locale + '.png'))
                    x, y = gap + index % 3 * (tile_w + gap), 82 + index // 3 * (tile_h + 42)
                    draw.text((x, y), name.title(), font=ImageFont.load_default(size=17), fill='#272034')
                    sheet.paste(frame.resize((tile_w, tile_h), Image.Resampling.LANCZOS), (x, y + 25))
            sheet.save(out / ('install-flow-' + locale + '.png'))
        for frame_path in sorted(list(build.glob('grouped-list-*.ppm')) + list(build.glob('design-*.ppm'))):
            with Image.open(frame_path) as frame:
                frame.save(out / (frame_path.stem + '.png'))
    print('Previews: ' + str(out))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default=os.environ.get('CXX', 'c++'))
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--ndk-clang', help='Optional Android NDK clang++ executable for arm64 object checks')
    args = parser.parse_args()
    run(args.cxx, args.out.resolve(), args.ndk_clang)

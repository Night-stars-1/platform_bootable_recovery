#!/usr/bin/env python3
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
        'bool ScreenRecoveryUI::ShouldHoldMenuFrameLocked() const',
        'void ScreenRecoveryUI::update_screen_locked()',
        'void ScreenRecoveryUI::update_progress_locked()'))
    result = function(recovery, 'static void ShowInstallResult(')
    update_menu = function(recovery, 'static InstallResult apply_update_menu(')
    flash = (ROOT / 'install/image_flash.cpp').read_text(encoding='utf-8')
    flash_target = function(flash, 'bool ChooseFlashTarget(')
    flash_entry = function(flash, 'void FlashPartitionImage(')
    return r'''
#include <cstdio>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
struct TestMenu { std::string title; std::string PageTitle() const {return title;} };
int frame_flips=0;void gr_flip(){++frame_flips;}
struct ScreenRecoveryUI {
  using InstallStage = recovery_ui::InstallStage;
  std::mutex updateMutex;std::unique_ptr<TestMenu> menu_;
  InstallStage m3e_install_stage_=InstallStage::NONE;
  std::vector<std::string> m3e_install_logs_;int redraws=0;
  bool menu_transition_=false,show_text=true,pagesIdentical=false;
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
''' + flash_entry + '\n#undef LOG\n' + result + '\n' + update_menu + '\n' + flash_target + r'''
void CheckNavigation() {
  for(size_t child:{0,1,2,3}) {
    Device d;Device::BuiltinAction reboot=Device::NO_ACTION;
    d.ui.selections={child,static_cast<size_t>(Device::kGoBack)};
    if(child==1) d.ui.selections={1,2,static_cast<size_t>(Device::kGoBack)};
    else install_results={INSTALL_NONE};
    assert(apply_update_menu(&d,&reboot)==INSTALL_NONE);
    assert(d.ui.titles.front()=="Apply update" && d.ui.titles.back()=="Apply update");
    assert(std::count(d.ui.titles.begin(),d.ui.titles.end(),"Apply update")==2);
    assert(d.ui.selections.empty() && install_results.empty());
  }
  Device success;Device::BuiltinAction reboot=Device::NO_ACTION;
  success.ui.selections={2};install_results={INSTALL_SUCCESS};
  assert(apply_update_menu(&success,&reboot)==INSTALL_SUCCESS);
  assert(success.ui.titles.size()==1);
  Device interrupted;interrupted.ui.selections={static_cast<size_t>(RecoveryUI::KeyError::INTERRUPTED)};
  assert(apply_update_menu(&interrupted,&reboot)==INSTALL_KEY_INTERRUPTED);
  const auto back=static_cast<size_t>(Device::kGoBack);
  Device confirmation;Target target;
  // Target -> review -> confirmation cancel -> review cancel -> target back.
  confirmation.ui.selections={0,1,0,0,back};
  assert(!ChooseFlashTarget(&confirmation,{{"boot_b"}},{"boot_b"},"/tmp/boot.img",4096,&target));
  assert(confirmation.ui.titles==std::vector<std::string>({"Choose target partition","Review image flash",
      "Confirm image flash","Review image flash","Choose target partition"}));
  assert(target.name.empty());
  Device confirmed;confirmed.ui.selections={0,1,1};
  assert(ChooseFlashTarget(&confirmed,{{"boot_b"}},{"boot_b"},"/tmp/boot.img",4096,&target));
  assert(target.name=="boot_b");
  Device source;source.ui.selections={1,2};image_paths={""};
  auto operations=image_operations;FlashPartitionImage(&source);
  assert(image_operations==operations && source.ui.selections.empty() && image_paths.empty());
  assert(source.ui.titles==std::vector<std::string>({"Flash partition image",
      "Choose partition image","Flash partition image"}));
}
void CheckRouting() {
  CheckNavigation();
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


def compile_android(clang, build):
    source = (ROOT / 'recovery_ui/screen_ui.cpp').read_text(encoding='utf-8')
    unit = '''#include "recovery_ui/screen_ui.h"
#include "recovery_ui/m3e_install.h"
unsigned int gr_get_width(const GRSurface*);
unsigned int gr_get_height(const GRSurface*);
class M3eCanvas : public recovery_m3e::Canvas {
 public:
  void Fill(recovery_m3e::Rect,recovery_m3e::Color) override {}
  void Text(int,int,const std::string&,recovery_m3e::Font,recovery_m3e::Color,bool) override {}
};
'''
    for signature in ('void ScreenRecoveryUI::SetInstallStage(',
                      'bool ScreenRecoveryUI::IsInstallPageLocked() const',
                      'void ScreenRecoveryUI::DrawInstallPageLocked()'):
        unit += '\n' + function(source, signature) + '\n'
    path = build / 'android-install-ui.cpp'
    path.write_text(unit, encoding='utf-8')
    flags = [clang, '--target=aarch64-linux-android35', '-std=c++17', '-Wall', '-Wextra',
             '-Werror', '-I' + str(ROOT / 'recovery_ui/include'), '-c']
    subprocess.run(flags + [str(path), '-o', str(build / 'android-install-ui.o')], check=True)
    subprocess.run(flags + [str(ROOT / 'tools/m3e/test_install_ui.cpp'), '-o',
                            str(build / 'android-install-renderer.o')], check=True)
    print('PASS: Android arm64 object compilation; real UI class headers, substitute minui canvas')


def run(cxx, out, ndk_clang=None):
    out.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='m3e-install-') as temp:
        build = Path(temp)
        (build / 'install_routing.inc').write_text(routing_test(), encoding='utf-8')
        exe = build / ('test.exe' if os.name == 'nt' else 'test')
        subprocess.run([cxx, '-std=c++17', '-O1', '-Wall', '-Wextra', '-Werror',
                        '-DM3E_INSTALL_ROUTING_TEST', '-I' + str(ROOT / 'recovery_ui/include'),
                        '-I' + str(build), str(ROOT / 'tools/m3e/test_install_ui.cpp'),
                        '-o', str(exe)], check=True)
        env = os.environ.copy()
        if Path(cxx).is_absolute():
            env['PATH'] = str(Path(cxx).parent) + os.pathsep + env.get('PATH', '')
        with Image.open(ROOT / 'res-xxxhdpi/images/uwu_recovery_status.png') as logo:
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
    print('Previews: ' + str(out))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cxx', default=os.environ.get('CXX', 'c++'))
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--ndk-clang', help='Optional Android NDK clang++ executable for arm64 object checks')
    args = parser.parse_args()
    run(args.cxx, args.out.resolve(), args.ndk_clang)

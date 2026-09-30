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
        'bool ScreenRecoveryUI::IsInstallPageLocked() const'))
    result = function(recovery, 'static void ShowInstallResult(')
    return r'''
#include <cstdio>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
struct TestMenu { std::string title; std::string PageTitle() const {return title;} };
struct ScreenRecoveryUI {
  using InstallStage = recovery_ui::InstallStage;
  std::mutex updateMutex;std::unique_ptr<TestMenu> menu_;
  InstallStage m3e_install_stage_=InstallStage::NONE;
  std::vector<std::string> m3e_install_logs_;int redraws=0;
  void update_screen_locked(){++redraws;}
  void SetInstallStage(InstallStage);bool IsInstallPageLocked() const;
};
''' + definitions + r'''
enum InstallResult {INSTALL_SUCCESS, INSTALL_ERROR, INSTALL_CORRUPT, INSTALL_NONE};
struct RecoveryUI {
  using InstallStage = recovery_ui::InstallStage;
  bool visible=true;int logs=0;InstallStage stage=InstallStage::NONE;
  std::deque<size_t> selections;std::vector<InstallStage> stages;
  void SetInstallStage(InstallStage s){stage=s;stages.push_back(s);}
  bool IsTextVisible(){return visible;}
  size_t ShowMenu(const std::vector<std::string>&,const std::vector<std::string>& items,
      size_t initial,bool menu_only,const std::function<int(int,bool)>&) {
    assert(items==std::vector<std::string>({"Continue","View recovery logs"}));
    assert(initial==0 && menu_only && !selections.empty());
    auto result=selections.front();selections.pop_front();return result;
  }
  void ShowFile(const std::string& path){assert(stage==InstallStage::NONE);assert(path=="/tmp/recovery.log");++logs;}
};
struct Device {
  RecoveryUI ui;RecoveryUI* GetUI(){return &ui;}int HandleMenuKey(int key,bool){return key;}
};
struct Paths {
  static Paths Get(){return {};}std::string temporary_log_file(){return "/tmp/recovery.log";}
};
''' + result + r'''
void CheckRouting() {
  ScreenRecoveryUI ui;assert(!ui.IsInstallPageLocked());
  ui.SetInstallStage(InstallStage::WAITING);assert(ui.IsInstallPageLocked());
  ui.menu_=std::make_unique<TestMenu>(TestMenu{"ADB Sideload"});assert(ui.IsInstallPageLocked());
  ui.SetInstallStage(InstallStage::VERIFYING);
  ui.menu_->title="Confirm or select";assert(!ui.IsInstallPageLocked());
  ui.menu_.reset();assert(ui.IsInstallPageLocked());
  ui.SetInstallStage(InstallStage::INSTALLING);assert(ui.IsInstallPageLocked());
  ui.menu_=std::make_unique<TestMenu>(TestMenu{"Factory reset"});assert(!ui.IsInstallPageLocked());
  ui.menu_->title="Install result";ui.SetInstallStage(InstallStage::ERROR);assert(ui.IsInstallPageLocked());
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

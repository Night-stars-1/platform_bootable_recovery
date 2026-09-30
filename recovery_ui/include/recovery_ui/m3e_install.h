// SPDX-License-Identifier: Apache-2.0
#pragma once
#include "install_status.h"
#include "m3e.h"

namespace recovery_m3e {
using InstallStage = recovery_ui::InstallStage;
struct InstallLayout { Rect panel; int menu_y; };

inline int InstallButtonSpace(const Metrics& m,int rows) {
  return rows>0?rows*m.row_height+(rows-1)*m.gap+Dp(m.width,16):0;
}
inline bool CompactInstallHeader(const Metrics& m,int top,int bottom,int menu_rows) {
  return bottom-HeaderBottom(m,top,false)-InstallButtonSpace(m,menu_rows)<Dp(m.width,188);
}
inline int DrawInstallHeader(Canvas& c,const Metrics& m,int top,int bottom,int menu_rows,
                             bool back_selected,const std::vector<std::string>& details,
                             const Palette& p) {
  if(!CompactInstallHeader(m,top,bottom,menu_rows)) {
    return DrawHeader(c,m,top,menu_rows>0,back_selected,false,0,0,details,p,"Install update",false);
  }
  // Keep both result actions reachable on landscape/compact displays. The back hit area
  // stays identical to the regular header used by ScreenRecoveryUI::SelectMenu.
  auto back=BackBounds(m,top);int x=m.inset;
  if(menu_rows>0) {
    Surface(c,back,back.h/2,p.surface,back_selected,p.primary,Dp(m.width,2));
    DrawIcon(c,Inset(back,Dp(m.width,14)),Icon::Back,p.text);
    x+=back.w+Dp(m.width,12);
  }
  Label(c,m,x,top+Dp(m.width,13),m.width-m.inset-x-Dp(m.width,90),
        "Install update",Font::Menu,p.text,true);
  return top+back.h+Dp(m.width,12);
}

inline InstallLayout InstallationLayout(const Metrics& m,int top,int bottom,int menu_rows) {
  int gap=Dp(m.width,16);
  int buttons=InstallButtonSpace(m,menu_rows);
  int available=std::max(0,bottom-top-buttons);
  int panel_height=std::min(Dp(m.width,188),available);
  Rect panel{m.inset,top,m.width-2*m.inset,panel_height};
  return {panel,top+panel_height+gap};
}

inline const char* InstallTitle(InstallStage stage,bool security_update) {
  switch(stage) {
    case InstallStage::WAITING: return "Waiting for a package";
    case InstallStage::VERIFYING: return "Verifying update";
    case InstallStage::INSTALLING: return security_update?"Installing security update":"Installing update";
    case InstallStage::SUCCESS: return "Installation complete";
    case InstallStage::ERROR: return "Installation failed";
    case InstallStage::CANCELLED: return "Installation not started";
    default: return "Install update";
  }
}
inline const char* InstallHint(InstallStage stage) {
  switch(stage) {
    case InstallStage::WAITING: return "Send the update package from your computer.";
    case InstallStage::VERIFYING: return "Checking the package signature.";
    case InstallStage::INSTALLING: return "Keep the USB cable connected.";
    case InstallStage::SUCCESS: return "You can return to the menu or read the log.";
    case InstallStage::ERROR: return "Open the recovery log for details.";
    case InstallStage::CANCELLED: return "Cancelled, or no package was received.";
    default: return "";
  }
}
inline void DrawInstallPanel(Canvas& c,const Metrics& m,Rect panel,InstallStage stage,
                             double fraction,bool determinate,bool security_update,
                             const std::vector<std::string>& logs,const Palette& p) {
  if(panel.w<=0 || panel.h<=0) return;
  Color accent=stage==InstallStage::ERROR?p.error:stage==InstallStage::SUCCESS?p.alert_success:
      stage==InstallStage::CANCELLED?p.alert_warning:p.primary;
  Rounded(c,panel,Dp(m.width,28),stage==InstallStage::ERROR?p.error_surface:p.card);
  bool compact=panel.h<Dp(m.width,140);
  int pad=Dp(m.width,compact?8:16),x=panel.x+pad,y=panel.y+pad;
  int width=panel.w-2*pad,bottom=panel.y+panel.h-pad;
  int title_height=LineHeight(FontPixels(Font::Menu,m.width));
  bool progress_stage=stage==InstallStage::VERIFYING || stage==InstallStage::INSTALLING;
  bool complete=stage==InstallStage::SUCCESS;
  fraction=std::isfinite(fraction)?std::clamp(fraction,0.0,1.0):0.0;
  std::string percent=(progress_stage && determinate) || complete?
      std::to_string(complete?100:static_cast<int>(fraction*100))+"%":"";
  int percent_width=percent.empty()?0:TextWidth(percent,FontPixels(Font::Small,m.width),true);
  if(y+title_height<=bottom) {
    Label(c,m,x,y,width-(percent.empty()?0:percent_width+Dp(m.width,12)),
          InstallTitle(stage,security_update),Font::Menu,accent,true);
    if(!percent.empty()) c.Text(x+width-percent_width,y+(title_height-LineHeight(FontPixels(Font::Small,m.width)))/2,
                               percent,Font::Small,accent,true);
    y+=title_height+Dp(m.width,compact?6:10);
  }
  if((progress_stage || complete) && y+Dp(m.width,8)<=bottom) {
    Rect track{x,y,width,Dp(m.width,8)};
    Rounded(c,track,track.h/2,p.outline);
    int filled=complete?width:determinate?static_cast<int>(width*fraction):width/4;
    if(filled>0) Rounded(c,{x,y,filled,track.h},track.h/2,accent);
    y+=track.h+Dp(m.width,12);
  }
  int body_height=LineHeight(FontPixels(Font::Body,m.width));
  if(stage==InstallStage::WAITING && y+body_height<=bottom) {
    Label(c,m,x,y,width,"adb sideload <filename>",Font::Body,p.blue,true);
    y+=body_height+Dp(m.width,8);
  }
  auto hint=WrapText(Tr(InstallHint(stage)),width,FontPixels(Font::Body,m.width));
  int count=0;
  for(const auto& line:hint) {
    if(++count>2 || y+body_height>bottom) break;
    c.Text(x,y,line,Font::Body,p.secondary,false);y+=body_height;
  }
  int small_height=LineHeight(FontPixels(Font::Small,m.width));
  if(!logs.empty() && y+Dp(m.width,12)+2*small_height<=bottom) {
    y+=Dp(m.width,12);
    Label(c,m,x,y,width,"RECENT OUTPUT",Font::Small,p.secondary,true);y+=small_height;
    size_t lines=std::min<size_t>(2,(bottom-y)/small_height);
    size_t first=logs.size()>lines?logs.size()-lines:0;
    for(size_t i=first;i<logs.size();++i) {
      Label(c,m,x,y,width,logs[i],Font::Small,p.secondary);y+=small_height;
    }
  }
}
}  // namespace recovery_m3e

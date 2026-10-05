/*
 * SPDX-FileCopyrightText: The uwuAOSP Project
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once
#include "m3e.h"
#include "install_status.h"

namespace recovery_m3e::design {
// Only the pages shown in recovery-design-1.svg opt into this presentation.
enum class Page { None, Home, Reboot, Sources };
inline Page MenuPage(bool dashboard,const std::string& title) {
  if(dashboard) return Page::Home;
  if(title=="Reboot options") return Page::Reboot;
  if(title=="Install update") return Page::Sources;
  return Page::None;
}
inline bool AdbPage(bool adb,recovery_ui::InstallStage stage) {
  return adb && (stage==recovery_ui::InstallStage::WAITING ||
                 stage==recovery_ui::InstallStage::INSTALLING);
}
constexpr Color background{15,13,19},surface{36,36,36},text{255,255,255};
constexpr Color green{0,185,99},muted{174,173,180},track{63,64,68};
inline Metrics LayoutMetrics(int width) {
  Metrics m(width);m.inset=Dp(width,18);m.row_height=Dp(width,63);m.gap=Dp(width,5);return m;
}
inline Rect Back(int width) {return {Dp(width,27),Dp(width,28),Dp(width,33),Dp(width,33)};}
inline int FooterTop(int width,int height) {return height-Dp(width,55);}
inline int TitleTop(int width,int height) {return Dp(width,height<Dp(width,600)?65:112);}
inline int MenuTop(int width,int height,Page page) {
  return TitleTop(width,height)+Dp(width,page==Page::Home?71:39);
}
inline void Chevron(Canvas& c,Rect b,Color color=text) {
  int stroke=std::max(1,b.w/9);
  Line(c,b.x+b.w/3,b.y+b.h/4,b.x+2*b.w/3,b.y+b.h/2,stroke,color);
  Line(c,b.x+2*b.w/3,b.y+b.h/2,b.x+b.w/3,b.y+3*b.h/4,stroke,color);
}
enum class Glyph { Android, Chip, Refresh, Gear, Terminal, Storage, Warning, Phone };
inline void Symbol(Canvas& c,Rect b,Glyph glyph,Color color=text) {
  int stroke=std::max(1,b.w/12);
  auto line=[&](int x1,int y1,int x2,int y2) {
    Line(c,b.x+x1*b.w/24,b.y+y1*b.h/24,b.x+x2*b.w/24,b.y+y2*b.h/24,stroke,color);
  };
  if(glyph==Glyph::Terminal) {
    line(2,2,22,2);line(22,2,22,22);line(22,22,2,22);line(2,22,2,2);
    line(5,6,11,12);line(11,12,5,18);line(13,18,19,18);
  } else if(glyph==Glyph::Phone) {
    line(6,2,18,2);line(18,2,18,22);line(18,22,6,22);line(6,22,6,2);
    line(12,7,12,17);line(8,13,12,17);line(16,13,12,17);
  } else if(glyph==Glyph::Android) {
    line(6,3,9,7);line(18,3,15,7);
    Rounded(c,{b.x+b.w/6,b.y+b.h/3,2*b.w/3,b.h/2},b.w/5,color);
    c.Fill({b.x+b.w/6,b.y+7*b.h/12,2*b.w/3,b.h/4},color);
    Rounded(c,{b.x+b.w/3,b.y+b.h/2,b.w/12,b.w/12},b.w/24,background);
    Rounded(c,{b.x+7*b.w/12,b.y+b.h/2,b.w/12,b.w/12},b.w/24,background);
  } else if(glyph==Glyph::Chip || glyph==Glyph::Storage) {
    line(6,6,18,6);line(18,6,18,18);line(18,18,6,18);line(6,18,6,6);
    for(int a:{9,15}) {line(a,2,a,6);line(a,18,a,22);line(2,a,6,a);line(18,a,22,a);}
    if(glyph==Glyph::Chip) Rounded(c,Inset(b,b.w/3),b.w/16,color);
  } else if(glyph==Glyph::Warning) {
    line(12,3,2,21);line(2,21,22,21);line(22,21,12,3);line(12,8,12,14);line(12,17,12,18);
  } else if(glyph==Glyph::Refresh) {
    for(int a=35;a<330;a+=5) {
      double r=a*3.141592653589793/180,r2=(a+5)*3.141592653589793/180;
      Line(c,b.x+b.w/2+std::lround(b.w/3.0*std::sin(r)),b.y+b.h/2-std::lround(b.h/3.0*std::cos(r)),
        b.x+b.w/2+std::lround(b.w/3.0*std::sin(r2)),b.y+b.h/2-std::lround(b.h/3.0*std::cos(r2)),stroke,color);
    }
    line(16,2,17,8);line(17,8,22,6);
  } else {
    for(int a=0;a<360;a+=45) {
      double r=a*3.141592653589793/180;
      line(12+static_cast<int>(7*std::sin(r)),12+static_cast<int>(7*std::cos(r)),
           12+static_cast<int>(10*std::sin(r)),12+static_cast<int>(10*std::cos(r)));
    }
    for(int a=0;a<360;a+=10) {
      double r=a*3.141592653589793/180,r2=(a+10)*3.141592653589793/180;
      line(12+static_cast<int>(6*std::sin(r)),12+static_cast<int>(6*std::cos(r)),
           12+static_cast<int>(6*std::sin(r2)),12+static_cast<int>(6*std::cos(r2)));
    }
    Rounded(c,Inset(b,5*b.w/12),b.w/12,color);
  }
}
inline void Battery(Canvas& c,int width,int capacity,bool charging) {
  Metrics m(width);int h=Dp(width,28),pad=Dp(width,9),icon=Dp(width,18);
  std::string value=capacity>=0 && capacity<=100?std::to_string(capacity)+"%":"--%";
  if(charging) value+="+";
  int tw=TextWidth(value,FontPixels(Font::Small,width),false),bw=tw+3*pad+icon;
  Rect box{width-Dp(width,32)-bw,Dp(width,32),bw,h};
  Rounded(c,box,h/2,surface);
  c.Text(box.x+pad,box.y+(h-LineHeight(FontPixels(Font::Small,width)))/2,value,Font::Small,text,false);
  Rect cell{box.x+bw-pad-icon,box.y+Dp(width,9),icon,Dp(width,11)};
  Rounded(c,cell,Dp(width,2),text);
  c.Fill({cell.x+cell.w+Dp(width,1),cell.y+cell.h/3,Dp(width,1),std::max(1,cell.h/3)},text);
  c.Fill({cell.x+cell.w-Dp(width,4),cell.y+Dp(width,1),Dp(width,3),cell.h-Dp(width,2)},surface);
}
inline void Footer(Canvas& c,int width,int height,const std::vector<std::string>& details) {
  Metrics m(width);int y=FooterTop(width,height),pad=Dp(width,24),lh=LineHeight(FontPixels(Font::Code,width));
  Rounded(c,{Dp(width,4),y,width-2*Dp(width,4),height-y},CornerRadii{Dp(width,17),Dp(width,5)},surface);
  auto info=ReadDeviceInfo(details);
  auto date=info.version.find(" (");if(date!=std::string::npos)info.version.resize(date);
  int right=width-Dp(width,143),ty=y+Dp(width,15);
  Label(c,m,pad,ty,right-pad-Dp(width,8),"uwuAOSP recovery",Font::Code,text);
  Label(c,m,pad,ty+lh,right-pad-Dp(width,8),"codename: "+info.product,Font::Code,text);
  Label(c,m,right,ty,width-right-pad,"Recovery version",Font::Code,text);
  int tw=TextWidth(info.version,FontPixels(Font::Code,width),false,true);
  Label(c,m,std::max(right,width-pad-tw),ty+lh,width-right-pad,info.version,Font::Code,text);
}
inline void Header(Canvas& c,int width,int height,Page page,bool back_selected=false,
                   const std::string& override_title={}) {
  Metrics m(width);
  if(page!=Page::Home) {
    auto b=Back(width);Surface(c,b,b.h/2,surface,back_selected,text,Dp(width,1));
    DrawIcon(c,Inset(b,Dp(width,9)),Icon::Back,text);
  }
  int y=TitleTop(width,height);
  if(page==Page::Home) {
    int x=Dp(width,29);std::string brand="uwuAOSP";
    for(size_t i=0;i<brand.size();++i) {
      std::string letter=brand.substr(i,1);
      c.Text(x,y,letter,Font::DesignBrand,{static_cast<uint8_t>(147+i*5),static_cast<uint8_t>(232-i*4),255},true);
      x+=TextWidth(letter,FontPixels(Font::DesignBrand,width),true);
    }
    x+=Dp(width,6);
    Label(c,m,x,y,width-x-Dp(width,20),"Recovery",Font::DesignBrand,text,true);
  } else {
    std::string title=override_title.empty()?(page==Page::Reboot?"Reboot to...":"Install or update by..."):override_title;
    if(GetLanguage()==Language::Chinese && override_title.empty())title=Tr(page==Page::Reboot?"Reboot options":"Install update");
    Label(c,m,Dp(width,21),y,width-Dp(width,42),title,Font::DesignTitle,text,true);
  }
}
struct HomeLayout {std::array<Rect,5> buttons;int height;bool valid;};
inline HomeLayout Home(int width,int top,int available) {
  int pad=Dp(width,29),gap=Dp(width,17),hero=Dp(width,130),pill=Dp(width,59);
  int total=2*hero+pill+2*gap,w=width-2*pad,half=(w-gap)/2;
  return {{{{pad,top,w,hero},{pad,top+hero+gap,w-pill-Dp(width,12),pill},
    {width-pad-pill,top+hero+gap,pill,pill},{pad,top+hero+pill+2*gap,half,hero},
    {pad+half+gap,top+hero+pill+2*gap,w-half-gap,hero}}},total,available>=total};
}
inline int HitHome(int width,int available,int x,int y) {
  auto layout=Home(width,0,available);if(!layout.valid)return -1;
  for(int i=0;i<5;++i)if(InRounded(layout.buttons[i],Dp(width,i==1 || i==2?30:i==0?22:17),x,y))return i;
  return -1;
}
inline int Dashboard(Canvas& c,int width,int top,int available,int selected,bool active) {
  auto layout=Home(width,top,available);if(!layout.valid)return 0;
  Metrics m(width);int pad=Dp(width,24),ring=Dp(width,1);
  const Color colors[]={{59,49,80},{38,58,64},{38,58,64},{38,60,50},{61,40,43}};
  const Color arrows[]={{102,85,127},{64,94,99},{64,94,99},{70,100,81},{104,72,79}};
  const char* labels[]={"Install or update","Terminal","","Power","Reset"};
  for(int i=0;i<5;++i) {
    auto b=layout.buttons[i];Color bg=active && selected==i?arrows[i]:colors[i];
    Surface(c,b,Dp(width,i==1 || i==2?30:i==0?22:17),bg,selected==i,text,ring);
    if(i==2) {Symbol(c,Inset(b,Dp(width,18)),Glyph::Gear);continue;}
    int icon=Dp(width,i==1?27:30),iy=i==0?b.y+(b.h-icon)/2:b.y+Dp(width,i==1?16:29);
    Rect ib{b.x+pad,iy,icon,icon};
    if(i==1)Symbol(c,ib,Glyph::Terminal);
    else if(i==3)Symbol(c,ib,Glyph::Refresh);
    else if(i==0)Symbol(c,ib,Glyph::Phone);
    else DrawIcon(c,ib,Icon::Trash,text);
    int circle=Dp(width,27);
    Rect cb{b.x+b.w-Dp(width,i>=3?17:19)-circle,b.y+(i<=1?(b.h-circle)/2:b.h-Dp(width,53)),circle,circle};
    Rounded(c,cb,circle/2,arrows[i]);Chevron(c,Inset(cb,Dp(width,7)));
    int tx=b.x+pad+(i==0?Dp(width,46):i==1?Dp(width,42):0);
    int ty=i<=1?b.y+(b.h-LineHeight(FontPixels(Font::DesignTitle,width)))/2:b.y+b.h-Dp(width,53);
    std::string label=labels[i];
    if(GetLanguage()==Language::Chinese)label=Tr(i==0?"Install update":i==3?"Reboot options":i==4?"Factory reset":labels[i]);
    Label(c,m,tx,ty,cb.x-tx-Dp(width,4),label,Font::DesignTitle,text);
  }
  return layout.height;
}
inline int ExtraGap(int width,Page page,bool before_last) {return page==Page::Reboot && before_last?Dp(width,27):0;}
inline CornerRadii Corners(int width,bool first,bool last) {return {Dp(width,first?17:3),Dp(width,last?17:3)};}
inline void Card(Canvas& c,int width,int y,const std::string& name,bool selected,bool active,
                 bool first,bool last) {
  auto m=LayoutMetrics(width);Rect b=m.Card(y);
  Surface(c,b,Corners(width,first,last),active?track:surface,selected,text,Dp(width,1));
  int icon=Dp(width,26),pad=Dp(width,22),ix=b.x+pad,iy=b.y+(b.h-icon)/2;
  Glyph glyph=Glyph::Storage;Color color=text;std::string label=name;bool arrow=false;
  if(name=="Reboot system now") {glyph=Glyph::Android;color=green;label="System";}
  else if(name=="Enter fastboot") {glyph=Glyph::Warning;color={255,0,64};label="FastbootD";}
  else if(name=="Reboot to bootloader") {glyph=Glyph::Chip;label="Bootloader";}
  else if(name=="Reboot to recovery") {glyph=Glyph::Refresh;label="Recovery";}
  else if(name=="Apply from ADB") {glyph=Glyph::Android;color=green;label="adb sideload";arrow=true;}
  else if(name=="Choose ZIP from internal storage") {label="package from local storage";arrow=true;}
  else if(name=="Power off")DrawIcon(c,{ix,iy,icon,icon},Icon::Power,text);
  else arrow=true;
  if(name!="Power off")Symbol(c,{ix,iy,icon,icon},glyph,color);
  if(GetLanguage()==Language::Chinese)label=Tr(name);
  int right=b.x+b.w-pad;
  if(arrow) {
    int circle=Dp(width,24);Rect cb{right-circle,b.y+(b.h-circle)/2,circle,circle};
    Rounded(c,cb,circle/2,{54,55,59});Chevron(c,Inset(cb,Dp(width,7)));right=cb.x-Dp(width,12);
  }
  int tx=ix+icon+Dp(width,20);
    Font font=label=="package from local storage"?Font::Source:Font::Menu;
    c.Text(tx,b.y+(b.h-LineHeight(FontPixels(font,width)))/2,
    FitText(label,right-tx,FontPixels(font,width),false),font,text,false);
}
inline int HitList(int width,Page page,int count,int first,int total,int x,int y) {
  auto m=LayoutMetrics(width);int top=0;
  for(int row=0;row<count;++row) {
    int index=first+row;bool separate=page==Page::Reboot && index==total-1;
    if(row>0)top+=ExtraGap(width,page,separate);
    bool bottom=index+1==total || (page==Page::Reboot && index+2==total);
    if(InRounded(m.Card(top),Corners(width,index==0 || separate,bottom),x,y))return row;
    top+=m.Pitch();
  }
  return -1;
}
inline void Adb(Canvas& c,int width,int height,bool waiting,double fraction,bool determinate,
                const std::vector<std::string>& logs,const std::vector<std::string>& details,
                bool back_selected=false) {
  Metrics m(width);c.Fill({0,0,width,height},background);
  Header(c,width,height,Page::None,back_selected,"ADB Sideload");
  int inset=Dp(width,18),w=width-2*inset,y=MenuTop(width,height,Page::Sources);
  Label(c,m,inset,y,w,"On your computer, run below command to send package:",Font::Instruction,text);
  y+=Dp(width,16);Label(c,m,inset,y,w,"adb sideload <filename>",Font::Command,text,true);
  y+=Dp(width,26);Label(c,m,inset,y,w,"Status",Font::Body,text);y+=Dp(width,20);
  int bottom=FooterTop(width,height)-Dp(width,10),available=bottom-y;
  int panel_h=std::min(Dp(width,121),std::max(Dp(width,53),available/2));
  Rect panel{inset,y,w,panel_h};Rounded(c,panel,Dp(width,17),surface);
  if(waiting) {
    auto label=FitText(Tr("Waiting for package..."),w-Dp(width,42),FontPixels(Font::Menu,width),false);
    c.Text(inset+(w-TextWidth(label,FontPixels(Font::Menu,width),false))/2,
      y+(panel_h-LineHeight(FontPixels(Font::Menu,width)))/2,label,Font::Menu,text,false);
  } else {
    double value=std::isfinite(fraction)?std::clamp(fraction,0.0,1.0):0;
    int pad=Dp(width,21),ty=y+Dp(width,12);
    Label(c,m,inset+pad,ty,w-2*pad,"Installing package:",panel_h>=Dp(width,100)?Font::Menu:Font::Code,text);
    if(panel_h>=Dp(width,100)) {
      // ADB's protocol supplies /sideload/package.zip, not the host's original filename.
      Label(c,m,inset+pad,ty+Dp(width,24),w-2*pad,"package.zip",Font::Code,text,true);
    }
    int percent_y=panel.y+panel.h-Dp(width,51);
    Label(c,m,inset+pad,percent_y,w-2*pad,determinate?std::to_string(static_cast<int>(value*100))+"%":"...",
      Font::Heading,text);
    Rect bar{inset+pad,panel.y+panel.h-Dp(width,22),w-2*pad,Dp(width,12)};
    Rounded(c,bar,bar.h/2,track);
    if(determinate && value>0)Rounded(c,{bar.x,bar.y,std::max(1,static_cast<int>(bar.w*value)),bar.h},bar.h/2,green);
  }
  y+=panel_h+Dp(width,17);
  int lh=LineHeight(FontPixels(Font::Caption,width));
  if(bottom-y>=lh+Dp(width,29)) {
    Label(c,m,inset,y,w,"Logs",Font::Body,text);y+=Dp(width,20);
    Rect box{inset,y,w,bottom-y};Rounded(c,box,Dp(width,17),surface);
    int pad=Dp(width,21),ty=y+Dp(width,12);
    std::vector<std::string> rows;
    for(const auto& log:logs)for(const auto& row:WrapText(log,w-2*pad,FontPixels(Font::Caption,width),false,true))rows.push_back(row);
    int visible=std::max(0,(box.h-Dp(width,24))/lh),start=std::max(0,static_cast<int>(rows.size())-visible);
    for(int i=start;i<static_cast<int>(rows.size());++i) {c.Text(inset+pad,ty,rows[i],Font::Caption,text,false);ty+=lh;}
  }
  Footer(c,width,height,details);
}
} // namespace recovery_m3e::design

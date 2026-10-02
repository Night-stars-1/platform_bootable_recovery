/*
 * SPDX-FileCopyrightText: The uwuAOSP Project
 * SPDX-License-Identifier: Apache-2.0
 */
// Native M3E Recovery, second-generation layout. Shared by device and host preview.
#pragma once
#include "m3e_text.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>
namespace recovery_m3e {
struct Color { uint8_t r,g,b; };
// Alerts sit on the solid page background; composite here for identical device/host output.
inline Color CompositeOver(Color foreground,Color background,uint8_t alpha) {
  auto channel=[alpha](uint8_t front,uint8_t back) {
    return static_cast<uint8_t>((front*alpha+back*(255-alpha)+127)/255);
  };
  return {channel(foreground.r,background.r),channel(foreground.g,background.g),
          channel(foreground.b,background.b)};
}
struct Rect {
  int x,y,w,h;
  bool Contains(int px,int py) const {return px>=x && py>=y && px<x+w && py<y+h;}
};
class Canvas {
 public:
  virtual ~Canvas()=default;
  virtual void Fill(Rect,Color)=0;
  virtual void Text(int,int,const std::string&,Font,Color,bool)=0;
};
struct Palette {
  Color background{18,17,24},surface{30,28,38},card{38,35,47};
  Color primary{220,202,255},on_primary{45,26,68};
  Color text{246,241,251},secondary{171,164,184},outline{77,69,92};
  Color error{245,170,168},error_surface{46,29,34},on_error{66,20,27};
  Color pressed{227,213,255},mint{202,237,214},on_mint{23,59,40};
  Color blue{207,222,255},on_blue{29,48,79};
  Color alert_info{150,194,255},alert_success{151,217,171},alert_warning{255,193,115};
  uint8_t alert_opacity=24;  // About 9% tint; the page remains visible underneath.
  static Palette ForMode(bool fastboot) {
    Palette p;
    if(fastboot) {p.primary={255,208,153};p.on_primary={71,43,16};}
    return p;
  }
};
struct Metrics {
  int width,char_width,char_height,inset,gap,row_height;
  Metrics(int w,int=0,int=0,int margin=0):width(w),char_width(Dp(w,10)),
      char_height(Dp(w,24)),inset(std::max(margin,Dp(w,24))),gap(Dp(w,8)),row_height(Dp(w,76)){}
  Rect Card(int y) const {return {inset,y,width-2*inset,row_height};}
  int Radius() const {return Dp(width,22);}
  int Pitch() const {return row_height+gap;}
};
inline void Rounded(Canvas& c, Rect b, int radius, Color color) {
  if (b.w <= 0 || b.h <= 0) return;
  int r = std::max(0, std::min(radius, std::min(b.w, b.h) / 2));
  if (r == 0) { c.Fill(b, color); return; }
  c.Fill({b.x, b.y + r, b.w, b.h - 2 * r}, color);
  for (int y = 0; y < r; ++y) {
    double dy = r - y - 0.5;
    int dx = static_cast<int>(std::ceil(r - std::sqrt(r * r - dy * dy) - 0.5));
    c.Fill({b.x + dx, b.y + y, b.w - 2 * dx, 1}, color);
    c.Fill({b.x + dx, b.y + b.h - 1 - y, b.w - 2 * dx, 1}, color);
  }
}
inline bool InRounded(Rect b, int r, int x, int y) {
  if (!b.Contains(x, y)) return false;
  r = std::max(0, std::min(r, std::min(b.w, b.h) / 2));
  int cx = std::clamp(x, b.x + r, b.x + b.w - r - 1);
  int cy = std::clamp(y, b.y + r, b.y + b.h - r - 1);
  return (x - cx) * (x - cx) + (y - cy) * (y - cy) <= r * r;
}
inline void Line(Canvas& c, int x1, int y1, int x2, int y2, int stroke, Color color) {
  int steps = std::max(std::abs(x2 - x1), std::abs(y2 - y1));
  for (int i = 0; i <= steps; ++i) {
    float f = steps ? static_cast<float>(i) / steps : 0;
    int x = x1 + static_cast<int>((x2 - x1) * f);
    int y = y1 + static_cast<int>((y2 - y1) * f);
    Rounded(c, {x - stroke / 2, y - stroke / 2, stroke, stroke}, stroke / 2, color);
  }
}
enum class Icon { Arrow, Download, Power, Tools, Trash, File, Back, Language };
inline Icon IconFor(const std::string& label) {
  if (label=="Language" || label=="English" || label=="简体中文") return Icon::Language;
  if (label=="Settings" || label=="Advanced tools") return Icon::Tools;
  if (label.find("Reboot") != std::string::npos || label.find("Power") != std::string::npos) return Icon::Power;
  if (label.find("Factory") != std::string::npos || label.find("Format") != std::string::npos || label.find("Wipe") != std::string::npos) return Icon::Trash;
  if (label.find("Apply") != std::string::npos || label.find("ADB") != std::string::npos || label=="Flash partition image") return Icon::Download;
  if (label.find("Advanced") != std::string::npos || label.find("fastboot") != std::string::npos) return Icon::Tools;
  if (label.find('/') != std::string::npos || label.find(".zip") != std::string::npos || label.find(".img") != std::string::npos || label.find("log") != std::string::npos) return Icon::File;
  return Icon::Arrow;
}
inline void DrawIcon(Canvas& c, Rect b, Icon icon, Color color) {
  const int s = std::max(2, b.w / 10);
  auto line = [&](int x1, int y1, int x2, int y2) {
    Line(c, b.x + x1 * b.w / 24, b.y + y1 * b.h / 24,
         b.x + x2 * b.w / 24, b.y + y2 * b.h / 24, s, color);
  };
  switch (icon) {
    case Icon::Download:
      line(12, 3, 12, 16); line(6, 10, 12, 16); line(18, 10, 12, 16);
      line(4, 17, 4, 21); line(4, 21, 20, 21); line(20, 21, 20, 17); break;
    case Icon::Power: {
      line(12, 2, 12, 12);
      for (int a = 45; a < 315; a += 5) {
        auto xy = [&](int angle, bool x) {
          double rad = angle * 3.141592653589793 / 180;
          return static_cast<int>(12 + 9 * (x ? std::sin(rad) : -std::cos(rad)));
        };
        line(xy(a, true), xy(a, false), xy(a + 5, true), xy(a + 5, false));
      } break;
    }
    case Icon::Trash:
      line(4, 6, 20, 6); line(9, 3, 15, 3); line(6, 6, 7, 21);
      line(7, 21, 17, 21); line(17, 21, 18, 6); line(10, 10, 10, 17); line(14, 10, 14, 17); break;
    case Icon::Tools:
      line(4, 7, 20, 7); line(4, 17, 20, 17); line(9, 3, 9, 11); line(16, 13, 16, 21); break;
    case Icon::File:
      line(5, 2, 15, 2); line(15, 2, 20, 7); line(20, 7, 20, 22);
      line(20, 22, 5, 22); line(5, 22, 5, 2); line(9, 12, 16, 12); line(9, 17, 16, 17); break;
    case Icon::Language:
      line(3,6,15,6); line(9,2,9,6); line(6,7,12,15); line(12,7,5,16);
      line(14,21,18,11); line(18,11,22,21); line(16,17,20,17); break;
    case Icon::Back:
      line(20, 12, 4, 12); line(11, 5, 4, 12); line(11, 19, 4, 12); break;
    case Icon::Arrow:
      line(5, 12, 19, 12); line(12, 5, 19, 12); line(12, 19, 19, 12); break;
  }
}

inline int VisibleCount(int height,int row,int gap) {
  return row<=0 || gap<0 || height<row ? 0 : (height+gap)/(row+gap);
}
inline int HitRow(const Metrics& m,int top,int count,int,int x,int y) {
  if(y<top || count<=0) return -1;
  int row=(y-top)/m.Pitch();
  if(row>=count) return -1;
  return InRounded(m.Card(top+row*m.Pitch()),m.Radius(),x,y)?row:-1;
}
inline Rect Inset(Rect r,int amount) {return {r.x+amount,r.y+amount,r.w-2*amount,r.h-2*amount};}
inline void Surface(Canvas& c,Rect b,int radius,Color bg,bool selected,Color ring,int width) {
  if(selected) {
    Rounded(c,b,radius,ring);
    Rounded(c,Inset(b,width),std::max(0,radius-width),bg);
  } else Rounded(c,b,radius,bg);
}
inline void Label(Canvas& c,const Metrics& m,int x,int y,int width,const std::string& text,
                  Font f,Color color,bool bold=false) {
  c.Text(x,y,FitText(Tr(text),width,FontPixels(f,m.width),bold),f,color,bold);
}
inline std::string Subtitle(const std::string& name) {
  if(name=="Language") return "Choose your recovery language";
  if(name=="Reboot options") return "System, bootloader and recovery";
  if(name=="Advanced tools") return "Diagnostics and maintenance";
  if(name=="English") return GetLanguage()==Language::English?"Current language":"Tap to use this language";
  if(name=="简体中文") return GetLanguage()==Language::Chinese?"Current language":"Tap to use this language";
  if(name=="Apply from ADB") return "Send a package from your computer";
  if(name=="Flash partition image") return "Choose an IMG and a physical partition";
  if(name=="Enter fastboot") return "Manage logical partitions";
  if(name=="Reboot to bootloader") return "Open the bootloader";
  if(name=="Reboot to recovery") return "Restart this recovery";
  if(name=="View recovery logs") return "Read the complete output";
  if(name=="Enable ADB") return "Enable the debugging connection";
  if(name=="Power off") return "Turn off the device";
  if(name=="Cancel" || name=="No") return "Go back without applying this action";
  return {};
}
inline void DrawCard(Canvas& c,const Metrics& m,int y,const std::string& name,
                     bool selected,bool active,const Palette& p) {
  Rect b=m.Card(y);
  Icon icon=IconFor(name);
  bool danger=icon==Icon::Trash;
  Color bg=danger?(active?p.on_error:p.error_surface):(active?p.outline:p.card);
  Color fg=danger?p.error:p.text;
  Surface(c,b,m.Radius(),bg,selected,danger?p.error:p.primary,Dp(m.width,2));
  int pad=Dp(m.width,16), size=Dp(m.width,40);
  Rect badge{b.x+pad,b.y+(b.h-size)/2,size,size};
  Rounded(c,badge,Dp(m.width,14),danger?p.error_surface:p.surface);
  DrawIcon(c,Inset(badge,Dp(m.width,9)),icon,danger?p.error:p.primary);
  int x=badge.x+badge.w+Dp(m.width,12), available=b.x+b.w-pad-x;
  std::string sub=Subtitle(name);
  if(sub.empty()) {
    auto lines=WrapText(Tr(name),available,FontPixels(Font::Menu,m.width),selected);
    int lh=LineHeight(FontPixels(Font::Menu,m.width));
    int used=std::min(2,static_cast<int>(lines.size()));
    int ty=y+(b.h-used*lh)/2;
    for(int i=0;i<used;++i) {
      Label(c,m,x,ty+i*lh,available,lines[i]+(i==1 && lines.size()>2?"...":""),Font::Menu,fg,selected);
    }
  } else {
    Label(c,m,x,y+Dp(m.width,15),available,name,Font::Menu,fg,selected);
    Label(c,m,x,y+Dp(m.width,43),available,sub,Font::Small,p.secondary);
  }
}
struct DashboardLayout {std::array<Rect,4> cards;int height;bool valid;};
inline int DashboardMinimum(const Metrics& m) {return Dp(m.width,332);}
inline DashboardLayout Dashboard(const Metrics& m,int y,int available) {
  int height=std::min(available,Dp(m.width,388)),gap=Dp(m.width,12);
  if(height<DashboardMinimum(m)) return {{{}},0,false};
  int inner=height-2*gap,hero=inner*43/100,tiles=inner*37/100,reset=inner-hero-tiles;
  int width=m.width-2*m.inset,half=(width-gap)/2;
  return {{{{m.inset,y,width,hero},
           {m.inset,y+hero+gap,half,tiles},
           {m.inset+half+gap,y+hero+gap,width-half-gap,tiles},
           {m.inset,y+hero+tiles+2*gap,width,reset}}},height,true};
}
inline int HitDashboard(const Metrics& m,int available,int x,int y) {
  auto layout=Dashboard(m,0,available);
  if(!layout.valid) return -1;
  for(int i=0;i<4;++i) if(InRounded(layout.cards[i],Dp(m.width,i==3?24:30),x,y)) return i;
  return -1;
}
inline int DrawDashboard(Canvas& c,const Metrics& m,int y,int available,int selected,bool active,const Palette& p) {
  auto layout=Dashboard(m,y,available);
  if(!layout.valid) return 0;
  const int pad=Dp(m.width,22),ring=Dp(m.width,2);
  Rect b=layout.cards[0];
  Surface(c,b,Dp(m.width,30),active && selected==0?p.pressed:p.primary,selected==0,p.text,ring);
  int badge_size=Dp(m.width,40);
  Rect badge{b.x+pad,b.y+pad,badge_size,badge_size};
  Rounded(c,badge,Dp(m.width,15),p.on_primary);
  DrawIcon(c,Inset(badge,Dp(m.width,9)),Icon::Download,p.primary);
  DrawIcon(c,{b.x+b.w-pad-Dp(m.width,22),b.y+pad,Dp(m.width,22),Dp(m.width,22)},Icon::Arrow,p.on_primary);
  Label(c,m,b.x+pad,b.y+b.h-Dp(m.width,68),b.w-2*pad,"Install update",Font::Heading,p.on_primary,true);
  Label(c,m,b.x+pad,b.y+b.h-Dp(m.width,32),b.w-2*pad,"Choose an update method",Font::Body,p.on_primary);
  for(int i=1;i<=2;++i) {
    b=layout.cards[i];
    Color bg=i==1?p.mint:p.blue,fg=i==1?p.on_mint:p.on_blue;
    Surface(c,b,Dp(m.width,30),bg,selected==i,p.text,active && selected==i?2*ring:ring);
    DrawIcon(c,{b.x+pad,b.y+pad,Dp(m.width,28),Dp(m.width,28)},i==1?Icon::Power:Icon::Tools,fg);
    Label(c,m,b.x+pad,b.y+b.h-Dp(m.width,66),b.w-2*pad,i==1?"Restart":"Settings",Font::Heading,fg,true);
    Label(c,m,b.x+pad,b.y+b.h-Dp(m.width,32),b.w-2*pad,i==1?"Reboot options":"Recovery preferences",Font::Small,fg);
  }
  b=layout.cards[3];
  Surface(c,b,Dp(m.width,24),active && selected==3?p.on_error:p.error_surface,selected==3,p.error,ring);
  DrawIcon(c,{b.x+pad,b.y+(b.h-Dp(m.width,24))/2,Dp(m.width,24),Dp(m.width,24)},Icon::Trash,p.error);
  int tx=b.x+pad+Dp(m.width,40);
  Label(c,m,tx,b.y+Dp(m.width,12),b.x+b.w-pad-tx,"Factory reset",Font::Menu,p.error,true);
  Label(c,m,tx,b.y+Dp(m.width,39),b.x+b.w-pad-tx,"Review erase options",Font::Small,p.secondary);
  return layout.height;
}
struct DeviceInfo {std::string product,slot,version;std::vector<std::string> extra;};
inline DeviceInfo ReadDeviceInfo(const std::vector<std::string>& lines) {
  DeviceInfo info;
  for(const auto& line:lines) {
    if(line.rfind("Product name - ",0)==0) info.product=line.substr(15);
    else if(line.rfind("Product name: ",0)==0) info.product=line.substr(14);
    else if(line.rfind("Active slot: ",0)==0) info.slot=line.substr(13);
    else if(line.rfind("Version ",0)==0) info.version=line.substr(8);
    else if(!line.empty()) info.extra.push_back(line);
  }
  return info;
}
inline Rect BackBounds(const Metrics& m,int top) {return {m.inset,top,Dp(m.width,48),Dp(m.width,48)};}
inline int HeaderBottom(const Metrics& m,int top,bool dashboard) {
  int y=top+Dp(m.width,48)+Dp(m.width,14);
  y+=LineHeight(FontPixels(dashboard?Font::Title:Font::Heading,m.width))+Dp(m.width,dashboard?8:0);
  if(dashboard) y+=Dp(m.width,28);
  return y+Dp(m.width,dashboard?20:12);
}
inline void Chip(Canvas& c,const Metrics& m,int x,int y,const std::string& label,const Palette& p) {
  int height=Dp(m.width,28),pad=Dp(m.width,12);
  int width=TextWidth(label,FontPixels(Font::Small,m.width),true)+2*pad;
  Rounded(c,{x,y,width,height},height/2,p.surface);
  c.Text(x+pad,y+(height-LineHeight(FontPixels(Font::Small,m.width)))/2,label,Font::Small,p.secondary,true);
}
inline int DrawHeader(Canvas& c,const Metrics& m,int top,bool back,bool back_selected,bool fastboot,
                      int,int,const std::vector<std::string>& details,const Palette& p,
                      const std::string& page="Recovery",bool dashboard=false) {
  auto b=BackBounds(m,top);
  int brand_x=m.inset;
  if(back) {
    Surface(c,b,b.h/2,p.surface,back_selected,p.primary,Dp(m.width,2));
    DrawIcon(c,Inset(b,Dp(m.width,14)),Icon::Back,p.text);
    brand_x+=b.w+Dp(m.width,12);
  }
  c.Text(brand_x,top+Dp(m.width,13),"uwuAOSP",Font::Menu,p.text,true);
  int y=top+b.h+Dp(m.width,14);
  Font title=dashboard?Font::Title:Font::Heading;
  if(!fastboot && page=="Recovery") {
    // Keep the home title in English in every UI language.
    c.Text(m.inset,y,FitText(page,m.width-2*m.inset,FontPixels(title,m.width),true),title,p.text,true);
  } else {
    Label(c,m,m.inset,y,m.width-2*m.inset,fastboot?"Fastboot":page,title,p.text,true);
  }
  y+=LineHeight(FontPixels(title,m.width))+Dp(m.width,dashboard?8:0);
  if(dashboard) {
    auto info=ReadDeviceInfo(details);
    int x=m.inset;
    if(!info.product.empty()) {
      std::string name=FitText(info.product,Dp(m.width,140),FontPixels(Font::Small,m.width),true);
      Chip(c,m,x,y,name,p);
      x+=TextWidth(name,FontPixels(Font::Small,m.width),true)+Dp(m.width,32);
    }
    if(!info.slot.empty()) Chip(c,m,x,y,Tr("Slot ")+FitText(info.slot,Dp(m.width,40),FontPixels(Font::Small,m.width),true),p);
    y+=Dp(m.width,28);
  }
  return HeaderBottom(m,top,dashboard);
}
inline void DrawBattery(Canvas& c,const Metrics& m,int top,int capacity,bool charging,const Palette& p) {
  std::string value=capacity>=0 && capacity<=100?std::to_string(capacity)+"%":"--%";
  if(charging) value+="+";
  int width=TextWidth(value,FontPixels(Font::Small,m.width),true)+Dp(m.width,24);
  Rect b{m.width-m.inset-width,top+Dp(m.width,9),width,Dp(m.width,30)};
  Rounded(c,b,b.h/2,p.surface);
  c.Text(b.x+Dp(m.width,12),b.y+Dp(m.width,7),value,Font::Small,capacity>=0 && capacity<=15?p.error:p.secondary,true);
}
enum class AlertLevel { Auto, Info, Success, Warning, Error };
inline Color AlertAccent(AlertLevel level,const Palette& p) {
  switch(level) {
    case AlertLevel::Error: return p.error;
    case AlertLevel::Warning: return p.alert_warning;
    case AlertLevel::Success: return p.alert_success;
    default: return p.alert_info;
  }
}
// Classify original prompt headers before translation, never arbitrary filename substrings.
inline AlertLevel PromptLevel(const std::vector<std::string>& lines) {
  AlertLevel level=AlertLevel::Info;
  for(const auto& line:lines) {
    auto start=line.find_first_not_of(" \t");
    if(start==std::string::npos) continue;
    auto text=line.substr(start);
    if(text.rfind("ERROR:",0)==0 || text.rfind("Error:",0)==0 ||
       text.rfind("Can't load Android system.",0)==0 ||
       text=="Signature verification failed" || text=="WARNING: Previous installation has failed.")
      return AlertLevel::Error;
    if(text.rfind("WARNING:",0)==0 || text.rfind("Warning:",0)==0 ||
       text=="Format user data?" || text=="Format cache?" || text=="Format system?" ||
       text=="THIS CANNOT BE UNDONE!" || text=="THIS CAN NOT BE UNDONE!" ||
       text=="This package will downgrade your system" || text=="Overwrite in-progress update?")
      level=AlertLevel::Warning;
    else if(level==AlertLevel::Info &&
            (text.rfind("SUCCESS:",0)==0 || text.rfind("Success:",0)==0))
      level=AlertLevel::Success;
  }
  return level;
}
// Non-interactive inline alert. Returned height drives the real menu viewport and hit targets.
inline int DrawPrompt(Canvas& c,const Metrics& m,int y,const std::vector<std::string>& lines,const Palette& p,
                      AlertLevel level=AlertLevel::Auto) {
  int pad=Dp(m.width,16),vertical_pad=Dp(m.width,14),rail=std::max(1,Dp(m.width,4));
  int font=FontPixels(Font::Body,m.width),lh=LineHeight(font),spacing=Dp(m.width,3);
  int available=m.width-2*m.inset-2*pad-rail;
  std::vector<std::string> rows;
  for(const auto& line:lines) {
    if(line.empty()) continue;
    for(const auto& wrapped:WrapText(Tr(line),available,font,false)) rows.push_back(wrapped);
  }
  if(rows.empty()) return y;
  if(level==AlertLevel::Auto) level=PromptLevel(lines);
  Color accent=AlertAccent(level,p);
  Color background=CompositeOver(accent,p.background,p.alert_opacity);
  Color foreground=p.text;
  int text_height=static_cast<int>(rows.size())*lh+(static_cast<int>(rows.size())-1)*spacing;
  Rect box{m.inset,y,m.width-2*m.inset,2*vertical_pad+text_height};
  int radius=std::min(Dp(m.width,12),std::min(box.w,box.h)/2);
  Rounded(c,box,radius,background);
  // Clip the full-height accent rail to the same rounded outline as the fill.
  c.Fill({box.x,box.y+radius,rail,box.h-2*radius},accent);
  for(int dy=0;dy<radius;++dy) {
    double distance=radius-dy-0.5;
    int dx=static_cast<int>(std::ceil(radius-std::sqrt(radius*radius-distance*distance)-0.5));
    if(dx<rail) {
      c.Fill({box.x+dx,box.y+dy,rail-dx,1},accent);
      c.Fill({box.x+dx,box.y+box.h-1-dy,rail-dx,1},accent);
    }
  }
  int ty=box.y+vertical_pad,tx=box.x+rail+pad;
  for(const auto& row:rows) {
    c.Text(tx,ty,row,Font::Body,foreground,false);
    ty+=lh+spacing;
  }
  return box.y+box.h+Dp(m.width,12);
}
inline void DrawFooter(Canvas& c,const Metrics& m,int y,int bottom,const std::vector<std::string>& details,
                      bool,bool,const std::vector<std::string>& logs,const Palette& p) {
  int small=FontPixels(Font::Small,m.width),lh=LineHeight(small);
  auto info=ReadDeviceInfo(details);
  int version_y=bottom-lh;
  std::string metadata=info.version;
  if(metadata.empty() && !info.extra.empty()) metadata=info.extra.front();
  if(version_y>=y && !metadata.empty()) Label(c,m,m.inset,version_y,m.width-2*m.inset,metadata,Font::Small,p.secondary);
  int box_bottom=version_y-Dp(m.width,16);
  int needed=Dp(m.width,22)+lh*(logs.empty()?1:3);
  if(!logs.empty() && box_bottom-y>=needed) {
    Rect box{m.inset,box_bottom-needed,m.width-2*m.inset,needed};
    Rounded(c,box,Dp(m.width,20),p.surface);
    int x=box.x+Dp(m.width,16),ty=box.y+Dp(m.width,10);
    Label(c,m,x,ty,box.w-Dp(m.width,32),"RECENT OUTPUT",Font::Small,p.primary,true);
    ty+=lh+Dp(m.width,4);
    size_t start=logs.size()>2?logs.size()-2:0;
    for(size_t i=start;i<logs.size();++i) {
      Label(c,m,x,ty,box.w-Dp(m.width,32),logs[i],Font::Small,p.secondary);ty+=lh;
    }
  }
}
} // namespace recovery_m3e

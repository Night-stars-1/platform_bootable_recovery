// SPDX-License-Identifier: Apache-2.0
// Host rendering and geometry checks using the same primitives as ScreenRecoveryUI.
#include "recovery_ui/m3e_install.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <limits>
using namespace recovery_m3e;

#ifdef M3E_INSTALL_ROUTING_TEST
#include "install_routing.inc"
#endif

struct PixelCanvas : Canvas {
  struct Run { Rect bounds; std::string text; Color color; };
  int width,height;bool pixels;std::vector<uint8_t> rgb;std::vector<Run> runs;
  PixelCanvas(int w,int h,bool draw=false):width(w),height(h),pixels(draw),rgb(draw?w*h*3:0) {}
  void Fill(Rect b,Color c) override {
    assert(b.x>=0 && b.y>=0 && b.w>=0 && b.h>=0);
    assert(b.x+b.w<=width && b.y+b.h<=height);
    if(!pixels) return;
    for(int y=b.y;y<b.y+b.h;++y)for(int x=b.x;x<b.x+b.w;++x) {
      size_t i=(y*width+x)*3;rgb[i]=c.r;rgb[i+1]=c.g;rgb[i+2]=c.b;
    }
  }
  void Text(int x,int y,const std::string& s,Font f,Color c,bool bold) override {
    if(s.empty()) return;
    Rect b{x,y,TextWidth(s,FontPixels(f,width),bold),LineHeight(FontPixels(f,width))};
    assert(x>=0 && y>=0 && x+b.w<=width && y+b.h<=height);
    runs.push_back({b,s,c});
    if(!pixels) return;
    auto raster=RasterText(s,FontPixels(f,width),bold);
    for(int sy=0;sy<raster.height;++sy)for(int sx=0;sx<raster.width;++sx) {
      int dx=x+sx,dy=y+sy;if(dx<0||dy<0||dx>=width||dy>=height) continue;
      int a=raster.alpha[sy*raster.width+sx];size_t i=(dy*width+dx)*3;
      rgb[i]=(rgb[i]*(255-a)+c.r*a+127)/255;
      rgb[i+1]=(rgb[i+1]*(255-a)+c.g*a+127)/255;
      rgb[i+2]=(rgb[i+2]*(255-a)+c.b*a+127)/255;
    }
  }
  bool Has(const std::string& s) const {
    return std::any_of(runs.begin(),runs.end(),[&](const Run& r){return r.text==s;});
  }
  void Blit(const std::string& path,Rect b) {
    if(!pixels || !b.w) return;
    std::ifstream f(path,std::ios::binary);std::string magic;int w,h,max;
    f>>magic>>w>>h>>max;f.get();assert(magic=="P6" && max==255 && w==b.w && h==b.h);
    std::vector<uint8_t> source(w*h*3);f.read(reinterpret_cast<char*>(source.data()),source.size());
    assert(f.good());
    for(int y=0;y<h;++y)std::copy_n(source.data()+y*w*3,w*3,rgb.data()+((b.y+y)*width+b.x)*3);
  }
  void Write(const std::string& path) const {
    std::ofstream f(path,std::ios::binary);f<<"P6\n"<<width<<" "<<height<<"\n255\n";
    f.write(reinterpret_cast<const char*>(rgb.data()),rgb.size());assert(f.good());
  }
};
constexpr InstallStage stages[]={InstallStage::WAITING,InstallStage::VERIFYING,
    InstallStage::INSTALLING,InstallStage::SUCCESS,InstallStage::ERROR,InstallStage::CANCELLED};
const char* names[]={"waiting","verifying","installing","success","error","cancelled"};

void Render(const std::string& out,int w,int h,bool zh,int index,bool pixels=false) {
  SetScaleBasis(w,h);SetLanguage(zh?Language::Chinese:Language::English);
  PixelCanvas c(w,h,pixels);Metrics m(w);auto p=Palette::ForMode(false);p.background={0,0,0};
  auto stage=stages[index];int rows=stage==InstallStage::WAITING?1:index>=3?2:0;
  int top=Dp(w,24),bottom=h-top;
  int y=DrawInstallHeader(c,m,top,bottom,rows,false,{},p);
  int logo_width=pixels?800:Dp(w,200),logo_height=pixels?568:Dp(w,142);
  auto layout=InstallationLayout(m,y,bottom,rows,logo_width,logo_height);
  assert(layout.panel.y>=y && layout.panel.y+layout.panel.h<=bottom);
  if(layout.logo.w) {
    assert(layout.logo.x==(w-logo_width)/2 && layout.logo.y==y);
    assert(layout.panel.y==y+logo_height+Dp(w,16));
    if(pixels)c.Blit(out+"/logo.ppm",layout.logo);
  } else {
    assert(layout.panel.y==y); // No empty gap when the bitmap cannot fit.
  }
  auto without_logo=InstallationLayout(m,y,bottom,rows);
  assert(without_logo.logo.w==0 && without_logo.panel.y==y);
  auto oversized=InstallationLayout(m,y,bottom,rows,w+1,h+1);
  assert(oversized.logo.w==0 && oversized.panel.y==y);
  assert(layout.panel.h==without_logo.panel.h); // Artwork cannot reduce readable status space.
  if(w==1220 && h==2712)assert(layout.logo.w>0);
  if(!CompactInstallHeader(m,top,bottom,rows)) {
    assert(c.Has("uwuAOSP"));
  }
  c.runs.clear();
  std::vector<std::string> logs;
  if(stage==InstallStage::VERIFYING) logs={"Verifying update package..."};
  if(stage==InstallStage::INSTALLING) logs={"Installing update...","Step 2/2"};
  if(stage==InstallStage::SUCCESS) logs={"Install completed with status 0."};
  if(stage==InstallStage::ERROR) logs={"signature verification failed","Installation aborted."};
  DrawInstallPanel(c,m,layout.panel,stage,0.42,true,false,logs,p);
  assert(!c.runs.empty()); // A readable result is required even on compact screens.
  for(const auto& run:c.runs) {
    assert(run.bounds.x>=layout.panel.x && run.bounds.y>=layout.panel.y);
    assert(run.bounds.x+run.bounds.w<=layout.panel.x+layout.panel.w);
    assert(run.bounds.y+run.bounds.h<=layout.panel.y+layout.panel.h);
  }
  if(stage==InstallStage::WAITING) assert(c.Has("adb sideload <filename>"));
  if(stage==InstallStage::VERIFYING || stage==InstallStage::INSTALLING) assert(c.Has("42%"));
  else assert(!c.Has("42%"));
  assert(c.Has("100%")== (stage==InstallStage::SUCCESS));
  if(rows) {
    assert(layout.menu_y>=layout.panel.y+layout.panel.h);
    assert(VisibleCount(bottom-layout.menu_y,m.row_height,m.gap)>=rows);
    for(int i=0;i<rows;++i) {
      int row_y=layout.menu_y+i*m.Pitch();
      DrawCard(c,m,row_y,rows==1?"Cancel":i==0?"Continue":"View recovery logs",i==0,false,p);
      assert(HitRow(m,0,rows,0,w/2,row_y+m.row_height/2-layout.menu_y)==i);
    }
  }
  DrawBattery(c,m,top,87,false,p);
  if(pixels)c.Write(out+"/"+names[index]+(zh?"-zh.ppm":"-en.ppm"));
}
void CheckProgress() {
  SetScaleBasis(360,800);SetLanguage(Language::English);Metrics m(360);Palette p;
  for(double value:{-1.0,0.0,0.42,1.0,5.0,std::numeric_limits<double>::quiet_NaN()}) {
    PixelCanvas c(360,800);
    DrawInstallPanel(c,m,{24,24,312,188},InstallStage::INSTALLING,value,true,false,{},p);
    int expected=std::isfinite(value)?static_cast<int>(std::clamp(value,0.0,1.0)*100):0;
    assert(c.Has(std::to_string(expected)+"%"));
  }
  PixelCanvas unknown(360,800);
  DrawInstallPanel(unknown,m,{24,24,312,188},InstallStage::INSTALLING,0.9,false,true,{},p);
  for(const auto& r:unknown.runs)assert(r.text.find('%')==std::string::npos);
  assert(std::string(InstallTitle(InstallStage::INSTALLING,true))=="Installing security update");
  for(auto stage:{InstallStage::ERROR,InstallStage::SUCCESS,InstallStage::CANCELLED}) {
    PixelCanvas c(360,800);DrawInstallPanel(c,m,{24,24,312,188},stage,0.42,true,false,{},p);
    auto expected=stage==InstallStage::ERROR?p.error:stage==InstallStage::SUCCESS?p.alert_success:p.alert_warning;
    assert(c.runs[0].color.r==expected.r && c.runs[0].color.g==expected.g && c.runs[0].color.b==expected.b);
  }
}
int main(int argc,char** argv) {
  assert(argc==2);std::string out=argv[1];
  CheckProgress();
#ifdef M3E_INSTALL_ROUTING_TEST
  CheckRouting();
#endif
  for(auto [w,h]:std::vector<std::pair<int,int>>{{1220,2712},{720,1280},{360,640},
        {320,480},{1280,720},{320,240},{1600,2560}})
    for(bool zh:{false,true})for(int i=0;i<6;++i)Render(out,w,h,zh,i);
  for(bool zh:{false,true})for(int i=0;i<6;++i)Render(out,1220,2712,zh,i,true);
  std::cout<<"PASS: six stages, two languages, seven screen sizes, centered artwork with compact fallback, progress and touch geometry\n";
}

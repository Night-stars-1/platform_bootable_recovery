/*
 * SPDX-FileCopyrightText: The uwuAOSP Project
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once
#include "m3e_design.h"

namespace recovery_m3e::terminal {
struct Key {Rect bounds;std::string label,value;};
inline std::vector<Key> Keyboard(int width,int height,bool symbols,bool shift) {
  int pad=Dp(width,10),gap=Dp(width,3),kh=Dp(width,38),top=height-4*(kh+gap)-Dp(width,10);
  std::vector<Key> keys;
  const std::array<std::string,3> letters{{"qwertyuiop","asdfghjkl","zxcvbnm"}};
  const std::array<std::string,3> punctuation{{"1234567890","/.-_~|&;$","<>*?=()"}};
  const std::array<std::string,3> more{{"\"'`\\[]{}!#","@%:+,^()","$&|;-_/"}};
  for(int row=0;row<3;++row) {
    auto chars=symbols?(shift?more[row]:punctuation[row]):letters[row];int count=chars.size(),kw=(width-2*pad-(count-1)*gap)/count;
    int left=(width-count*kw-(count-1)*gap)/2;
    for(int i=0;i<count;++i) {
      std::string value=chars.substr(i,1);if(!symbols && shift)value[0]=static_cast<char>(value[0]-'a'+'A');
      keys.push_back({{left+i*(kw+gap),top+row*(kh+gap),kw,kh},value,value});
    }
  }
  const std::array<std::string,7> labels{{symbols?"ABC":"123",symbols?(shift?"Less":"More"):(shift?"abc":"ABC"),"Space","Del","Enter","^C","Clear"}};
  int kw=(width-2*pad-6*gap)/7;
  for(int i=0;i<7;++i)keys.push_back({{pad+i*(kw+gap),top+3*(kh+gap),kw,kh},labels[i],{}});
  return keys;
}
inline int HitKey(const std::vector<Key>& keys,int x,int y) {
  for(size_t i=0;i<keys.size();++i)if(InRounded(keys[i].bounds,3,x,y))return i;
  return -1;
}
inline void Draw(Canvas& c,int width,int height,const std::string& input,
                 const std::vector<std::string>& output,bool symbols,bool shift,int focus) {
  Metrics m(width);c.Fill({0,0,width,height},design::background);
  auto back=design::Back(width);Surface(c,back,back.h/2,design::surface,focus==-1,design::text,Dp(width,1));
  design::Symbol(c,Inset(back,Dp(width,7)),design::Glyph::Back);
  Label(c,m,back.x+back.w+Dp(width,14),back.y+Dp(width,3),width-back.x-back.w-Dp(width,30),"Terminal",Font::DesignMenu,design::text);
  auto keys=Keyboard(width,height,symbols,shift);int pad=Dp(width,12),lh=FontLineHeight(Font::Code,width);
  int input_y=keys.front().bounds.y-Dp(width,32),top=back.y+back.h+Dp(width,12);
  std::vector<std::string> rows;
  for(const auto& line:output)for(const auto& row:WrapText(line,width-2*pad,FontPixels(Font::Code,width),false,true))rows.push_back(row);
  int visible=std::max(0,(input_y-top-Dp(width,5))/lh),start=std::max(0,static_cast<int>(rows.size())-visible);
  for(int i=start;i<static_cast<int>(rows.size());++i) {c.Text(pad,top,rows[i],Font::Code,design::text,false);top+=lh;}
  Rounded(c,{pad,input_y,width-2*pad,Dp(width,26)},Dp(width,6),design::surface);
  std::string tail=input;while(!tail.empty() && TextWidth(tail+"_",FontPixels(Font::Code,width),false,true)>width-4*pad)tail.erase(0,1);
  c.Text(2*pad,input_y+Dp(width,5),tail+"_",Font::Code,design::text,false);
  for(size_t i=0;i<keys.size();++i) {
    auto b=keys[i].bounds;Surface(c,b,Dp(width,5),design::surface,focus==static_cast<int>(i),design::green,Dp(width,1));
    auto label=FitText(keys[i].label,b.w-Dp(width,2),FontPixels(Font::Caption,width),false,true);
    int tw=TextWidth(label,FontPixels(Font::Caption,width),false,true);
    c.Text(b.x+(b.w-tw)/2,b.y+(b.h-FontLineHeight(Font::Caption,width))/2,label,Font::Caption,design::text,false);
  }
}
// Consume terminal control characters without exposing escape sequences in the log view.
class Output {
 public:
  void Append(const char* bytes,size_t count) {
    for(size_t i=0;i<count;++i) {
      unsigned char ch=bytes[i];
      if(escape_) {
        if(ch=='[' || ch==']') {sequence_=true;continue;}
        if(!sequence_ || (ch>=0x40 && ch<=0x7e)) {escape_=sequence_=false;}
        continue;
      }
      if(ch==27) {escape_=true;sequence_=false;}
      else if(ch=='\n') {if(lines.back().size()>4096)lines.back().resize(4096);lines.emplace_back();column_=0;}
      else if(ch=='\r')column_=0;
      else if(ch=='\b') {if(column_>0)--column_;}
      else if(ch=='\t') {for(int n=4-column_%4;n>0;--n)Put(' ');}
      else if(ch>=32 && ch!=127)Put(ch);
    }
    if(lines.size()>256)lines.erase(lines.begin(),lines.end()-256);
  }
  void Clear() {lines={""};column_=0;escape_=sequence_=false;}
  std::vector<std::string> lines{""};
 private:
  void Put(char ch) {
    auto& line=lines.back();if(column_>=4096)return;
    if(column_<line.size())line[column_]=ch;else line+=ch;
    ++column_;
  }
  size_t column_=0;bool escape_=false,sequence_=false;
};
} // namespace recovery_m3e::terminal

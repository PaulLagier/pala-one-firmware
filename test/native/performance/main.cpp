#include <U8g2_for_Adafruit_GFX.h>
#include "src/ui/font_render.h"
#include "src/ui/language_fonts.h"
#include "src/pure/paginator.h"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>
uint32_t paginateReference(IReadStream&,uint32_t,const LayoutMetrics&,const MeasureFn&,const LineCallback&);
int main(){
  Adafruit_GFX gfx; U8G2_FOR_ADAFRUIT_GFX f;f.begin(gfx);
  const uint8_t* faces[]={pala_font_sans_r08,pala_font_sans_r10,pala_font_sans_r12,pala_font_sans_r14,pala_font_dys_r08,pala_font_dys_r10,pala_font_dys_r12,pala_font_dys_r14};
  const char* words[]={"Hello,world!", "café", "Größe", "ÄÉÖÜ", "приклад", "їжак", "é", "https://abc.def/xyz", "fij", "łódź", "résumé", "|j)f"};
  unsigned cases=0;size_t refBytes=0,newBytes=0;
  for(auto face:faces){Font::useTransparentFace(f,face);
    auto measure=[&](const char* t){newBytes+=strlen(t);return Font::renderText(f,t,strlen(t));};
    auto oldMeasure=[&](const char* t){refBytes+=strlen(t);return Font::renderText(f,t,strlen(t));};
    for(int width:{30,71,238})for(int seed=0;seed<30;++seed){
      std::string text;for(int i=0;i<120;++i){text+=words[(i*7+seed)%12];text+=(i%23==0?"\n\n":" ");}
      LayoutMetrics m{12,-3,16,width,7};StringReadStream a(text.c_str()),b(text.c_str());uint32_t pos=0;
      while(pos<text.size()){
        std::vector<std::string> oldLines,newLines;
        auto oldNext=paginateReference(a,pos,m,oldMeasure,[&](const char*t,size_t n){oldLines.emplace_back(t,n);});
        auto next=paginatePage(b,pos,m,measure,[&](const char*t,size_t n){newLines.emplace_back(t,n);});
        if(next!=oldNext||oldLines!=newLines){printf("MISMATCH face %u width %d seed %d pos %u old %u new %u\n",cases/90,width,seed,pos,oldNext,next);return 1;}
        assert(next>pos);pos=next;++cases;
      }
    }
  }
  printf("PASS %u real-font page comparisons; measured bytes old=%zu new=%zu reduction=%.1f%%\n",cases,refBytes,newBytes,100.0*(refBytes-newBytes)/refBytes);
}

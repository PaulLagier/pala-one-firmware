#!/usr/bin/env python3
"""Exercise the actual sketch unlock branch against slow-display/Toast lifecycle adapters."""
import argparse,subprocess,tempfile
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--firmware',type=Path,required=True);p.add_argument('--sketch',type=Path);a=p.parse_args()
s=(a.sketch or a.firmware/'Pala_One_2_1/Pala_One_2_1.ino').read_text();start=s.index('if (Lock::isUnlockGesture(ev)) {')+len('if (Lock::isUnlockGesture(ev)) {');depth=1;end=start
while depth:
 if s[end]=='{':depth+=1
 elif s[end]=='}':depth-=1
 end+=1
branch=s[start:end-1]
cpp=r'''
#include <cassert>
#include <cstdint>
#include <cstdio>
static uint32_t now=0,drawTime=0;static int draws=0,readerFull=0,menuFull=0,activity=0;
static bool s_lockedWakePressConsumed=true;
namespace Lock {bool locked=true;void disengage(){locked=false;}}
namespace Toast {bool active=false;uint32_t until=0;void show(const char*){active=true;until=now+650;}void reset(){active=false;until=0;}bool clearIfExpired(){if(active&&int32_t(now-until)>0){reset();return true;}return false;}}
#define D_TOAST_UNLOCKED "Unlocked"
void markUserActivity(){++activity;}
void forceNextRenderFull(){++readerFull;}
void forceNextMenuFrameFull(){++menuFull;}
void setDisplayRefreshMode(bool full){assert(full);}
struct Screen {void draw(){++draws;now+=drawTime;}} screen;
Screen* g_currentScreen=&screen;
void unlock(){BRANCH}
int main(){
 for(uint32_t ms:{0u,300u,1200u,2500u}){
  now=0;drawTime=ms;draws=readerFull=menuFull=activity=0;Lock::locked=true;s_lockedWakePressConsumed=true;
  Toast::show("earlier status");unlock();
  if(Toast::clearIfExpired())screen.draw();
  now+=1000;if(Toast::clearIfExpired())screen.draw();
  assert(!Lock::locked&&!s_lockedWakePressConsumed&&activity==1);
  assert(readerFull==1&&menuFull==1);assert(draws==1);assert(!Toast::active);
 }
 puts("PASS actual unlock branch: 0/300/1200/2500ms display adapters, one redraw only, prior toast cleared, lock/activity and full ghosting refresh preserved");
}
'''.replace('BRANCH',branch).replace('#include <cstdio>','#include <cstdio>\n#include <initializer_list>')
with tempfile.TemporaryDirectory(prefix='pala-unlock-')as t:
 t=Path(t);(t/'test.cpp').write_text(cpp);subprocess.run(['clang++','-std=c++17','-fsanitize=address,undefined',str(t/'test.cpp'),'-o',str(t/'test')],check=True);subprocess.run([str(t/'test')],check=True)

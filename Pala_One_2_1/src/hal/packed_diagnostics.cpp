#include "src/config.h"
#if PALA_DIAGNOSTICS && defined(PALA_COMPANION_TEST)
#include "src/hal/file_stream.h"
#include "src/hal/cpu_burst.h"
#include "src/hal/companion_ble.h"
#include "src/hal/packed_fixture.h"
#include "src/storage/fs_util.h"
#include "src/storage/statistics.h"
#include "src/ui/screens/library_screen.h"
#include "src/ui/screen.h"
#include "src/ui/reader.h"
#include "src/ui/text.h"
#include "src/ui/font.h"
#include "src/hal/display.h"
#include <mbedtls/sha256.h>

void packedBookDeviceTest(){
  if(g_currentScreen!=&g_libraryScreen || g_bookview.book.isOpen() || companionBleRunning()){
    Serial.println("[packed-test] busy");return;
  }
  const char* path="/.pala_format_check.pbt";
  if(FS.exists(path)||fsFreeBytesSafe()<sizeof(kPackedFixture)+8192){Serial.println("[packed-test] unavailable");return;}
  const auto before=Statistics::snapshot();CpuBurst cpu;
  File output=FS.open(path,"w");
  bool written=output && output.write(kPackedFixture,sizeof(kPackedFixture))==sizeof(kPackedFixture);
  output.close();
  File file=FS.open(path,"r");
  bool hashOk=false,seeksOk=false,pagesOk=written&&bool(file),corruptRejected=false;unsigned frames=0;
  if(pagesOk){
    FileReadStream stream(file);mbedtls_sha256_context digest;mbedtls_sha256_init(&digest);
    bool ok=mbedtls_sha256_starts(&digest,0)==0;uint8_t bytes[512],hash[32];size_t total=0;
    while(ok && stream.available()){
      size_t count=0;while(count<sizeof(bytes)){int b=stream.read();if(b<0)break;bytes[count++]=uint8_t(b);}
      if(!count)break;
      ok=mbedtls_sha256_update(&digest,bytes,count)==0;total+=count;yield();
    }
    hashOk=ok&&total==kPackedRawSize&&mbedtls_sha256_finish(&digest,hash)==0;
    if(hashOk)for(size_t i=0;i<32;++i)hashOk=hashOk&&hash[i]==kPackedRawHash[i];
    mbedtls_sha256_free(&digest);
    seeksOk=hashOk;uint32_t state=39;
    for(unsigned i=0;i<512 && seeksOk;++i){state=state*1664525U+1013904223U;const auto pos=state%kPackedRawSize;
      seeksOk=stream.seek(pos)&&stream.position()==pos&&stream.read()==uint8_t(kPackedPattern[pos%(sizeof(kPackedPattern)-1)]);
    }
    for(int family=0;family<2 && pagesOk;++family)for(int size:{8,10,12,14})for(int bionic:{0,1}){
      Font::setDiagnosticLayout(size,Font::Family(family),0,bionic!=0);
      const uint32_t positions[]={0,4032,4160,kPackedRawSize-512};
      for(uint32_t pos:positions){
        while((uint8_t(kPackedPattern[pos%(sizeof(kPackedPattern)-1)])&0xc0)==0x80)++pos;
        clearPagePrefetch();beginPageCanvas();const auto direct=drawPageAt(file,pos);const auto directHash=gfx.captureHash();
        pagesOk=pagesOk&&!lastDrawUsedPrefetch();prefetchPage(file,pos);beginPageCanvas();const auto cached=drawPageAt(file,pos);
        pagesOk=pagesOk&&lastDrawUsedPrefetch()&&direct==cached&&direct>pos&&direct<=kPackedRawSize&&directHash==gfx.captureHash();
        ++frames;yield();
      }
    }
  }
  file.close();clearPagePrefetch();Font::loadSettings();
  if(written){
    File damaged=FS.open(path,"r+");const uint8_t changed=kPackedFixture[sizeof(kPackedFixture)-1]^0xff;
    const bool changedOk=damaged && damaged.seek(sizeof(kPackedFixture)-1)&&damaged.write(&changed,1)==1;damaged.close();
    File candidate=FS.open(path,"r");corruptRejected=changedOk&&candidate&&!validatePackedBook(candidate);candidate.close();
  }
  const bool cleaned=FS.remove(path);const auto after=Statistics::snapshot();
  const bool stats=before.pagesRead==after.pagesRead&&before.buttonPresses==after.buttonPresses&&before.totalSessions==after.totalSessions;
  cpu.restore();g_libraryScreen.draw();
  Serial.printf("[packed-test] raw=%lu stored=%u sha256=%d seeks=%d frames=%u cache=%d corrupt_rejected=%d cleaned=%d statistics=%d pass=%d\n",
    (unsigned long)kPackedRawSize,unsigned(sizeof(kPackedFixture)),hashOk,seeksOk,frames,pagesOk,corruptRejected,cleaned,stats,
    written&&hashOk&&seeksOk&&pagesOk&&corruptRejected&&cleaned&&stats);
}
#endif

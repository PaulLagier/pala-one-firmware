#include "src/lang/plural.h"
#include "src/lang/runtime_strings.h"
#include <string.h>
namespace Language {
// Cardinal rules for the eleven supported locales; Portuguese is pt-PT.
static unsigned form(uint32_t n) {
  const char* c=currentCode(); unsigned ten=n%10, hundred=n%100;
  if(!strcmp(c,"ru")||!strcmp(c,"uk")) {
    if(ten==1&&hundred!=11)return 0;
    return ten>=2&&ten<=4&&(hundred<12||hundred>14)?1:2;
  }
  if(!strcmp(c,"pl")) {
    if(n==1)return 0;
    return ten>=2&&ten<=4&&(hundred<12||hundred>14)?1:2;
  }
  if(!strcmp(c,"cs"))return n==1?0:n>=2&&n<=4?1:2;
  if(!strcmp(c,"fr"))return n<=1?0:2;
  return n==1?0:2;
}
const char* days(uint32_t n) {
  unsigned f=form(n);return f==0?D_STATS_DAY:f==1?D_STATS_DAYS_FEW:D_STATS_DAYS;
}
const char* bookSuffix(uint32_t n) {
  unsigned f=form(n);return f==0?D_WEB_HOME_BOOKS_ONE:f==1?D_WEB_HOME_BOOKS_FEW:D_WEB_HOME_BOOKS_SUFFIX;
}
}

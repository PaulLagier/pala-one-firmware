#include "src/web/web_locale.h"
#include "src/web/chrome.h"
#include "src/lang/runtime_strings.h"
void appendWebLocale(String& out) {
  struct Item { const char* attribute; Language::Key key; };
  static const Item items[]={
  {"no-matches", Language::Key::K_WEB_JS_NO_MATCHES},
  {"match", Language::Key::K_WEB_JS_MATCH},
  {"enter-search", Language::Key::K_WEB_JS_ENTER_SEARCH},
  {"find-first", Language::Key::K_WEB_JS_FIND_FIRST},
  {"saving", Language::Key::K_WEB_JS_SAVING},
  {"saved", Language::Key::K_WEB_JS_SAVED},
  {"save-failed", Language::Key::K_WEB_JS_SAVE_FAILED},
  {"loaded", Language::Key::K_WEB_JS_LOADED},
  {"read-failed", Language::Key::K_WEB_JS_READ_FAILED},
  {"decode-image", Language::Key::K_WEB_JS_DECODE_IMAGE},
  {"read-image", Language::Key::K_WEB_JS_READ_IMAGE},
  {"choose-image", Language::Key::K_WEB_JS_CHOOSE_IMAGE},
  {"preview-pending", Language::Key::K_WEB_JS_PREVIEW_PENDING},
  {"uploading", Language::Key::K_WEB_JS_UPLOADING},
  {"uploaded", Language::Key::K_WEB_JS_UPLOADED},
  {"upload-failed", Language::Key::K_WEB_JS_UPLOAD_FAILED},
  {"no-image",Language::Key::K_WEB_SS_NO_IMAGE},
  };
  out+="<div id='palaLocale' hidden";
  for(const auto& item:items) {
    out+=" data-";out+=item.attribute;out+="='";
    out+=htmlEscape(Language::text(item.key));out+="'";
  }
  out+="></div>";
}

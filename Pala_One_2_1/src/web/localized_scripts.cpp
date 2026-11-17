#include "src/web/localized_scripts.h"
#include "src/lang/lang.h"
#include "src/pure/web_script.h"
#include "src/web/web_assets.h"
#include <initializer_list>

namespace {
void appendLocalizedScript(String &out, const char *name,
                           std::initializer_list<const char *> labels, WebAssets::Id id) {
  out += "<script>window.";
  out += name;
  out += "=[";
  bool first = true;
  for (const char *label : labels) {
    if (!first)
      out += ",";
    first = false;
    out += webJsString(label);
  }
  out += "];</script>";
  WebAssets::appendScript(out, id);
}
} // namespace

void appendReadScript(String &out) {
  appendLocalizedScript(out, "palaReaderLabels",
                        {D_WEB_READ_JS_NO_MATCHES, D_WEB_READ_JS_MATCH_FMT,
                         D_WEB_READ_JS_ENTER_PHRASE, D_WEB_READ_JS_FIND_FIRST, D_WEB_READ_JS_SAVING,
                         D_WEB_READ_JS_SAVED_FMT, D_WEB_READ_JS_SAVE_HTTP_FMT,
                         D_WEB_READ_JS_SAVE_FAIL_FMT, D_WEB_READ_JS_ERROR, D_WEB_READ_JS_LOADED_FMT,
                         D_WEB_READ_JS_LOAD_FAILED},
                        WebAssets::Id::ReaderJs);
}

void appendEditorScript(String &out) {
  appendLocalizedScript(out, "palaEditorLabels",
                        {D_WEB_SS_JS_PX_SUFFIX, D_WEB_SS_NO_IMAGE, D_WEB_SS_JS_PREVIEW_FMT,
                         D_WEB_SS_JS_DECODE_FAILED, D_WEB_SS_JS_READ_FAILED,
                         D_WEB_SS_ERR_CHOOSE_IMAGE, D_WEB_SS_JS_PREVIEW_NOT_READY,
                         D_WEB_SS_JS_UPLOADING, D_WEB_SS_JS_HTTP_FMT, D_WEB_SS_JS_UPLOAD_COMPLETE,
                         D_WEB_SS_JS_UPLOAD_FAIL_FMT, D_WEB_SS_JS_ERROR},
                        WebAssets::Id::EditorJs);
}

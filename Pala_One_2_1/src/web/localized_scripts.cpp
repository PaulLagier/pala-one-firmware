#include "src/web/localized_scripts.h"
#include "src/lang/lang.h"
#include "src/pure/web_script.h"
#include "src/web/web_assets.h"

// Retain main #128 typography folding and UTF-8 offsets with runtime-localized labels.
void appendReadScript(String &out) {
  out += "<script>window.palaReaderLabels=[";
  out += webJsString(D_WEB_READ_JS_NO_MATCHES);
  out += ",";
  out += webJsString(D_WEB_READ_JS_MATCH_FMT);
  out += ",";
  out += webJsString(D_WEB_READ_JS_ENTER_PHRASE);
  out += ",";
  out += webJsString(D_WEB_READ_JS_FIND_FIRST);
  out += ",";
  out += webJsString(D_WEB_READ_JS_SAVING);
  out += ",";
  out += webJsString(D_WEB_READ_JS_SAVED_FMT);
  out += ",";
  out += webJsString(D_WEB_READ_JS_SAVE_HTTP_FMT);
  out += ",";
  out += webJsString(D_WEB_READ_JS_SAVE_FAIL_FMT);
  out += ",";
  out += webJsString(D_WEB_READ_JS_ERROR);
  out += ",";
  out += webJsString(D_WEB_READ_JS_LOADED_FMT);
  out += ",";
  out += webJsString(D_WEB_READ_JS_LOAD_FAILED);
  out += "];</script>";
  WebAssets::appendScript(out, WebAssets::Id::ReaderJs);
}

void appendEditorScript(String &out) {
  out += "<script>window.palaEditorLabels=[";
  out += webJsString(D_WEB_SS_JS_PX_SUFFIX);
  out += ",";
  out += webJsString(D_WEB_SS_NO_IMAGE);
  out += ",";
  out += webJsString(D_WEB_SS_JS_PREVIEW_FMT);
  out += ",";
  out += webJsString(D_WEB_SS_JS_DECODE_FAILED);
  out += ",";
  out += webJsString(D_WEB_SS_JS_READ_FAILED);
  out += ",";
  out += webJsString(D_WEB_SS_ERR_CHOOSE_IMAGE);
  out += ",";
  out += webJsString(D_WEB_SS_JS_PREVIEW_NOT_READY);
  out += ",";
  out += webJsString(D_WEB_SS_JS_UPLOADING);
  out += ",";
  out += webJsString(D_WEB_SS_JS_HTTP_FMT);
  out += ",";
  out += webJsString(D_WEB_SS_JS_UPLOAD_COMPLETE);
  out += ",";
  out += webJsString(D_WEB_SS_JS_UPLOAD_FAIL_FMT);
  out += ",";
  out += webJsString(D_WEB_SS_JS_ERROR);
  out += "];</script>";
  WebAssets::appendScript(out, WebAssets::Id::EditorJs);
}

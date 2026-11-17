#include "src/web/reading_settings.h"
#include "src/state.h"
#include "src/ui/font.h"
#include "src/ui/sleep.h"

bool applyReadingSettingsForm() {
  // Other cards omit reading checkboxes. Absence there must not disable
  // bionic text or paragraph gaps when saving buttons, Wi-Fi or header icons.
  if (!server.hasArg("font") && !server.hasArg("family") && !server.hasArg("lgap") &&
      !server.hasArg("sleep") && !server.hasArg("bionic") && !server.hasArg("hgap"))
    return false;
  bool layoutChanged = false;

  // Font::setBodySize / setLineGap / setFamily / Sleep::setIdleTimeout all
  // clamp + validate internally — no inline guards needed here.
  if (server.hasArg("font")) {
    int fs = server.arg("font").toInt();
    if (fs != Font::currentBodySize()) {
      Font::setBodySize(fs);
      layoutChanged = true;
    }
  }
  if (server.hasArg("family")) {
    String f = server.arg("family");
    Font::Family want = (f == "dys") ? Font::Family::OpenDyslexic : Font::Family::Helvetica;
    if (want != Font::currentFamily()) {
      Font::setFamily(want);
      layoutChanged = true;
    }
  }
  if (server.hasArg("sleep")) {
    int ss = server.arg("sleep").toInt();
    if (ss != Sleep::idleTimeoutSecs())
      Sleep::setIdleTimeout(ss);
  }
  if (server.hasArg("lgap")) {
    int lg = server.arg("lgap").toInt();
    if (lg != Font::currentLineGap()) {
      Font::setLineGap(lg);
      layoutChanged = true;
    }
  }
  // Checkbox is absent from the POST when unchecked.
  bool wantBionic = server.hasArg("bionic");
  if (wantBionic != Font::bionicEnabled()) {
    Font::setBionic(wantBionic);
    layoutChanged = true;
  }
  bool wantHalfGaps = server.hasArg("hgap");
  if (wantHalfGaps != Font::halfParagraphGapsEnabled()) {
    Font::setHalfParagraphGaps(wantHalfGaps);
    layoutChanged = true;
  }
  return layoutChanged;
}

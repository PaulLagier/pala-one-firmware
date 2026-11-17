#include "src/web/settings_controls.h"
namespace {
void appendSelectOption(String &out, const char *value, const char *label, bool selected) {
  out += "<option value='";
  out += value;
  out += "'";
  if (selected)
    out += " selected";
  out += ">";
  out += label;
  out += "</option>";
}
} // namespace
void appendSettingsSelect(String &out, const char *name, const char *label, const char *hint,
                          std::initializer_list<SettingsChoice> choices, const String &current,
                          const String &currentLabel) {
  out += "<div><label for='";
  out += name;
  out += "'>";
  out += label;
  out += "</label><select id='";
  out += name;
  out += "' name='";
  out += name;
  out += "'>";
  bool found = false;
  for (const auto &choice : choices) {
    const bool selected = current == choice.value;
    found = found || selected;
    appendSelectOption(out, choice.value, choice.label, selected);
  }
  if (!found && currentLabel.length())
    appendSelectOption(out, current.c_str(), currentLabel.c_str(), true);
  out += "</select>";
  if (hint && *hint) {
    out += "<div class='hint'>";
    out += hint;
    out += "</div>";
  }
  out += "</div>";
}

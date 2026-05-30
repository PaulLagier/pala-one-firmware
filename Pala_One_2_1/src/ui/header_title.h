#ifndef PALA_UI_HEADER_TITLE_H
#define PALA_UI_HEADER_TITLE_H

// ============================================================================
//  HeaderTitle module — owns the user-configurable library screen header
//  title (NVS key `cfg_hdr_title`, default: compile-time LIB_HEADER_TITLE,
//  max 31 chars). An empty string means "show no header text."
// ============================================================================
namespace HeaderTitle {

void loadSettings();

const char* current();

void set(const char* title);

void resetToDefault();

}  // namespace HeaderTitle

#endif  // PALA_UI_HEADER_TITLE_H

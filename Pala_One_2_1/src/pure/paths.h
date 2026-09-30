#ifndef PALA_PURE_PATHS_H
#define PALA_PURE_PATHS_H

#include "arduino_compat.h"

// Pure path / folder / filename utilities. No hardware deps.

// ".txt" extension stripping. "book.txt" -> "book", "book" -> "book".
String stripTxtExt(const String& s);

// Last "/" component. "/a/b/c" -> "c", "c" -> "c".
String lastPathComponent(const String& path);

// Parent folder of a relative path. "a/b/c" -> "a/b", "c" -> "".
String folderParent(const String& relPath);

// Replace '_' with ' ' and '/' with ' / ', strip ".txt". For UI display.
String prettyRelativeLabel(const String& relPath);

// Last component with '_' -> ' '. For UI display.
String folderLeafLabel(const String& relPath);

// Display label for a book path: strip the leading folders, strip the
// ".txt" suffix, replace '_' with ' '.
String bookLeafLabel(const String& path);

// Byte-level character set used by sanitizeFolderSegment.
bool isAllowedFolderByte(uint8_t c);

// Replace disallowed bytes with '_', trim whitespace.
String sanitizeFolderSegment(const String& segment);

// Normalize separators, drop "." and "..", sanitize each segment, rejoin with '/'.
// Returns "" if every segment is empty or invalid.
String sanitizeFolderInput(const String& raw);

// Sanitize a user-uploaded filename. Removes path components, restricts to a
// safe byte set, kills repeated "..", ensures ".txt" suffix, falls back to
// "book.txt" if empty.
String sanitizeUploadedFilename(String fname);

// Sibling of `sanitizeUploadedFilename` for app .bin uploads. Removes path
// components, restricts to a safe byte set, kills repeated "..", strips
// every extension (so `my.app.bin` becomes `my_app.bin` after byte
// filtering — single extension), and ensures `.bin` suffix. Falls back to
// `"app.bin"` if empty.
String sanitizeUploadedAppFilename(String fname);

#endif  // PALA_PURE_PATHS_H

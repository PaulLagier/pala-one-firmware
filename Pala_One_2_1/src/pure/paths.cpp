#include "paths.h"

String stripTxtExt(const String& s) {
  return s.endsWith(".txt") ? s.substring(0, s.length() - 4) : s;
}

String lastPathComponent(const String& path) {
  int slash = path.lastIndexOf('/');
  return (slash >= 0) ? path.substring(slash + 1) : path;
}

String folderParent(const String& relPath) {
  int slash = relPath.lastIndexOf('/');
  return (slash < 0) ? String("") : relPath.substring(0, slash);
}

String prettyRelativeLabel(const String& relPath) {
  String out;
  out.reserve(relPath.length() + 8);
  for (size_t i = 0; i < relPath.length(); i++) {
    char c = relPath[i];
    if (c == '_') out += ' ';
    else if (c == '/') out += " / ";
    else out += c;
  }
  return stripTxtExt(out);
}

String folderLeafLabel(const String& relPath) {
  String leaf = lastPathComponent(relPath);
  leaf.replace('_', ' ');
  return leaf;
}

String bookLeafLabel(const String& path) {
  String leaf = stripTxtExt(lastPathComponent(path));
  leaf.replace('_', ' ');
  return leaf;
}

bool isAllowedFolderByte(uint8_t c) {
  return ((c >= 'a' && c <= 'z') ||
          (c >= 'A' && c <= 'Z') ||
          (c >= '0' && c <= '9') ||
          c == '_' || c == '-' || c == ' ' ||
          c >= 128);
}

String sanitizeFolderSegment(const String& segment) {
  String clean;
  clean.reserve(segment.length());
  for (size_t i = 0; i < segment.length(); i++) {
    uint8_t c = (uint8_t)segment[i];
    clean += isAllowedFolderByte(c) ? (char)c : '_';
  }
  clean.trim();
  return clean;
}

String sanitizeFolderInput(const String& raw) {
  String normalized = raw;
  normalized.replace('\\', '/');

  String out;
  int start = 0;
  while (start <= (int)normalized.length()) {
    int slash = normalized.indexOf('/', start);
    String part = (slash >= 0) ? normalized.substring(start, slash)
                               : normalized.substring(start);
    start = (slash >= 0) ? slash + 1 : (int)normalized.length() + 1;

    part.trim();
    if (part.length() == 0 || part == "." || part == "..") continue;
    String clean = sanitizeFolderSegment(part);
    if (clean.length() == 0) continue;
    if (out.length() > 0) out += '/';
    out += clean;
  }
  return out;
}

String sanitizeUploadedFilename(String fname) {
  int slash = fname.lastIndexOf('/');
  if (slash >= 0) fname = fname.substring(slash + 1);

  String clean;
  clean.reserve(fname.length());
  for (size_t i = 0; i < fname.length(); i++) {
    uint8_t c = (uint8_t)fname[i];
    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '_' || c == '-' || c == ' ' || c == '.' ||
        c >= 128) {
      clean += (char)c;
    } else {
      clean += '_';
    }
  }

  clean.replace("..", "");
  while (clean.startsWith(".")) clean.remove(0, 1);
  if (!clean.endsWith(".txt")) clean += ".txt";
  if (clean.length() == 0) clean = "book.txt";
  return clean;
}

String sanitizeUploadedAppFilename(String fname) {
  int slash = fname.lastIndexOf('/');
  if (slash >= 0) fname = fname.substring(slash + 1);
  int back  = fname.lastIndexOf('\\');
  if (back >= 0)  fname = fname.substring(back + 1);

  // Strip all extensions so "foo.tar.bin" → "foo" (then we re-add .bin
  // below). Belt-and-braces against double-extension confusion.
  int dot = fname.lastIndexOf('.');
  while (dot > 0) { fname = fname.substring(0, dot); dot = fname.lastIndexOf('.'); }

  String clean;
  clean.reserve(fname.length() + 4);
  for (size_t i = 0; i < fname.length(); i++) {
    uint8_t c = (uint8_t)fname[i];
    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9') ||
        c == '_' || c == '-') {
      clean += (char)c;
    } else {
      clean += '_';
    }
  }

  clean.replace("..", "");
  while (clean.startsWith(".")) clean.remove(0, 1);
  if (clean.length() == 0) clean = "app";
  clean += ".bin";
  return clean;
}

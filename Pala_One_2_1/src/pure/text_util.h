#ifndef PALA_PURE_TEXT_UTIL_H
#define PALA_PURE_TEXT_UTIL_H

#include "arduino_compat.h"

// Pure UTF-8 helpers and typography normalization. No hardware deps.

// True for any byte of the form 10xxxxxx (UTF-8 continuation byte).
inline bool isUtf8ContinuationByte(uint8_t b) {
  return (b & 0xC0) == 0x80;
}

// Returns the UTF-8 character byte-length encoded by `b` (the leading byte).
// Falls back to 1 for malformed bytes.
int utf8CharLenFromLead(uint8_t b);

// Returns the UTF-8 character byte-length at `index` in `s`. If `index` is
// out of range or the encoded character would run past the string end, falls
// back to 1 (caller can advance one byte and try again).
int utf8SafeCharLenAt(const String& s, int index);

// Buffer-based variant for the paginator's hot path. `len` is the live byte
// length of `buf`. Returns 0 if `index >= len`, 1 for malformed sequences
// or sequences that would run past `len`, otherwise the encoded length.
int utf8SafeCharLenAt(const char* buf, size_t len, size_t index);

// Returns the UTF-8 character at `index` (as a String), or empty if out of range.
String utf8CharAt(const String& s, int index);

// Truncate `in` so that it fits `maxWidth` under the caller's `measure`
// function, appending "..." when anything was dropped. Never splits a UTF-8
// sequence, and right-trims spaces before the ellipsis so labels don't read
// "Foo ...". Returns `in` unchanged when it already fits, and "" when even
// the ellipsis alone doesn't fit.
//
// `measure` is a plain function pointer rather than std::function to keep
// this layer free of <functional> — callers on the device pass a wrapper
// around u8g2.getUTF8Width, host tests pass a fixed-width stub.
//
// The ellipsis is three ASCII dots, not U+2026: `normalizeTypography` below
// already converts U+2026 to "..." as the project convention, and the small
// UI fonts are ASCII-only (see src/ui/font.h).
String truncateWithEllipsis(const String& in, int maxWidth, int (*measure)(const char*));

bool isBreakableWhitespaceByte(char b);
bool isBreakablePunctuationByte(char b);

// Normalize typography: strip BOM, NBSP -> space, smart quotes -> ASCII
// quotes, em/en dashes -> '-', ellipsis -> "...". Preserves all other bytes.
String normalizeTypography(const String& in);

// Compact text for storage: collapse runs of spaces, normalize line endings,
// limit consecutive newlines to 2, strip trailing whitespace.
String compactText(const String& in);

// Streaming overload. Carries `lastWasSpace` and `newlineCount` across calls
// so whitespace runs and newline runs that span chunk boundaries collapse
// correctly. Pass `trimTail=false` on every chunk except the final one to
// keep trailing whitespace from being chopped between chunks. State pointers
// must be non-null; initialize *ioLastWasSpace to false and *ioNewlineCount
// to 0 before the first call.
//
// Note: the in-chunk "strip trailing spaces before a newline" behavior does
// NOT carry across a chunk boundary — a chunk ending in spaces followed by
// a chunk starting with a newline will keep those spaces. PR #10 against
// PaulLagier/main accepted this trade-off; the alternative (file-truncate
// from the writer side) is not worth the complexity.
String compactText(const String& in,
                   bool* ioLastWasSpace,
                   int* ioNewlineCount,
                   bool trimTail);

#endif  // PALA_PURE_TEXT_UTIL_H

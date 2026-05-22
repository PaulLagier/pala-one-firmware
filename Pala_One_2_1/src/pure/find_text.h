#ifndef PALA_PURE_FIND_TEXT_H
#define PALA_PURE_FIND_TEXT_H

#include "arduino_compat.h"
#include "stream.h"

// Search for the first occurrence of `pattern` inside `text`.
// Uses the Boyer-Moore-Horspool variant to skip ahead by the
// last-character shift when possible.
// Returns the zero-based byte index of the match, or 0xFFFFFFFFu if not found.
uint32_t find_text(const String& text, const String& pattern);

// Stream `in` in `chunkSize`-byte windows, keeping the last (patLen-1) bytes
// of each window as overlap for the next so matches that straddle a chunk
// boundary are not missed. Returns the absolute byte offset of the first
// occurrence of `pattern`, or 0xFFFFFFFFu if not found.
uint32_t findByteOffset(IReadStream& in, const String& pattern,
                        size_t chunkSize = 512);

#endif // PALA_PURE_FIND_TEXT_H
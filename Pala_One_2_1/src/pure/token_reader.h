#ifndef PALA_PURE_TOKEN_READER_H
#define PALA_PURE_TOKEN_READER_H

#include "arduino_compat.h"
#include "stream.h"

// What kind of byte (or condition) terminated the token that was just read.
// The paginator uses this to decide whether to start a new line, whether to
// add an inter-word space, and whether to keep the punctuation byte joined
// to the preceding letters.
enum class TokenEnd {
  // ' ' or '\t' separator. Caller typically adds an inter-word space after
  // the token's bytes when laying out a line.
  Space,
  // .,;:!?)]}-/' (per isBreakablePunctuationByte). The punctuation byte is
  // INCLUDED in the token (it's the last byte of `buf[0..len]`). No inter-
  // word space follows.
  Punctuation,
  // '\n' separator. Forces a hard line break for the caller.
  Newline,
  // Stream reached EOF before any separator or punctuation was encountered.
  Eof,
  // Token buffer ran out of room before a natural separator was hit. The
  // byte that would have overflowed was unread (the stream's read position
  // is just before it), so a subsequent call to readNextToken continues
  // the same logical run-of-bytes as a fresh token.
  BufferFull,
};

// One unit of output from readNextToken.
struct ScannedToken {
  // Absolute byte offset in the stream at which this token's first byte
  // sits. For empty tokens (Space immediately following another Space, or
  // a leading Newline) this is the position where the token *would* have
  // started — typically the position of the separator that produced it.
  uint32_t startPos;
  // Number of bytes written into the caller's buffer (not counting NUL).
  size_t   len;
  // What terminated the token. See TokenEnd above.
  TokenEnd end;
};

// Read one token from `in` into `buf` (capacity `cap`, must be >= 1; one byte
// is reserved for the NUL terminator).
//
// On success the function fills `out` and writes 0 to `buf[out.len]`. Empty
// tokens are valid output — they represent runs of separators (e.g. ` \n`
// produces an empty Space token followed by an empty Newline token).
//
// Returns true if any bytes were consumed from the stream (a token, even
// empty, was logically produced). Returns false only when called at EOF
// with nothing left to read; in that case `out.end == TokenEnd::Eof` and
// `out.len == 0`.
bool readNextToken(IReadStream& in, char* buf, size_t cap, ScannedToken& out);

#endif  // PALA_PURE_TOKEN_READER_H

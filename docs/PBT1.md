# PBT1 packed text books

PBT1 stores the exact UTF-8 text bytes in independently seekable 4096-byte
blocks. Logical byte offsets refer to the decoded text, so pagination and
bookmarks can use the same byte positions as a plain-text source. No text or
newline normalization is performed by the container encoder.

All integers are unsigned little-endian 32-bit values.

| Offset | Field |
| --- | --- |
| 0 | ASCII magic `PBT1` |
| 4 | Decoded byte count, 1 through 2097152 |
| 8 | Block size, exactly 4096 |
| 12 | Block count, `ceil(decodedBytes / 4096)` |
| 16 | `blockCount + 1` absolute physical offsets |

The first offset is exactly the end of the offset table. Each next offset
must be greater than the previous offset plus its eight-byte block header,
with at most 4096 payload bytes. The final offset equals physical EOF;
trailing data is rejected. At most 512 blocks are permitted.

Each block starts with a stored-length word and the CRC-32 of its decoded
bytes. Bit 31 of the length means uncompressed bytes; the low 31 bits give
the stored payload length. Compressed blocks use a complete zlib-wrapped
DEFLATE stream. The decoded output is exactly 4096 bytes except for the final
block, which uses the remaining logical byte count. CRC-32 uses the standard
reflected polynomial 0xEDB88320, with the usual initial/final complement.

The maximum physical file size is 2103316 bytes (raw blocks plus headers and
offset table). Consumers must distinguish physical upload/storage sizes from
logical text positions. A compressed block must consume all its input and
produce exactly its expected bounded output; CRC mismatch rejects it.

A reader should keep its block stream alive across sequential page scans,
cache at most one decoded block, and invalidate it when the source closes or
changes. Packed files use `.pbt`; ordinary `.txt` books retain their behavior.

Create a book with `python3 scripts/pack_book.py source.txt [destination.pbt]`.
Python 3.8+ is sufficient; no third-party package is required. Existing output
files (including the input itself) are never overwritten. The CLI validates
UTF-8 and preserves every source byte, including CRLF and whitespace. Level 0
uses raw blocks; the default level 9 chooses compressed blocks when smaller.

Upload `.pbt` through the existing web book-upload form. Packed bytes bypass
plain-text typography/whitespace normalization. Before promotion, the firmware
validates every block and CRC, then renames the staging file atomically; a
failed validation or rename retains any previous book. New uploads invalidate
the old page-offset cache. Download returns the original packed bytes, whereas
the browser reader/search endpoint decodes text incrementally.

The active reader retains one stream across rendering and indexing, so both
packed blocks and the existing plain-text read-ahead buffer remain cached.
Reading progress, bookmark offsets and page-cache size stamps refer to decoded
text, while library/file-manager sizes report actual storage usage. Existing
`.txt` files remain supported. Existing page-count/bookmark limits still apply;
this format does not increase those limits. Migrating `.txt` to a new `.pbt`
path creates a separate book identity; existing path-keyed progress/bookmarks
are not moved automatically.

RAM use is two 4096-byte block buffers plus a temporary heap-allocated SDK
miniz decoder state while decoding. Allocation failure or invalid data stops
the stream. Public flash partition layouts are unchanged.

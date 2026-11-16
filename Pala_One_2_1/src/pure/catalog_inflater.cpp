#include "src/pure/catalog_inflater.h"
#include <new>
#include <setjmp.h>
namespace CatalogDeflate {
#include "src/third_party/puff/puff_impl.h"
}
#undef local
#undef NIL
#undef MAXBITS
#undef MAXLCODES
#undef MAXDCODES
#undef MAXCODES
#undef FIXLCODES
struct CatalogInflater::Workspace {
  CatalogDeflate::state state;
};
static_assert(sizeof(CatalogDeflate::state) <= 2048,
              "Dictionary decoder workspace must remain <=2KB");
CatalogInflater::CatalogInflater() : workspace_(new(std::nothrow) Workspace) {}
CatalogInflater::~CatalogInflater() = default;
size_t CatalogInflater::workspaceBytes() { return sizeof(Workspace); }
bool CatalogInflater::inflate(const uint8_t *in, size_t size, uint8_t *out, size_t capacity) {
  if (!workspace_ || !in || !out || size < 6 || !capacity)
    return false;
  const unsigned header = (unsigned(in[0]) << 8) | in[1];
  if ((in[0] & 15) != 8 || (in[0] >> 4) > 7 || header % 31 || (in[1] & 32))
    return false;
  unsigned long read = size - 6, written = capacity;
  if (CatalogDeflate::puffWorkspace(&workspace_->state, out, &written, in + 2, &read) != 0 ||
      read != size - 6 || written != capacity)
    return false;
  uint32_t a = 1, b = 0;
  for (size_t i = 0; i < capacity; ++i) {
    a = (a + out[i]) % 65521;
    b = (b + a) % 65521;
  }
  const uint32_t adler = (uint32_t(in[size - 4]) << 24) | (uint32_t(in[size - 3]) << 16) |
                         (uint32_t(in[size - 2]) << 8) | in[size - 1];
  return ((b << 16) | a) == adler;
}

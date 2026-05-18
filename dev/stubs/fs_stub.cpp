#include "stubs/fs_stub.h"

#include <filesystem>
#include <cstring>
#include <sys/statvfs.h>

namespace stdfs = std::filesystem;

LittleFSClass LittleFS;

// ---------------------------------------------------------------------------
//  LittleFSClass
// ---------------------------------------------------------------------------

void LittleFSClass::setRoot(const std::string& root) {
  root_ = root;
  std::error_code ec;
  stdfs::create_directories(root_, ec);
}

std::string LittleFSClass::hostPath(const std::string& virtPath) const {
  // virtPath is always absolute ("/books/foo.txt"); strip leading slash then join.
  if (!virtPath.empty() && virtPath[0] == '/') return root_ + virtPath;
  return root_ + "/" + virtPath;
}

File LittleFSClass::open(const char* virtPath, const char* mode) {
  std::string hp = hostPath(virtPath);
  std::error_code ec;

  if (!stdfs::exists(hp, ec)) return File();

  File f;
  f.valid_     = true;
  f.virt_path_ = virtPath;
  f.is_dir_    = stdfs::is_directory(hp, ec);

  if (f.is_dir_) {
    for (auto& entry : stdfs::directory_iterator(hp, ec)) {
      // Build virtual child path.
      std::string child = f.virt_path_;
      if (child.back() != '/') child += '/';
      child += entry.path().filename().string();
      f.children_.push_back(child);
    }
    std::sort(f.children_.begin(), f.children_.end());
  } else {
    const char* fmode = (mode && mode[0] == 'w') ? "wb" : "rb";
    f.fp_ = fopen(hp.c_str(), fmode);
    if (!f.fp_) f.valid_ = false;
  }
  return f;
}

bool LittleFSClass::exists(const char* virtPath) {
  std::error_code ec;
  return stdfs::exists(hostPath(virtPath), ec);
}

bool LittleFSClass::remove(const char* virtPath) {
  std::error_code ec;
  return stdfs::remove(hostPath(virtPath), ec);
}

bool LittleFSClass::rename(const char* from, const char* to) {
  std::error_code ec;
  stdfs::rename(hostPath(from), hostPath(to), ec);
  return !ec;
}

bool LittleFSClass::mkdir(const char* virtPath) {
  std::error_code ec;
  stdfs::create_directories(hostPath(virtPath), ec);
  return !ec;
}

bool LittleFSClass::rmdir(const char* virtPath) {
  std::error_code ec;
  stdfs::remove(hostPath(virtPath), ec);
  return !ec;
}

size_t LittleFSClass::usedBytes() const {
  size_t total = 0;
  std::error_code ec;
  for (auto& entry : stdfs::recursive_directory_iterator(root_, ec)) {
    if (!entry.is_directory(ec)) total += entry.file_size(ec);
  }
  return total;
}

// ---------------------------------------------------------------------------
//  File
// ---------------------------------------------------------------------------

File::File(File&& o) noexcept
    : valid_(o.valid_), is_dir_(o.is_dir_), virt_path_(std::move(o.virt_path_)),
      fp_(o.fp_), children_(std::move(o.children_)), child_idx_(o.child_idx_) {
  o.valid_ = false;
  o.fp_    = nullptr;
}

File& File::operator=(File&& o) noexcept {
  if (this != &o) {
    close();
    valid_     = o.valid_;
    is_dir_    = o.is_dir_;
    virt_path_ = std::move(o.virt_path_);
    fp_        = o.fp_;
    children_  = std::move(o.children_);
    child_idx_ = o.child_idx_;
    o.valid_   = false;
    o.fp_      = nullptr;
  }
  return *this;
}

void File::close() {
  if (fp_) { fclose(fp_); fp_ = nullptr; }
  valid_ = false;
}

size_t File::size() const {
  if (!fp_) return 0;
  long cur = ftell(fp_);
  fseek(fp_, 0, SEEK_END);
  long end = ftell(fp_);
  fseek(fp_, cur, SEEK_SET);
  return (size_t)(end < 0 ? 0 : end);
}

int File::read() {
  if (!fp_) return -1;
  int c = fgetc(fp_);
  return (c == EOF) ? -1 : c;
}

size_t File::read(uint8_t* buf, size_t len) {
  if (!fp_) return 0;
  return fread(buf, 1, len, fp_);
}

bool File::seek(uint32_t pos) {
  if (!fp_) return false;
  return fseek(fp_, (long)pos, SEEK_SET) == 0;
}

uint32_t File::position() const {
  if (!fp_) return 0;
  long p = ftell(fp_);
  return (p < 0) ? 0 : (uint32_t)p;
}

int File::available() const {
  if (!fp_) return 0;
  long cur = ftell(fp_);
  fseek(fp_, 0, SEEK_END);
  long end = ftell(fp_);
  fseek(fp_, cur, SEEK_SET);
  return (int)(end - cur);
}

size_t File::write(const uint8_t* buf, size_t len) {
  if (!fp_) return 0;
  return fwrite(buf, 1, len, fp_);
}

File File::openNextFile() {
  if (!is_dir_ || child_idx_ >= children_.size()) return File();
  return LittleFS.open(children_[child_idx_++].c_str());
}

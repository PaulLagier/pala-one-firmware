#pragma once

#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <algorithm>

// Forward declaration for openNextFile() which calls back into LittleFSClass.
class LittleFSClass;
extern LittleFSClass LittleFS;

// Mirrors the Arduino File API used by the firmware.
class File {
public:
  File() = default;
  ~File() { close(); }

  // Non-copyable, movable.
  File(const File&)            = delete;
  File& operator=(const File&) = delete;
  File(File&& o) noexcept;
  File& operator=(File&& o) noexcept;

  explicit operator bool() const { return valid_; }
  bool isDirectory() const { return is_dir_; }
  const char* name() const { return virt_path_.c_str(); }
  size_t size() const;

  // Reading
  int      read();
  size_t   read(uint8_t* buf, size_t len);
  bool     seek(uint32_t pos);
  uint32_t position() const;
  int      available() const;

  // Writing
  size_t write(const uint8_t* buf, size_t len);

  // Directory iteration
  File openNextFile();

  void close();

private:
  friend class LittleFSClass;

  bool        valid_     = false;
  bool        is_dir_    = false;
  std::string virt_path_;

  // Regular file
  FILE*       fp_        = nullptr;

  // Directory: sorted virtual paths of direct children
  std::vector<std::string> children_;
  size_t                   child_idx_ = 0;
};

// Mirrors the subset of the LittleFS API used by the firmware.
class LittleFSClass {
public:
  // Call once at startup before any FS operations.
  void setRoot(const std::string& root);

  // Convert a virtual device path ("/books/foo.txt") to a host path.
  std::string hostPath(const std::string& virtPath) const;

  File   open(const char* path, const char* mode = "r");
  File   open(const std::string& path, const char* mode = "r") { return open(path.c_str(), mode); }

  bool   exists(const char* path);
  bool   remove(const char* path);
  bool   rename(const char* from, const char* to);
  bool   mkdir(const char* path);
  bool   rmdir(const char* path);

  // String overloads — the firmware calls these with Arduino String objects.
  // The arduino_compat.h String has an implicit c_str() via operator const char*.
  // Providing explicit overloads avoids the implicit conversion ambiguity.
  template<typename S> File open(const S& path, const char* mode = "r")   { return open(path.c_str(), mode); }
  template<typename S> bool exists(const S& path)                          { return exists(path.c_str()); }
  template<typename S> bool remove(const S& path)                          { return remove(path.c_str()); }
  template<typename S> bool rename(const S& from, const S& to)             { return rename(from.c_str(), to.c_str()); }
  template<typename S> bool mkdir(const S& path)                           { return mkdir(path.c_str()); }
  template<typename S> bool rmdir(const S& path)                           { return rmdir(path.c_str()); }

  bool   begin(bool /*formatOnFail*/ = false) { return true; }
  void   end()                                {}
  bool   format()                             { return true; }
  size_t totalBytes() const                   { return 4u * 1024u * 1024u; }
  size_t usedBytes()  const;

private:
  std::string root_;
};

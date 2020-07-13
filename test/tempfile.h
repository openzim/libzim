#ifndef ZIM_TEST_TEMPFILE_H
#define ZIM_TEST_TEMPFILE_H

#include <string>

#ifdef _WIN32
#include <codecvt>
#include <windows.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <fileapi.h>
#undef min
#undef max
#else
#include <unistd.h>
#endif

namespace zim
{

namespace unittests
{

class TempFile
{
  int fd_;
#ifndef _WIN32
  std::string path_;
#endif
public:
  explicit TempFile(const char* name)
  {
#ifdef _WIN32
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> utf8ToUtf16;
    wchar_t cbase[MAX_PATH];
    wchar_t ctmp[MAX_PATH];
    const std::wstring wname = utf8ToUtf16.from_bytes(name);
    GetTempPathW(MAX_PATH-(wname.size()+2), cbase);
    // This create a file for us, ensure it is unique.
    // So we need to delete it and create the directory using the same name.
    GetTempFileNameW(cbase, wname.c_str(), 0, ctmp);
    auto tmp_fd = _wopen(ctmp, _O_CREAT | _O_TEMPORARY | _O_SHORT_LIVED | _O_RDWR | _O_TRUNC);
#else
    const char* const TMPDIR = std::getenv("TMPDIR");
    const std::string tmpdir(TMPDIR ? TMPDIR : "/tmp");
    path_ = tmpdir + "/" + name + "_XXXXXX";
    auto tmp_fd = mkstemp(&path_[0]);
#endif
    fd_ = tmp_fd;
  }

  TempFile(const TempFile& ) = delete;
  void operator=(const TempFile& ) = delete;

  ~TempFile()
  {
    close(fd_);
#ifndef _WIN32
    unlink(path_.c_str());
#endif
  }

  int fd() const { return fd_; }
};

} // namespace unittests

} // namespace zim

#endif // ZIM_TEST_TEMPFILE_H

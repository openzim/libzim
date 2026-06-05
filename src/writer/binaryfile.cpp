#include "binaryfile.h"

#include <fcntl.h>
#include <cstring>

#include <stdexcept>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#define _dup(fd) dup(fd)
#endif

namespace zim
{

namespace writer
{

BinaryFile::BinaryFile(int fd)
  : file(fd >= 0 ? fdopen(_dup(fd), "wb") : nullptr)
{
}

BinaryFile::~BinaryFile()
{
  closeFile();
}

void BinaryFile::openFile(const std::string& filePath)
{
  closeFile();

  file = fopen(filePath.c_str(), "wb");
  if ( !file ){
    throw std::runtime_error(std::strerror(errno));
  }
}

void BinaryFile::flush()
{
  if ( file ) {
    fflush(file);
  }
}

void BinaryFile::closeFile()
{
  if ( file ) {
    flush();
    fclose(file);
    file = nullptr;
  }
}

offset_type BinaryFile::tellFilePos() const
{
  return ftell(file);
}

void BinaryFile::seek(offset_type pos)
{
  if ( fseek(file, pos, SEEK_SET) == -1 ) {
    throw std::runtime_error(std::strerror(errno));
  }
}

offset_type BinaryFile::seekEnd()
{
  if ( fseek(file, 0, SEEK_END) == -1 ) {
    throw std::runtime_error(std::strerror(errno));
  }

  return tellFilePos();
}

void BinaryFile::write(const char* buf, size_t size)
{
  if ( fwrite(buf, 1, size, file) != size ) {
    throw std::runtime_error(std::strerror(errno));
  }
}

} // namespace writer

} // namespace zim

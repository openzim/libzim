#include "binaryfile.h"

#include <fcntl.h>
#include <cstring>

#include <stdexcept>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#define _write(fd, addr, size) if(::write((fd), (addr), (size)) != (ssize_t)(size)) \
{throw std::runtime_error("Error writing");}
#endif

namespace zim
{

namespace writer
{

BinaryFile::BinaryFile(int fd)
  : out_fd(fd)
{
}

BinaryFile::~BinaryFile()
{
  closeFile();
}

void BinaryFile::openFile(const std::string& filePath)
{
  closeFile();

#ifdef _WIN32
  int flag = _O_RDWR | _O_CREAT | _O_TRUNC | _O_BINARY;
  int mode =  _S_IREAD | _S_IWRITE;
#else
  int flag = O_RDWR | O_CREAT | O_TRUNC;
  mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
#endif
  out_fd = ::open(filePath.c_str(), flag, mode);
  if (out_fd == -1){
    throw std::runtime_error(std::strerror(errno));
  }
}

void BinaryFile::closeFile()
{
  if ( out_fd != -1 ) {
    ::close(out_fd);
    out_fd = -1;
  }
}

offset_type BinaryFile::tellFilePos() const
{
  return lseek(out_fd, 0, SEEK_CUR);
}

void BinaryFile::seek(offset_type pos)
{
  if ( offset_type(lseek(out_fd, pos, SEEK_SET)) != pos ) {
    throw std::runtime_error(std::strerror(errno));
  }
}

offset_type BinaryFile::seekEnd()
{
  return lseek(out_fd, 0, SEEK_END);
}

void BinaryFile::write(const char* buf, size_t size)
{
  _write(out_fd, buf, size);
}

} // namespace writer

} // namespace zim

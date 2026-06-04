#include "binaryfile.h"

#include <fcntl.h>
#include <cstring>

#include <stdexcept>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace zim
{

namespace writer
{

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

} // namespace writer

} // namespace zim

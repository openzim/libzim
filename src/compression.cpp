#include "compression.h"

#include "envvalue.h"

#include <stdexcept>
#include <zlib.h>

const std::string LZMA_INFO::name = "lzma";
void LZMA_INFO::init_stream_decoder(stream_t* stream, char* raw_data)
{
  *stream = LZMA_STREAM_INIT;
  unsigned memsize = zim::envMemSize("ZIM_LZMA_MEMORY_SIZE", LZMA_MEMORY_SIZE * 1024 * 1024);
  auto errcode = lzma_stream_decoder(stream, memsize, 0);
  if (errcode != LZMA_OK) {
    throw std::runtime_error("Impossible to allocated needed memory to uncompress lzma stream");
  }
}

void LZMA_INFO::init_stream_encoder(stream_t* stream, char* raw_data)
{
  *stream = LZMA_STREAM_INIT;
  auto errcode = lzma_easy_encoder(stream, 9 | LZMA_PRESET_EXTREME, LZMA_CHECK_CRC32);
  if (errcode != LZMA_OK) {
    throw std::runtime_error("Cannot initialize lzma_easy_encoder");
  }
}

CompStatus LZMA_INFO::stream_run_encode(stream_t* stream, CompStep step) {
  return stream_run(stream, step);
}

CompStatus LZMA_INFO::stream_run_decode(stream_t* stream, CompStep step) {
  return stream_run(stream, step);
}

CompStatus LZMA_INFO::stream_run(stream_t* stream, CompStep step)
{
  auto errcode = lzma_code(stream, step==CompStep::STEP?LZMA_RUN:LZMA_FINISH);
  if (errcode == LZMA_BUF_ERROR)
    return CompStatus::BUF_ERROR;
  if (errcode == LZMA_STREAM_END)
    return CompStatus::STREAM_END;
  if (errcode == LZMA_OK)
    return CompStatus::OK;
  return CompStatus::OTHER;
}

void LZMA_INFO::stream_end_decode(stream_t* stream)
{
  lzma_end(stream);
}

void LZMA_INFO::stream_end_encode(stream_t* stream)
{
  lzma_end(stream);
}


#if defined(ENABLE_ZLIB)
const std::string ZIP_INFO::name = "zlib";
void ZIP_INFO::init_stream_decoder(stream_t* stream, char* raw_data)
{
  memset(stream, 0, sizeof(stream_t));
  stream->next_in = (unsigned char*) raw_data;
  stream->avail_in = 1024;
  auto errcode = ::inflateInit(stream);
  if (errcode != Z_OK) {
    throw std::runtime_error("Impossible to allocated needed memory to uncompress zlib stream");
  }
}

void ZIP_INFO::init_stream_encoder(stream_t* stream, char* raw_data)
{
  memset(stream, 0, sizeof(z_stream));
  auto errcode = ::deflateInit(stream, Z_DEFAULT_COMPRESSION);
  if (errcode != Z_OK) {
    throw std::runtime_error("Impossible to allocated needed memory to uncompress zlib stream");
  }
}

CompStatus ZIP_INFO::stream_run_decode(stream_t* stream, CompStep step) {
  auto errcode = ::inflate(stream, step==CompStep::STEP?Z_SYNC_FLUSH:Z_FINISH);
  if (errcode == Z_BUF_ERROR)
    return CompStatus::BUF_ERROR;
  if (errcode == Z_STREAM_END)
    return CompStatus::STREAM_END;
  if (errcode == Z_OK)
    return CompStatus::OK;
  return CompStatus::OTHER;
}

CompStatus ZIP_INFO::stream_run_encode(stream_t* stream, CompStep step) {
  auto errcode = ::deflate(stream, step==CompStep::STEP?Z_SYNC_FLUSH:Z_FINISH);
  if (errcode == Z_BUF_ERROR)
    return CompStatus::BUF_ERROR;
  if (errcode == Z_STREAM_END)
    return CompStatus::STREAM_END;
  if (errcode == Z_OK)
    return CompStatus::OK;
  return CompStatus::OTHER;
}

void ZIP_INFO::stream_end_decode(stream_t* stream) {
  auto ret = ::inflateEnd(stream);
  ASSERT(ret, ==, Z_OK);
}

void ZIP_INFO::stream_end_encode(stream_t* stream) {
  auto ret = ::deflateEnd(stream);
  ASSERT(ret, ==, Z_OK);
}
#endif // ENABLE_ZLIB

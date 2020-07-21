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

const std::string ZSTD_INFO::name = "zstd";

ZSTD_INFO::stream_t::stream_t()
: next_in(nullptr),
  avail_in(0),
  next_out(nullptr),
  avail_out(0),
  total_out(0),
  encoder_stream(nullptr),
  decoder_stream(nullptr)
{}

ZSTD_INFO::stream_t::~stream_t()
{
  if ( encoder_stream )
    ::ZSTD_freeCStream(encoder_stream);

  if ( decoder_stream )
    ::ZSTD_freeDStream(decoder_stream);
}

void ZSTD_INFO::init_stream_decoder(stream_t* stream, char* raw_data)
{
  stream->decoder_stream = ::ZSTD_createDStream();
  auto ret = ::ZSTD_initDStream(stream->decoder_stream);
  if (::ZSTD_isError(ret)) {
    throw std::runtime_error("Failed to initialize Zstd decompression");
  }
}

void ZSTD_INFO::init_stream_encoder(stream_t* stream, char* raw_data)
{
  stream->encoder_stream = ::ZSTD_createCStream();
  auto ret = ::ZSTD_initCStream(stream->encoder_stream, ::ZSTD_maxCLevel());
  if (::ZSTD_isError(ret)) {
    throw std::runtime_error("Failed to initialize Zstd compression");
  }
}

CompStatus ZSTD_INFO::stream_run_encode(stream_t* stream, CompStep step) {
  ::ZSTD_inBuffer inBuf;
  inBuf.src = stream->next_in;
  inBuf.size = stream->avail_in;
  inBuf.pos = 0;

  ::ZSTD_outBuffer outBuf;
  outBuf.dst = stream->next_out;
  outBuf.size = stream->avail_out;
  outBuf.pos = 0;

  auto ret = step == CompStep::STEP
           ? ::ZSTD_compressStream(stream->encoder_stream, &outBuf, &inBuf)
           : ::ZSTD_endStream(stream->encoder_stream, &outBuf);
  stream->next_in += inBuf.pos;
  stream->avail_in -= inBuf.pos;
  stream->next_out += outBuf.pos;
  stream->avail_out -= outBuf.pos;
  stream->total_out += outBuf.pos;

  if (::ZSTD_isError(ret)) {
    return CompStatus::OTHER;
  }

  if ( step == CompStep::STEP ) {
    if ( stream->avail_in != 0) {
      ASSERT(stream->avail_out, ==, 0u);
      return CompStatus::BUF_ERROR;
    }
  } else if ( ret > 0 ) {
      return CompStatus::BUF_ERROR;
  }

  return CompStatus::OK;
}

CompStatus ZSTD_INFO::stream_run_decode(stream_t* stream, CompStep /*step*/) {
  ::ZSTD_inBuffer inBuf;
  inBuf.src = stream->next_in;
  inBuf.size = stream->avail_in;
  inBuf.pos = 0;

  ::ZSTD_outBuffer outBuf;
  outBuf.dst = stream->next_out;
  outBuf.size = stream->avail_out;
  outBuf.pos = 0;

  auto ret = ::ZSTD_decompressStream(stream->decoder_stream, &outBuf, &inBuf);
  stream->next_in += inBuf.pos;
  stream->avail_in -= inBuf.pos;
  stream->next_out += outBuf.pos;
  stream->avail_out -= outBuf.pos;
  stream->total_out += outBuf.pos;

  if (::ZSTD_isError(ret))
    return CompStatus::OTHER;

  if (ret == 0)
    return CompStatus::STREAM_END;

  return CompStatus::BUF_ERROR;
}

void ZSTD_INFO::stream_end_decode(stream_t* stream)
{
}

void ZSTD_INFO::stream_end_encode(stream_t* stream)
{
}

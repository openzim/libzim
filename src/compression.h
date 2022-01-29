/*
 * Copyright (C) 2020-2021 Matthieu Gautier <mgautier@kymeria.fr>
 * Copyright (C) 2020 Emmanuel Engelhart <kelson@kiwix.org>
 * Copyright (C) 2020 Veloman Yunkan
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * is provided AS IS, WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, and
 * NON-INFRINGEMENT.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

#ifndef _LIBZIM_COMPRESSION_
#define _LIBZIM_COMPRESSION_

#include <vector>
#include "string.h"

#include "file_reader.h"
#include <zim/error.h>

#include "config.h"

#include <lzma.h>
#include <zstd.h>

#include "zim_types.h"
#include "constants.h"

//#define DEB(X) std::cerr << __func__ << " " << X << std::endl ;
#define DEB(X)

enum class CompStep {
  STEP,
  FINISH
};

enum class CompStatus {
  OK,
  STREAM_END,
  BUF_ERROR,
};

enum class RunnerStatus {
  OK,
  NEED_MORE,
  ERROR
};

struct LZMA_INFO {
  typedef lzma_stream stream_t;
  static const std::string name;
  static void init_stream_encoder(stream_t* stream, int compression_level, char* raw_data);
  static void init_stream_decoder(stream_t* stream, char* raw_data);
  static CompStatus stream_run_encode(stream_t* stream, CompStep step);
  static CompStatus stream_run_decode(stream_t* stream, CompStep step);
  static CompStatus stream_run(stream_t* stream, CompStep step);
  static void stream_end_encode(stream_t* stream);
  static void stream_end_decode(stream_t* stream);
};


struct ZSTD_INFO {
  struct stream_t
  {
    const unsigned char* next_in;
    size_t avail_in;
    unsigned char* next_out;
    size_t avail_out;
    size_t total_out;

    ::ZSTD_CStream* encoder_stream;
    ::ZSTD_DStream* decoder_stream;

    stream_t();
    ~stream_t();
  private:
    stream_t(const stream_t& t) = delete;
    void operator=(const stream_t& t) = delete;
  };

  static const std::string name;
  static void init_stream_encoder(stream_t* stream, int compression_level, char* raw_data);
  static void init_stream_decoder(stream_t* stream, char* raw_data);
  static CompStatus stream_run_encode(stream_t* stream, CompStep step);
  static CompStatus stream_run_decode(stream_t* stream, CompStep step);
  static void stream_end_encode(stream_t* stream);
  static void stream_end_decode(stream_t* stream);
};


namespace zim {

template<typename INFO>
class Uncompressor
{
  public:
    Uncompressor(size_t initial_size) :
      ret_data(new char[initial_size]),
      data_size(initial_size)
    {}
    ~Uncompressor() = default;

    void init(char* data) {
      INFO::init_stream_decoder(&stream, data);
      stream.next_out = (uint8_t*)ret_data.get();
      stream.avail_out = data_size;
    }

    RunnerStatus feed(char* data, size_t size, CompStep step = CompStep::STEP) {
      stream.next_in = (unsigned char*)data;
      stream.avail_in = size;
      while (true) {
        auto errcode = INFO::stream_run_decode(&stream, step);
        DEB((int)errcode)
        switch (errcode) {
          case CompStatus::BUF_ERROR:
            if (stream.avail_in == 0 && stream.avail_out != 0)  {
              // End of input stream.
              // compressor hasn't recognize the end of the input stream but there is
              // no more input.
              return RunnerStatus::NEED_MORE;
            } else {
              // Not enought output size.
              // Allocate more memory and continue the loop.
              DEB("need memory " << data_size << " " << stream.avail_out << " " << stream.total_out)
              data_size *= 2;
              std::unique_ptr<char[]> new_ret_data(new char[data_size]);
              memcpy(new_ret_data.get(), ret_data.get(), stream.total_out);
              stream.next_out = (unsigned char*)(new_ret_data.get() + stream.total_out);
              stream.avail_out = data_size - stream.total_out;
              DEB(data_size << " " << stream.avail_out << " " << stream.avail_in)
              ret_data = std::move(new_ret_data);
            }
            break;
          case CompStatus::OK:
            // On first call where lzma cannot progress (no output size).
            // Lzma return OK. If we return NEED_MORE, then we will try to compress
            // with new input data, but we should not as current one is not processed.
            // We must do a second step to have te BUF_ERROR and handle thing correctly.
            // If we have no more input, then we must ask for more.
            if (stream.avail_in == 0) {
              return RunnerStatus::NEED_MORE;
            }
            break;
          case CompStatus::STREAM_END:
            // End of compressed stream. Everything is ok.
            return RunnerStatus::OK;
          default:
            // unreachable
            return RunnerStatus::ERROR;
        }
      };
      // unreachable
      return RunnerStatus::NEED_MORE;
    }

    std::unique_ptr<char[]> get_data(zim::zsize_t* size) {
      feed(nullptr, 0, CompStep::FINISH);
      size->v = stream.total_out;
      INFO::stream_end_decode(&stream);
      return std::move(ret_data);
    }

  private:
    std::unique_ptr<char[]> ret_data;
    size_type data_size;
    typename INFO::stream_t stream;
};

#define CHUNCK_SIZE ((zim::size_type)(1024))
/**
 * Uncompress data of the reader at startOffset.
 *
 * @param reader         The reader where the data is.
 * @param startOffset    The offset where the data is in the reader.
 * @param[out] dest_size The size of the uncompressed data.
 * @return A pointer to the uncompressed data. This must be deleted (delete[])
*/
template<typename INFO>
std::unique_ptr<char[]> uncompress(const zim::Reader* reader, zim::offset_t startOffset, zim::zsize_t* dest_size) {
  // Use a compressor to compress the data.
  // As we don't know the result size, neither the compressed size,
  // we have to do chunk by chunk until decompressor is happy.
  // Let's assume it will be something like the default clusterSize used at creation
  Uncompressor<INFO> runner(DEFAULT_CLUSTER_SIZE);
  // The input is a buffer of CHUNCK_SIZE char max. It may be less if the last chunk
  // is at the end of the reader and the reader size is not a multiple of CHUNCK_SIZE.
  std::vector<char> raw_data(CHUNCK_SIZE);

  DEB("Init")
  runner.init(raw_data.data());

  zim::size_type availableSize = reader->size().v - startOffset.v;
  auto ret = RunnerStatus::NEED_MORE;
  while(ret != RunnerStatus::OK) {
    if (ret == RunnerStatus::NEED_MORE and availableSize) {
      zim::size_type inputSize = std::min(availableSize, CHUNCK_SIZE);
      reader->read(raw_data.data(), startOffset, zim::zsize_t(inputSize));
      startOffset.v += inputSize;
      availableSize -= inputSize;
      DEB("Step " << startOffset.v)
      ret = runner.feed(raw_data.data(), inputSize);
      DEB("Ret " << (int)ret)
    }
    if (ret == RunnerStatus::ERROR) {
      throw zim::ZimFileFormatError(std::string("Invalid ") + INFO::name
                               + std::string(" stream for cluster."));
    }
  }

  DEB("Finish")
  return runner.get_data(dest_size);
}

template<typename INFO>
class Compressor
{
  public:
    Compressor(size_t initial_size=1024*1024) :
      ret_data(new char[initial_size]),
      ret_size(initial_size)
    {}

    ~Compressor() = default;

    void init(int compression_level, char * data) {
      INFO::init_stream_encoder(&stream, compression_level, data);
      stream.next_out = (uint8_t*)ret_data.get();
      stream.avail_out = ret_size;
    }

    RunnerStatus feed(const char* data, size_t size, CompStep step=CompStep::STEP) {
      stream.next_in = (unsigned char*)data;
      stream.avail_in = size;
      while (true) {
        auto errcode = INFO::stream_run_encode(&stream, step);
        switch (errcode) {
          case CompStatus::OK:
            if (stream.avail_out == 0) {
              // lzma return a OK return status the first time it runs out of output memory.
              // The BUF_ERROR is returned only the second time we call a lzma_code.
              continue;
            } else {
              return RunnerStatus::NEED_MORE;
            }
          case CompStatus::STREAM_END:
            return RunnerStatus::NEED_MORE;
          case CompStatus::BUF_ERROR:
            if (stream.avail_out == 0) {
              //Not enought output size
              ret_size *= 2;
              std::unique_ptr<char[]> new_ret_data(new char[ret_size]);
              memcpy(new_ret_data.get(), ret_data.get(), stream.total_out);
              stream.next_out = (unsigned char*)(new_ret_data.get() + stream.total_out);
              stream.avail_out = ret_size - stream.total_out;
              ret_data = std::move(new_ret_data);
              continue;
            } else {
              return RunnerStatus::ERROR;
            }
          break;
          default:
            // unreachable
            return RunnerStatus::ERROR;
        };
      };
      // urreachable
      return RunnerStatus::NEED_MORE;
    }

    std::unique_ptr<char[]> get_data(zim::zsize_t* size) {
      feed(nullptr, 0, CompStep::FINISH);
      INFO::stream_end_encode(&stream);
      size->v = stream.total_out;
      return std::move(ret_data);
    }

  private:
    std::unique_ptr<char[]> ret_data;
    size_t ret_size;
    typename INFO::stream_t stream;
};

} // namespace zim

#endif // _LIBZIM_COMPRESSION_

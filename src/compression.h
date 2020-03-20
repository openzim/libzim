#ifndef _LIBZIM_COMPRESSION_
#define _LIBZIM_COMPRESSION_

#include <vector>
#include "string.h"

#include "file_reader.h"
#include <zim/error.h>

#include "config.h"

#include <lzma.h>
#if defined(ENABLE_ZLIB)
#include <zlib.h>
#endif


#include "zim_types.h"

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
  OTHER
};

enum class RunnerStatus {
  OK,
  NEED_MORE,
  ERROR
};

struct LZMA_INFO {
  typedef lzma_stream stream_t;
  static const std::string name;
  static void init_stream_decoder(stream_t* stream, char* raw_data);
  static void init_stream_encoder(stream_t* stream, char* raw_data);
  static CompStatus stream_run_encode(stream_t* stream, CompStep step);
  static CompStatus stream_run_decode(stream_t* stream, CompStep step);
  static CompStatus stream_run(stream_t* stream, CompStep step);
  static void stream_end_encode(stream_t* stream);
  static void stream_end_decode(stream_t* stream);
};


#if defined(ENABLE_ZLIB)
struct ZIP_INFO {
  typedef z_stream stream_t;
  static const std::string name;
  static void init_stream_decoder(stream_t* stream, char* raw_data);
  static void init_stream_encoder(stream_t* stream, char* raw_data);
  static CompStatus stream_run_encode(stream_t* stream, CompStep step);
  static CompStatus stream_run_decode(stream_t* stream, CompStep step);
  static void stream_end_encode(stream_t* stream);
  static void stream_end_decode(stream_t* stream);
};
#endif

namespace zim {

template<typename INFO>
class Uncompressor
{
  public:
    Uncompressor(size_t initial_size=1024*1024) :
      ret_data(new char[initial_size]),
      data_size(initial_size)
    {}
    ~Uncompressor() = default;

    void init(char* data) {
      INFO::init_stream_decoder(&stream, data);
      stream.next_out = (uint8_t*)ret_data;
      stream.avail_out = data_size;
    }

    RunnerStatus feed(char* data, size_t size, CompStep step = CompStep::STEP) {
      stream.next_in = (unsigned char*)data;
      stream.avail_in = size;
      auto errcode = CompStatus::OTHER;
      while (true) {
        errcode = INFO::stream_run_decode(&stream, step);
        DEB((int)errcode)
        if (errcode == CompStatus::BUF_ERROR) {
          if (stream.avail_in == 0 && stream.avail_out != 0)  {
            // End of input stream.
            // compressor hasn't recognize the end of the input stream but there is
            // no more input.
            return RunnerStatus::NEED_MORE;
          } else {
            //Not enought output size
            DEB("need memory " << data_size << " " << stream.avail_out << " " << stream.total_out)
            data_size *= 2;
            char * new_ret_data = new char[data_size];
            memcpy(new_ret_data, ret_data, stream.total_out);
            stream.next_out = (unsigned char*)(new_ret_data + stream.total_out);
            stream.avail_out = data_size - stream.total_out;
            DEB(data_size << " " << stream.avail_out << " " << stream.avail_in)
            delete [] ret_data;
            ret_data = new_ret_data;
            continue;
          }
        }
        if (errcode == CompStatus::STREAM_END)
          break;
        // On first call where lzma cannot progress (no output size).
        // Lzma return OK. If we return NEED_MORE, then we will try to compress
        // with new input data, but we should not as current one is not processed.
        // We must do a second step to have te BUF_ERROR and handle thing correctly.
        if (errcode == CompStatus::OK) {
          if (stream.avail_in == 0)
            break;
          continue;
        }
        return RunnerStatus::ERROR;
      };
      return errcode==CompStatus::STREAM_END?RunnerStatus::OK:RunnerStatus::NEED_MORE;
    }

    char* get_data(zim::zsize_t* size) {
      size->v = stream.total_out;
      INFO::stream_end_decode(&stream);
      return ret_data;
    }

  private:
    char* ret_data;
    size_type data_size;
    typename INFO::stream_t stream;
};

#define CHUNCK_SIZE ((zim::size_type)(1024))
/**
 * Uncompress data of the reader at startOffset.
 *
 * @param reader         The reader where the data is.
 * @param startOffset    The offset where the data is in the reader.
 * @param dest_size[out] The size of the uncompressed data.
 * @return A pointer to the uncompressed data. This must be deleted (delete[])
*/
template<typename INFO>
char* uncompress(const zim::Reader* reader, zim::offset_t startOffset, zim::zsize_t* dest_size) {
  // Use a compressor to compress the data.
  // As we don't know the result size, neither the compressed size,
  // we have to do chunk by chunk until decompressor is happy.
  // Let's assume it will be something like the minChunkSize used at creation
  Uncompressor<INFO> runner(1024*1024);
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

    void init(char* data) {
      INFO::init_stream_encoder(&stream, data);
      stream.next_out = (uint8_t*)ret_data;
      stream.avail_out = ret_size;
    }

    RunnerStatus feed(const char* data, size_t size, CompStep step=CompStep::STEP) {
      stream.next_in = (unsigned char*)data;
      stream.avail_in = size;
      auto errcode = CompStatus::OTHER;
      while (1) {
        errcode = INFO::stream_run_encode(&stream, step);
        if (errcode == CompStatus::BUF_ERROR) {
          if (stream.avail_out == 0 && stream.avail_in != 0) {
            //Not enought output size
            ret_size *= 2;
            char * new_ret_data = new char[ret_size];
            memcpy(new_ret_data, ret_data, stream.total_out);
            stream.next_out = (unsigned char*)(new_ret_data + stream.total_out);
            stream.avail_out = ret_size - stream.total_out;
            delete [] ret_data;
            ret_data = new_ret_data;
            continue;
          }
        }
        if (errcode == CompStatus::STREAM_END || errcode == CompStatus::OK)
          break;
        return RunnerStatus::ERROR;
      };
      return RunnerStatus::NEED_MORE;
    }

    char* get_data(zim::zsize_t* size) {
      feed(nullptr, 0, CompStep::FINISH);
      INFO::stream_end_encode(&stream);
      size->v = stream.total_out;
      return ret_data;
    }

  private:
    char* ret_data;
    size_t ret_size;
    typename INFO::stream_t stream;
};

} // namespace zim

#endif // _LIBZIM_COMPRESSION_

/*
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

#ifndef ZIM_DECODERSTREAMREADER_H
#define ZIM_DECODERSTREAMREADER_H

#include "compression.h"
#include "istreamreader.h"

namespace zim
{

template<typename Decoder>
class DecoderStreamReader : public IStreamReader
{
private: // constants
  enum { CHUNK_SIZE = 1024 };

public: // functions
  DecoderStreamReader(std::shared_ptr<const Reader> inputReader)
    : m_encodedDataReader(inputReader),
      m_currentInputOffset(0),
      m_inputBytesLeft(inputReader->size()),
      m_encodedDataChunk(Buffer::makeBuffer(zsize_t(CHUNK_SIZE)))
  {
    Decoder::init_stream_decoder(&m_decoderState, nullptr);
    readNextChunk();
  }

  ~DecoderStreamReader()
  {
    Decoder::stream_end_decode(&m_decoderState);
  }

private: // functions
  void readNextChunk()
  {
    const auto n = std::min(zsize_t(CHUNK_SIZE), m_inputBytesLeft);
    m_encodedDataChunk = m_encodedDataReader->get_buffer(m_currentInputOffset, n);
    m_currentInputOffset += n;
    m_inputBytesLeft -= n;
    // XXX: ugly C-style cast (casting away constness) on the next line
    m_decoderState.next_in  = (unsigned char*)m_encodedDataChunk.data();
    m_decoderState.avail_in = m_encodedDataChunk.size().v;
  }

  CompStatus decodeMoreBytes()
  {
    CompStep step = CompStep::STEP;
    if ( m_decoderState.avail_in == 0 )
    {
      if ( m_inputBytesLeft.v == 0 )
        step = CompStep::FINISH;
      else
        readNextChunk();
    }

    return Decoder::stream_run_decode(&m_decoderState, step);
  }

  void readImpl(char* buf, zsize_t nbytes) override
  {
    m_decoderState.next_out = (unsigned char*)buf;
    m_decoderState.avail_out = nbytes.v;
    while ( m_decoderState.avail_out != 0 )
    {
      // We don't car of the return code of decodeMoreBytes.
      // We feed (or stop feeding) the decoder based on what
      // we need to decode and the `avail_in`.
      // If there is a error somehow, a exception will be thrown.
      decodeMoreBytes();
    }
  }

private: // types
  typedef typename Decoder::stream_t DecoderState;

private: // data
  std::shared_ptr<const Reader> m_encodedDataReader;
  offset_t m_currentInputOffset;
  zsize_t m_inputBytesLeft; // count of bytes left in the input stream
  DecoderState m_decoderState;
  Buffer m_encodedDataChunk;
};

} // namespace zim

#endif // ZIM_DECODERSTREAMREADER_H

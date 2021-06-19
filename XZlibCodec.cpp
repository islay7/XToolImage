//-----------------------------------------------------------------------------
//								XZlibCodec.cpp
//								==============
//
// Auteur : F.Becirspahic - IGN / DSTI / SIMV
//
// Date : 10/06/2021
//-----------------------------------------------------------------------------


#include "XZlibCodec.h"
#include "zlib.h"


bool XZlibCodec::Decompress(byte* lzw, uint32 size_in, byte* out, uint32 size_out)
{
	int ret, flush = 0;
	z_stream strm;

  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  ret = inflateInit(&strm);
  if (ret != Z_OK)
    return false;

  strm.avail_in = size_in;
  strm.next_in = lzw;
  strm.avail_out = size_out;
  strm.next_out = out;

  ret = inflate(&strm, flush);
  if (ret == Z_STREAM_ERROR)
    return false;
  (void)inflateEnd(&strm);

  return true;
}
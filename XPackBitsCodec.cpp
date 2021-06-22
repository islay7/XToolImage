//-----------------------------------------------------------------------------
//								XPackBitsCodec.cpp
//								==================
//
// Auteur : F.Becirspahic - MODSP
//
// 23/08/2009
//
// Classe XPackBitsCodec : compresseur / Decompresseur PackBits
//-----------------------------------------------------------------------------

#include <cstring>
#include "XPackBitsCodec.h"

bool XPackBitsCodec::Decompress(byte* lzw, uint32 size_in, byte* out, uint32 size_out)
{
  uint32 i, count = 0, cmpt = 0;
  signed char n;
  while (cmpt < size_in) {
    n = (signed char)lzw[cmpt];
    if ((n >= 0) && (n <= 127)) {
      ::memcpy((void*)&out[count], (void*)&lzw[cmpt + 1], (n + 1));
      count += (n + 1);
      cmpt += (n + 2);
    }
    else {
      if (n <= -1) {
        for (i = count; i < (count + 1 - n); i++)
          out[i] = lzw[cmpt + 1];
        count += (1 - n);
        cmpt += 2;
      }
      else // Cas n == 128
        break;
    }
  }
  return true;
}

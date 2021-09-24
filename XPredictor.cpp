//-----------------------------------------------------------------------------
//								XPredictor.cpp
//								===============
//
// Auteur : F.Becirspahic - IGN / DSTI / SIMV
//
// Date : 24/9/2021
//-----------------------------------------------------------------------------

#include "XPredictor.h"
#include <cstring>

bool XPredictor::Decode(byte* Pix, uint32 W, uint32 H, uint32 pixSize, uint32 nbBits, unsigned int num_algo)
{
  if (num_algo == 1) return true;
  if (num_algo == 2) { // PREDICTOR_HORIZONTAL
    uint32 lineW = W * pixSize;
    if (nbBits == 8){
      for (uint32 i = 0; i < H; i++)
        for (uint32 j = i * lineW; j < (i + 1) * lineW - pixSize; j++)
          Pix[j + pixSize] += Pix[j];
    }
    if (nbBits == 16){
      for (uint32 i = 0; i < H; i++){
        uint16* ptr = (uint16*)&Pix[lineW*i];
        for (uint32 j = 0; j < (W - 1); j++) {
          ptr[1] += ptr[0];
          ptr++;
        }
      }
    }
    if (nbBits == 32){
      for (uint32 i = 0; i < H; i++){
        uint32* ptr = (uint32*)&Pix[lineW*i];
        for (uint32 j = 0; j < (W - 1); j++) {
          ptr[1] += ptr[0];
          ptr++;
        }
      }
    }
    return true;
  }
  if (num_algo == 3) { // PREDICTOR_FLOATINGPOINT
    uint32 lineW = W * pixSize;
    byte* buf = new byte[lineW];
    for (uint32 i = 0; i < H; i++) {
      for (uint32 j = i * lineW; j < (i + 1) * lineW - 1; j++)
        Pix[j + 1] += Pix[j];
      std::memcpy(buf, &Pix[i * lineW], lineW);
      byte* ptr_0 = buf;
      byte* ptr_1 = &buf[W];
      byte* ptr_2 = &buf[W*2];
      byte* ptr_3 = &buf[W*3];
      for (uint32 j = i * lineW; j < (i + 1) * lineW; j += 4) {
        Pix[j] = *ptr_3; ptr_3++;
        Pix[j + 1] = *ptr_2; ptr_2++;
        Pix[j + 2] = *ptr_1; ptr_1++;
        Pix[j + 3] = *ptr_0; ptr_0++;
      }
    }
    delete[] buf;
    return true;
  }
  return false;
}

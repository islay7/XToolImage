//-----------------------------------------------------------------------------
//								XPredictor.h
//								===============
//
// Auteur : F.Becirspahic - IGN / DSTI / SIMV
//
// Date : 24/9/2021
//-----------------------------------------------------------------------------

#ifndef XPREDICTOR_H
#define XPREDICTOR_H

#include "../XTool/XBase.h"

class XPredictor {
public:
  XPredictor() {;}

  bool Decode(byte* Pix, uint32 W, uint32 H, uint32 pixSize, uint32 nbBits, uint32 num_algo);
};

#endif // XPREDICTOR_H

//-----------------------------------------------------------------------------
//								XZlibCodec.h
//								============
//
// Auteur : F.Becirspahic - IGN / DSTI / SIMV
//
// Date : 10/06/2021
//-----------------------------------------------------------------------------

#ifndef XZLIBCODEC_H
#define XZLIBCODEC_H

#include "../XTool/XBase.h"

class XZlibCodec {
public:
	XZlibCodec() { ; }
	virtual ~XZlibCodec() { ; }

	bool Decompress(byte* lzw, uint32 size_in, byte* out, uint32 size_out);
};

#endif //XZLIBCODEC_H
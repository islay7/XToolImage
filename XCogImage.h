//-----------------------------------------------------------------------------
//								XCogImage.h
//								===========
//
// Auteur : F.Becirspahic - IGN / DSTI / SIMV
//
// Date : 24/06/2021
//-----------------------------------------------------------------------------

#ifndef XCOGIMAGE_H
#define XCOGIMAGE_H

#include "XBaseImage.h"
#include "XTiffReader.h"
#include "XTiffTileImage.h"

class XCogImage : public XBaseImage {
protected:
	XTiffReader		m_Reader;
	std::vector< XTiffTileImage*>	m_TImages;
	std::vector<uint32>						m_Factor;

public:
	XCogImage() { ; }
	virtual ~XCogImage() { Clear(); }

  virtual bool Open(XFile* file);
	virtual void Clear();

	virtual bool GetArea(XFile* file, uint32 x, uint32 y, uint32 w, uint32 h, byte* area);
	virtual bool GetLine(XFile* file, uint32 num, byte* area);
	virtual bool GetZoomArea(XFile* file, uint32 x, uint32 y, uint32 w, uint32 h, byte* area, uint32 factor);

};

#endif //XCOGIMAGE_H

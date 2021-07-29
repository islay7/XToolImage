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
#include "XTiffTileImage.h"

class XBaseTiffReader;

class XCogImage : public XBaseImage {
protected:
	XBaseTiffReader*							m_Reader;
	std::vector<XTiffTileImage*>	m_TImages;
	std::vector<uint32>						m_Factor;

public:
	XCogImage() { m_Reader = NULL; }
	virtual ~XCogImage() { Clear(); }

  virtual bool Open(XFile* file);
	virtual void Clear();
  virtual std::string Format() { return "COG";}
  virtual std::string Metadata();

	virtual bool GetArea(XFile* file, uint32 x, uint32 y, uint32 w, uint32 h, byte* area);
	virtual bool GetLine(XFile* file, uint32 num, byte* area);
	virtual bool GetZoomArea(XFile* file, uint32 x, uint32 y, uint32 w, uint32 h, byte* area, uint32 factor);
};

#endif //XCOGIMAGE_H

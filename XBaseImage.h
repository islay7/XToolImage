//-----------------------------------------------------------------------------
//								XBaseImage.h
//								============
//
// Auteur : F.Becirspahic - IGN / DSTI / SIMV
//
// Date : 18/06/2021
//-----------------------------------------------------------------------------

#ifndef XBASEIMAGE_H
#define XBASEIMAGE_H

#include "../XTool/XBase.h"
#include "../XTool/XFile.h"

class XBaseImage {
protected:
	uint32		m_nW;
	uint32		m_nH;
	uint16		m_nNbBits;
	uint16		m_nNbSample;

	// Georeferencement
	double		m_dX0;
	double		m_dY0;
	double		m_dGSD;

public:
	XBaseImage();
	virtual ~XBaseImage();

	inline uint32 W() { return m_nW; }
	inline uint32 H() { return m_nH; }
	inline uint16 NbBits() { return m_nNbBits; }
	inline uint16 NbSample() { return m_nNbSample; }
	inline double GSD() { return m_dGSD; }
	inline double X0() { return m_dX0; }
	inline double Y0() { return m_dY0; }

	virtual uint32 PixSize();
	virtual byte* AllocArea(uint32 w, uint32 h);

	virtual bool GetArea(XFile* file, uint32 x, uint32 y, uint32 w, uint32 h, byte* area) = 0;

	static bool CMYK2RGB(byte* buffer, uint32 w, uint32 h);
};

#endif //XBASEIMAGE_H
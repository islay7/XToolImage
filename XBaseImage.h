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
	virtual bool GetLine(XFile* file, uint32 num, byte* area) = 0;
	virtual bool GetZoomArea(XFile* file, uint32 x, uint32 y, uint32 w, uint32 h, byte* area, uint32 factor) = 0;

	static bool CMYK2RGB(byte* buffer, uint32 w, uint32 h);
	static bool ZoomArea(byte* in, byte* out, uint32 win, uint32 hin, uint32 wout, uint32 hout, uint32 nbbyte);
	static void SwitchRGB2BGR(byte* buf, uint32 buf_size);
	static bool RotateArea(byte* in, byte* out, uint32 win, uint32 hin, uint32 nbbyte, uint32 rot);

};

#endif //XBASEIMAGE_H
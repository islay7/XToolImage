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
  uint16*		m_ColorMap;
  uint16    m_nColorMapSize;

	// Georeferencement
	double		m_dX0;
	double		m_dY0;
	double		m_dGSD;

public:
	XBaseImage();
	virtual ~XBaseImage();

	// Geometrie de l'image
	inline uint32 W() { return m_nW; }
	inline uint32 H() { return m_nH; }
	inline uint16 NbBits() { return m_nNbBits; }
	inline uint16 NbSample() { return m_nNbSample; }

  // Metadonnees
  virtual std::string Format() { return "Undefined";}
	virtual std::string Metadata();
  virtual std::string XmlMetadata() { return "";}

  // Palette de couleurs
  uint16 ColorMapSize() { return m_nColorMapSize;}
  uint16* ColorMap() { return m_ColorMap;}

	// Georeferencement de l'image
	inline double GSD() { return m_dGSD; }
	inline double X0() { return m_dX0; }
	inline double Y0() { return m_dY0; }
	void GetGeoref(double* xmin, double* ymax, double* gsd) { *xmin = m_dX0; *ymax = m_dY0; *gsd = m_dGSD; }
	void SetGeoref(double xmin, double ymax, double gsd) { m_dX0 = xmin; m_dY0 = ymax; m_dGSD = gsd; }

	virtual uint32 PixSize();
	virtual byte* AllocArea(uint32 w, uint32 h);

	// Acces aux pixels
	virtual bool GetArea(XFile* file, uint32 x, uint32 y, uint32 w, uint32 h, byte* area) = 0;
	virtual bool GetLine(XFile* file, uint32 num, byte* area) = 0;
	virtual bool GetZoomArea(XFile* file, uint32 x, uint32 y, uint32 w, uint32 h, byte* area, uint32 factor) = 0;
	virtual bool GetRawArea(XFile* file, uint32 x, uint32 y, uint32 w, uint32 h, float* pix,
                          uint32* nb_sample, uint32 factor = 1);
  virtual bool GetRawPixel(XFile* file, uint32 x, uint32 y, uint32 win, double* pix, uint32* nb_sample);

	// Methodes statiques de manipulation de pixels
	static bool CMYK2RGB(byte* buffer, uint32 w, uint32 h);
	static bool Uint16To8bits(byte* buffer, uint32 w, uint32 h, uint16 min = 0, uint16 max = 0);
	static bool ZoomArea(byte* in, byte* out, uint32 win, uint32 hin, uint32 wout, uint32 hout, uint32 nbbyte);
	static void SwitchRGB2BGR(byte* buf, uint32 buf_size);
	static bool RotateArea(byte* in, byte* out, uint32 win, uint32 hin, uint32 nbbyte, uint32 rot);

};

#endif //XBASEIMAGE_H

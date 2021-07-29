//-----------------------------------------------------------------------------
//								XJpeg2000Image.h
//								================
//
// Auteur : F.Becirspahic - IGN / DSTI / SIMV
//
// Date : 21/06/2021
//-----------------------------------------------------------------------------

#ifndef XJPEG2000IMAGE_H
#define XJPEG2000IMAGE_H

// Include Kakadu
#undef byte
#include "kdu_file_io.h"
#include "kdu_region_compositor.h"

#ifndef byte
#define byte unsigned char
#endif

#include "XBaseImage.h"

class XKduRegionCompositor : public kdu_region_compositor {
public:
  virtual kdu_compositor_buf* allocate_buffer(kdu_coords min_size, kdu_coords& actual_size,
    bool read_access_required);
  virtual void delete_buffer(kdu_compositor_buf* buf) { delete buf; }
};


class XJpeg2000Image : public XBaseImage {
protected:
  bool											m_bValid;	// Indique si l'image est valide
  uint32										m_nNumli;	// Numero de la ligne active
  jpx_source*               m_Jpx_in;
  kdu_region_compositor*    m_Compositor;
  jp2_family_src*           m_Src;
  std::string							  m_strXmlMetadata;
  int                       m_nBitDepth;
  XKduRegionCompositor*     m_FloatCompositor;

  bool ReadGeorefXmlOld();
  bool ReadGeorefXml();
  bool ReadGeorefUuid(std::istream* uuid);

public:
  XJpeg2000Image(const char* filename);
  virtual ~XJpeg2000Image();

  virtual std::string Format() { return "JP2";}
  bool IsValid() { return m_bValid; }
  virtual inline bool NeedFile() { return false; }

  virtual bool GetArea(XFile* file, uint32 x, uint32 y, uint32 w, uint32 h, byte* area);
  virtual bool GetZoomArea(XFile* file, uint32 x, uint32 y, uint32 w, uint32 h, byte* area, uint32 factor);
  virtual bool GetJp2Area(uint32 x, uint32 y, uint32 w, uint32 h, byte* area,
                          uint32 factor = 1, uint32 wout = 0, uint32 hout = 0);
  virtual bool GetLine(XFile* file, uint32 num, byte* area) { return false; }

  virtual uint32 FileSize() { return 0; }

  // Donnees brutes
  virtual bool GetRawPixel(XFile* file, uint32 x, uint32 y, uint32 win, double* pix,
    uint32* nb_sample);
  virtual bool GetRawArea(XFile* file, uint32 x, uint32 y, uint32 w, uint32 h, float* pix,
    uint32* nb_sample, uint32 factor = 1);

  virtual std::string XmlMetadata() { return m_strXmlMetadata; }
};

#endif //XJPEG2000IMAGE_H

//-----------------------------------------------------------------------------
//								XFileImage.h
//								============
//
// Auteur : F.Becirspahic - IGN / DSTI / SIMV
//
// Date : 31/08/2021
//-----------------------------------------------------------------------------

#ifndef XFILEIMAGE_H
#define XFILEIMAGE_H

#include "XBaseImage.h"
#include "../XTool/XFile.h"

class XTransfo;
class XInterpol;

class XFileImage {
public:
  XFileImage();
  ~XFileImage() { Close(); }

  bool AnalyzeImage(std::string path);

  uint32 Width() { if (m_Image != NULL) return m_Image->W(); return 0; }
  uint32 Height() { if (m_Image != NULL) return m_Image->H(); return 0; }
  void GetGeoref(double* xmin, double* ymax, double* gsd)
  {
    if (m_Image == NULL) return; *xmin = m_Image->X0(); *ymax = m_Image->Y0(); *gsd = m_Image->GSD();
  }
  void SetGeoref(double xmin, double ymax, double gsd)
  {
    if (m_Image != NULL) m_Image->SetGeoref(xmin, ymax, gsd);
  }

  int NbByte();
  int NbChannel();
  uint16 NbBits() { if (m_Image == NULL) return 0; return m_Image->NbBits(); }
  uint16 NbSample() { if (m_Image == NULL) return 0; return m_Image->NbSample(); }
  std::string Format() { if (m_Image == NULL) return 0; return m_Image->Format(); }

  void Close();

  std::string GetMetadata() { if (m_Image == NULL) return ""; return m_Image->Metadata(); }
  std::string GetXmlMetadata() { if (m_Image == NULL) return ""; return m_Image->XmlMetadata(); }

  virtual bool GetArea(uint32 x, uint32 y, uint32 w, uint32 h, byte* area,
    bool BGROrder = false, uint32 offset = 0L);
  virtual bool GetZoomArea(uint32 x, uint32 y, uint32 w, uint32 h, byte* area,
    uint32 factor, bool BGROrder = false, uint32 offset = 0L);

  virtual bool GetRawPixel(uint32 x, uint32 y, uint32 win, double* pix, uint32* nb_sample);
  virtual bool GetRawArea(uint32 x, uint32 y, uint32 w, uint32 h, float* pix, uint32* nb_sample, uint32 factor = 1);

  // Rotation d'une zone de pixels
  bool RotateArea(byte* in, byte* out, uint32 win, uint32 hin, uint32 nbbyte, uint32 rot) const;

  // Creation d'un contexte
  inline bool SupportContext() { return false; }
  void SetContext(int numChannel, byte min, byte max, double gamma) { ; }

  // Allocation d'une ligne de pixels
  byte* AllocArea(uint32 w, uint32 h) { return new byte[w * h * NbByte()]; }

  // Fonction de reechantillonnage
  bool Resample(std::string file_out, XTransfo* transfo, XInterpol* inter, XWait* wait = NULL);

protected:
  XBaseImage* m_Image;
  std::string   m_strFilename;
  XFile         m_File;

  bool AnalyzeTiff();
  bool AnalyzeBigTiff();
  bool AnalyzeCog();
  bool AnalyzeJpeg2000();
};

#endif //XFILEIMAGE_H

//-----------------------------------------------------------------------------
//								XFileImage.cpp
//								==============
//
// Auteur : F.Becirspahic - IGN / DSTI / SIMV
//
// Date : 31/08/2021
//-----------------------------------------------------------------------------


#include "XFileImage.h"

#include "XTiffReader.h"
#include "XBigTiffReader.h"
#include "XTiffStripImage.h"
#include "XTiffTileImage.h"
#include "XCogImage.h"
#include "XJpeg2000Image.h"
#include "XDtmShader.h"
#include "../XTool/XTransfo.h"
#include "../XTool/XInterpol.h"
#include "../XTool/XTiffWriter.h"

//-----------------------------------------------------------------------------
// Constructeur
//-----------------------------------------------------------------------------
XFileImage::XFileImage()
{
  m_Image = NULL;
}

//-----------------------------------------------------------------------------
// Fermeture de l'image
//-----------------------------------------------------------------------------
void XFileImage::Close()
{
  m_File.Close();
  if (m_Image != NULL)
    delete m_Image;
  m_Image = NULL;
}

//-----------------------------------------------------------------------------
// Nombre d'octets utilises pour l'affiche de l'image
//-----------------------------------------------------------------------------
int XFileImage::NbByte()
{
  if (m_Image->ColorMapSize() > 0)
    return 3;
  if ((m_Image->NbSample() == 1) && (m_Image->NbBits() == 32))
    return 3;
  if (m_Image->NbSample() == 1)
    return 1;
  return 3;
}

int XFileImage::NbChannel()
{
  if (m_Image->ColorMapSize() > 0)
    return 3;
  if ((m_Image->NbSample() == 1) && (m_Image->NbBits() == 32))
    return 3;
  if (m_Image->NbSample() == 1)
    return 1;
  return 3;
}

//-----------------------------------------------------------------------------
// Analyse de l'image
//-----------------------------------------------------------------------------
bool XFileImage::AnalyzeImage(std::string path)
{
  m_strFilename = path;
  if (AnalyzeJpeg2000())
    return true;
  if (AnalyzeCog()) // COG avant le TIFF, car le COG est du TIFF
    return true;
  if (AnalyzeTiff())
    return true;
  if (AnalyzeBigTiff())
    return true;
  return false;
}

//-----------------------------------------------------------------------------
// Analyse d'une image TIFF
//-----------------------------------------------------------------------------
bool XFileImage::AnalyzeTiff()
{
  if (!m_File.Open(m_strFilename.c_str(), std::ios::in | std::ios::binary))
    return false;

  XTiffReader reader;
  if (!reader.Read(m_File.IStream())) {
    m_File.Close();
    return false;
  }

  if (!reader.AnalyzeIFD(m_File.IStream())) {
    m_File.Close();
    return false;
  }

  if (reader.RowsPerStrip() == 0) { // Tile
    XTiffTileImage* tile_image = new XTiffTileImage;
    if (!tile_image->SetTiffReader(&reader)) {
      delete tile_image;
      m_File.Close();
      return false;
    }
    m_Image = tile_image;
  }
  else {  // Strip
    XTiffStripImage* strip_image = new XTiffStripImage;
    if (!strip_image->SetTiffReader(&reader)) {
      delete strip_image;
      m_File.Close();
      return false;
    }
    m_Image = strip_image;
  }
  return true;
}

//-----------------------------------------------------------------------------
// Analyse d'une image BigTIFF
//-----------------------------------------------------------------------------
bool XFileImage::AnalyzeBigTiff()
{
  if (!m_File.Open(m_strFilename.c_str(), std::ios::in | std::ios::binary))
    return false;

  XBigTiffReader reader;
  if (!reader.Read(m_File.IStream())) {
    m_File.Close();
    return false;
  }

  if (!reader.AnalyzeIFD(m_File.IStream())) {
    m_File.Close();
    return false;
  }

  if (reader.RowsPerStrip() == 0) { // Tile
    XTiffTileImage* tile_image = new XTiffTileImage;
    if (!tile_image->SetTiffReader(&reader)) {
      delete tile_image;
      m_File.Close();
      return false;
    }
    m_Image = tile_image;
  }
  else {  // Strip
    XTiffStripImage* strip_image = new XTiffStripImage;
    if (!strip_image->SetTiffReader(&reader)) {
      delete strip_image;
      m_File.Close();
      return false;
    }
    m_Image = strip_image;
  }
  return true;
}

//-----------------------------------------------------------------------------
// Analyse d'une image JPEG2000
//-----------------------------------------------------------------------------
bool XFileImage::AnalyzeJpeg2000()
{
  XJpeg2000Image* image = new XJpeg2000Image(m_strFilename.c_str());
  if (image->IsValid() != true) {
    delete image;
    return false;
  }
  m_Image = image;
  return true;
}

//-----------------------------------------------------------------------------
// Analyse d'une image COG
//-----------------------------------------------------------------------------
bool XFileImage::AnalyzeCog()
{
  if (!m_File.Open(m_strFilename.c_str(), std::ios::in | std::ios::binary))
    return false;
  XCogImage* image = new XCogImage;
  if (image == NULL) {
    m_File.Close();
    return false;
  }
  try {
    if (!image->Open(&m_File)) {
      m_File.Close();
      delete image;
      return false;
    }
  }
  catch (const std::bad_alloc&) {
    m_File.Close();
  }
  m_Image = image;
  return true;
}

//-----------------------------------------------------------------------------
// Acces aux pixels
//-----------------------------------------------------------------------------
bool XFileImage::GetArea(uint32 x, uint32 y, uint32 w, uint32 h, byte* area,
  bool BGROrder, uint32 offset)
{
  if (m_Image == NULL)
    return false;
  if (m_Image->ColorMapSize() == 0) {
    if ((m_Image->NbBits() <= 8) && (m_Image->NbSample() == 1)) {  // Niveaux de gris
      return m_Image->GetArea(&m_File, x, y, w, h, area);
    }
    if ((m_Image->NbBits() == 8) && (m_Image->NbSample() == 3)) {  // RGB
      return m_Image->GetArea(&m_File, x, y, w, h, area);
    }
  }
  // Cas necessitant un passage en RGB
  byte* val = m_Image->AllocArea(w, h);
  if (val == NULL)
    return false;
  if (!m_Image->GetArea(&m_File, x, y, w, h, val)) {
    delete[] val;
    return false;
  }

  if (m_Image->ColorMapSize() > 0) {  // Image palette
    m_Image->ApplyColorMap(val, area, w, h);
    delete[] val;
    return true;
  }

  if ((m_Image->NbBits() == 8) && (m_Image->NbSample() == 4)) {  // CYMK
    XBaseImage::CMYK2RGB(val, w, h);
    ::memcpy(area, val, w * h * 3L);
    delete[] val;
    return true;
  }

  if ((m_Image->NbBits() == 16) && (m_Image->NbSample() == 1)) {  // Image 16 bits
    XBaseImage::Uint16To8bits(val, w, h);
    ::memcpy(area, val, w * h);
    delete[] val;
    return true;
  }

  if ((m_Image->NbSample() == 1) && (m_Image->NbBits() == 32)) { // MNT 32 bits
    XDtmShader dtm(m_Image->GSD());
    dtm.ConvertArea((float*)val, w, h, area);
    delete[] val;
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
// Acces aux pixels avec un facteur de zoom
//-----------------------------------------------------------------------------
bool XFileImage::GetZoomArea(uint32 x, uint32 y, uint32 w, uint32 h, byte* area,
  uint32 factor, bool BGROrder, uint32 offset)
{
  if (m_Image == NULL)
    return false;
  if (m_Image->ColorMapSize() == 0) {
    if ((m_Image->NbBits() <= 8) && (m_Image->NbSample() == 1)) { // Niveaux de gris
      return m_Image->GetZoomArea(&m_File, x, y, w, h, area, factor);
    }
    if ((m_Image->NbBits() == 8) && (m_Image->NbSample() == 3)) { // RGB
      return m_Image->GetZoomArea(&m_File, x, y, w, h, area, factor);
    }
  }

  // Cas necessitant un passage en RGB
  uint32 wout = w / factor, hout = h / factor;
  byte* val = m_Image->AllocArea(wout, hout);
  if (val == NULL)
    return false;
  if (!m_Image->GetZoomArea(&m_File, x, y, w, h, val, factor)) {
    delete[] val;
    return false;
  }

  if (m_Image->ColorMapSize() > 0) {  // Image palette
    m_Image->ApplyColorMap(val, area, wout, hout);
    delete[] val;
    return true;
  }
  if ((m_Image->NbBits() == 8) && (m_Image->NbSample() == 4)) {  // CYMK
    XBaseImage::CMYK2RGB(val, wout, hout);
    ::memcpy(area, val, wout * hout * 3L);
    delete[] val;
    return true;
  }
  if ((m_Image->NbBits() == 16) && (m_Image->NbSample() == 1)) {  // Image 16 bits
    XBaseImage::Uint16To8bits(val, wout, hout);
    ::memcpy(area, val, wout * hout);
    delete[] val;
    return true;
  }

  if ((m_Image->NbSample() == 1) && (m_Image->NbBits() == 32)) { // MNT 32 bits
    XDtmShader dtm(m_Image->GSD() * factor);
    dtm.ConvertArea((float*)val, wout, hout, area);
    delete[] val;
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
// Acces aux pixels en valeurs brutes avec un facteur de zoom
//-----------------------------------------------------------------------------
bool XFileImage::GetRawArea(uint32 x, uint32 y, uint32 w, uint32 h, float* pix, uint32* nb_sample, uint32 factor)
{
  if (m_Image == NULL)
    return false;
  return m_Image->GetRawArea(&m_File, x, y, w, h, pix, nb_sample, factor);
}

bool XFileImage::GetRawPixel(uint32 x, uint32 y, uint32 win, double* pix, uint32* nb_sample)
{
  if (m_Image == NULL)
    return false;
  return m_Image->GetRawPixel(&m_File, x, y, win, pix, nb_sample);
}

//-----------------------------------------------------------------------------
// Rotation de l'image
//-----------------------------------------------------------------------------
bool XFileImage::RotateArea(byte* in, byte* out, uint32 win, uint32 hin, uint32 nbbyte, uint32 rot) const
{
  return XBaseImage::RotateArea(in, out, win, hin, nbbyte, rot);
}

//-----------------------------------------------------------------------------
//	Fonction de re-echantillonnage
//-----------------------------------------------------------------------------
bool XFileImage::Resample(std::string file_out, XTransfo* transfo, XInterpol* interpol, XWait* wait)
{
  int i, j, k, lg = interpol->Win();
  double xf, yf, xi, yi, x, y, value;
  int line, col, ycur, ytmp, xcur, nb_canal, canal, W, H;

  double* inter;
  double* val;
  byte** buf;
  byte* out;
  byte white = 255;

  XTiffWriter tiff;

  if (m_Image->ColorMapSize() == 256 * 256 * 256) {	// Recherche du blanc pour les images avec palettes
    uint16* colormap = m_Image->ColorMap();
    for (uint32 i = 0; i < 256; i++)
      if (colormap[i] == 255)
        if (colormap[i + 256] == 255)
          if (colormap[i + 512] == 255) {
            white = i;
            break;
          }
    tiff.SetColorMap(colormap);
  }

  transfo->Dimension(Width(), Height(), &W, &H);
  nb_canal = NbChannel();

  // Creation de l'entete de l'image en sortie
  double xmin, ymax, gsd;
  uint16 espg;
  if (transfo->SetGeoref(&xmin, &ymax, &gsd, &espg))
    tiff.SetGeoTiff(xmin, ymax, gsd, espg);
  if (!tiff.Write(file_out.c_str(), W, H, NbChannel(), NbBits()))
    return false;

  // Ecriture de la palette ...

  std::ofstream fic_out;
  fic_out.open(file_out.c_str(), std::ios_base::out | std::ios_base::binary | std::ios_base::app);
  if (!fic_out.good())
    return false;
  fic_out.seekp(0, std::ios_base::end);

  // Allocation des tableaux
  typedef byte* byteptr;
  buf = new byteptr[lg * 2];
  for (i = 0; i < lg * 2; i++)
    buf[i] = new byte[Width() * nb_canal];
  out = new byte[W * nb_canal];
  val = new double[2 * lg];
  inter = new double[2 * lg];

  // Gestion
  if (wait != NULL)
    wait->SetRange(0, H);

  // Debut de l'iteration ligne par ligne de l'image finale
  for (line = 0; line < H; line++) {
    xf = 0;
    yf = line;
    transfo->Direct(xf, yf, &xi, &yi);
    ycur = (int)yi;
    j = 0;
    if ((ycur >= lg - 1) && (ycur < (int)Height() - lg)) {
      for (i = ycur - lg + 1; i <= ycur + lg; i++)
        //GetLine(i, buf[j++]);
        GetArea(0, i, Width(), 1, buf[j++]);
    }

    for (col = 0; col < W; col++) {
      xf = col;
      transfo->Direct(xf, yf, &xi, &yi);
      ytmp = (int)yi;
      if (ycur != ytmp) {
        ycur = ytmp;
        j = 0;
        if ((ycur >= lg - 1) && (ycur < (int)Height() - lg))
          for (i = ycur - lg + 1; i <= ycur + lg; i++)
            //GetLine(i, buf[j++]);
            GetArea(0, i, Width(), 1, buf[j++]);
      }
      xcur = (int)xi;
      for (canal = 0; canal < nb_canal; canal++) {
        // Gestion des pixels hors zone
        if ((xcur < lg - 1) || (xcur > (int)Width() - lg) ||
          (ycur < lg - 1) || (ycur > (int)Height() - lg)) {
          out[nb_canal * col + canal] = white;
          continue;
        }

        x = xi - xcur;
        y = yi - ycur;

        for (k = 0; k < 2 * lg; k++) {
          j = 0;
          for (i = xcur - lg + 1; i <= xcur + lg; i++)
            val[j++] = buf[k][i * nb_canal + canal];
          inter[k] = interpol->Compute(val, x);
        }

        value = interpol->Compute(inter, y);
        if ((value - floor(value) < 0.5))
          out[nb_canal * col + canal] = (byte)floor(value);
        else
          out[nb_canal * col + canal] = (byte)ceil(value);

        if (value < 0.1)
          out[nb_canal * col + canal] = 0;
        if (value > 254.9)
          out[nb_canal * col + canal] = 255;
      }
    }
    fic_out.write((char*)out, W * nb_canal * sizeof(byte));
    if (wait != NULL) {
      wait->StepIt();
      if (wait->CheckCancel())
        break;
    }
  }
  // Liberation de la memoire allouee
  for (i = 0; i < 2 * lg; i++)
    delete[] buf[i];
  delete[] buf;
  delete[] out;
  delete[] val;
  delete[] inter;

  fic_out.close();
  return true;
}


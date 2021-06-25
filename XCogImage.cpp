//-----------------------------------------------------------------------------
//								XCogImage.cpp
//								=============
//
// Auteur : F.Becirspahic - IGN / DSTI / SIMV
//
// Date : 24/06/2021
//-----------------------------------------------------------------------------

#include "XCogImage.h"


//-----------------------------------------------------------------------------
// Ouverture d'une image COG
//-----------------------------------------------------------------------------
void XCogImage::Clear()
{
  m_nW = m_nH = 0;
  m_nNbBits = m_nNbSample = 0;
  m_dX0 = m_dY0 = m_dGSD = 0.;
  m_Factor.clear();
  for (uint32 i = 0; i < m_TImages.size(); i++)
    delete m_TImages[i];
  m_TImages.clear();
}

//-----------------------------------------------------------------------------
// Ouverture d'une image COG
//-----------------------------------------------------------------------------
bool XCogImage::Open(XFile* file)
{
  Clear();

  if (!m_Reader.Read(file->IStream()))
    return false;
  if (m_Reader.NbIFD() < 2) // Image TIFF standard
    return false;

  // Analyse des IFD
  m_Reader.SetActiveIFD(0);
  if (!m_Reader.AnalyzeIFD(file->IStream()))
    return false;
  if (m_Reader.RowsPerStrip() > 0)  // Image en strip => pas du COG
    return false;

  m_nW = m_Reader.Width();
  m_nH = m_Reader.Height();
  m_nNbBits = m_Reader.NbBits();
  m_nNbSample = m_Reader.NbSample();
  m_dX0 = m_Reader.X0();
  m_dY0 = m_Reader.Y0();
  m_dGSD = m_Reader.GSD();

  m_Factor.push_back(1);
  for (uint32 i = 1; i < m_Reader.NbIFD(); i++) {
    m_Reader.SetActiveIFD(i);
    if (!m_Reader.AnalyzeIFD(file->IStream()))
      return false;
    if ((m_Reader.Width() == 0) || (m_Reader.Height() == 0))
      break;
    uint32 factorW = XRint((double)m_nW / (double)m_Reader.Width());
    uint32 factorH = XRint((double)m_nH / (double)m_Reader.Height());
    if (factorW != factorH) {
      Clear();
      return false;
    }
    m_Factor.push_back(factorW);
  }

  // Creation des images
  for (uint32 i = 0; i < m_Factor.size(); i++) {
    XTiffTileImage* image = new XTiffTileImage;
    if (image == NULL) {
      Clear();
      return false;
    }
    m_Reader.SetActiveIFD(i);
    m_Reader.AnalyzeIFD(file->IStream());
    if (!image->SetTiffReader(&m_Reader)) {
      delete image;
      Clear();
      return false;
    }
    m_TImages.push_back(image);
  }
  return true;
}

//-----------------------------------------------------------------------------
// lecture d'une ROI
//-----------------------------------------------------------------------------
bool XCogImage::GetArea(XFile* file, uint32 x, uint32 y, uint32 w, uint32 h, byte* area)
{
  if (m_TImages.size() < 1)
    return false;
  XTiffTileImage* image = m_TImages[0];
  if (image == NULL)
    return false;
  return image->GetArea(file, x, y, w, h, area);
}

//-----------------------------------------------------------------------------
// Ouverture d'une image COG
//-----------------------------------------------------------------------------
bool XCogImage::GetLine(XFile* file, uint32 num, byte* area)
{
  if (m_TImages.size() < 1)
    return false;
  XTiffTileImage* image = m_TImages[0];
  if (image == NULL)
    return false;
  return image->GetLine(file, num, area);
}

//-----------------------------------------------------------------------------
// Ouverture d'une image COG
//-----------------------------------------------------------------------------
bool XCogImage::GetZoomArea(XFile* file, uint32 x, uint32 y, uint32 w, uint32 h, byte* area, uint32 factor)
{
  if (m_TImages.size() < 1)
    return false;
  uint32 index = 0;
  for (uint32 i = 0; i < m_Factor.size(); i++) {
    if (factor == m_Factor[i]) {
      index = i;
      break;
    }
    if (factor > m_Factor[i])
      index = i;
  }
  if (index >= m_TImages.size())
    return false;
  XTiffTileImage* image = m_TImages[index];
  if (image == NULL)
    return false;
  if (factor == m_Factor[index])
    return image->GetArea(file, x / m_Factor[index], y / m_Factor[index], w / m_Factor[index], h / m_Factor[index],
                            area);
  byte* buffer = image->AllocArea(w / m_Factor[index], h / m_Factor[index]);
  if (buffer == NULL)
    return false;
  image->GetArea(file, x / m_Factor[index], y / m_Factor[index], w / m_Factor[index], h / m_Factor[index],
                              buffer);
  ZoomArea(buffer, area, w / m_Factor[index], h / m_Factor[index], w / factor, h / factor, PixSize());
  delete[] buffer;
  return true;
}

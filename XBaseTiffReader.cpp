//-----------------------------------------------------------------------------
//								XBaseTiffReader.cpp
//								===================
//
// Auteur : F.Becirspahic - IGN / DSTI / SIMV
//
// Date : 21/07/2021
//-----------------------------------------------------------------------------

#include <cstring>
#include "XBaseTiffReader.h"



//-----------------------------------------------------------------------------
// Nettoyage des donnees
//-----------------------------------------------------------------------------
void XBaseTiffReader::Clear()
{
  m_nWidth = m_nHeight = m_nRowsPerStrip = m_nTileWidth = m_nTileHeight = 0;
  m_nNbStripOffsets = m_nNbStripCounts = m_nNbTileOffsets = m_nNbTileCounts = 0;
  if (m_StripOffsets != NULL) {
    delete[] m_StripOffsets;
    m_StripOffsets = NULL;
  }
  if (m_StripCounts != NULL) {
    delete[] m_StripCounts;
    m_StripCounts = NULL;
  }
  if (m_TileOffsets != NULL) {
    delete[] m_TileOffsets;
    m_TileOffsets = NULL;
  }
  if (m_TileCounts != NULL) {
    delete[] m_TileCounts;
    m_TileCounts = NULL;
  }
  if (m_ColorMap != NULL) {
    delete[] m_ColorMap;
    m_ColorMap = NULL;
  }
  if (m_JpegTables != NULL) {
    delete[] m_JpegTables;
    m_JpegTables = NULL;
  }
  m_nJpegTablesSize = 0;
  m_YCbCrSub[0] = m_YCbCrSub[1] = 2;
}

//-----------------------------------------------------------------------------
// Taille des types TIFF
//-----------------------------------------------------------------------------
uint32 XBaseTiffReader::TypeSize(eDataType type)
{
  switch (type) {
  case BYTE:  return sizeof(byte);
  case ASCII:	return sizeof(byte);
  case SHORT:	return sizeof(uint16);
  case LONG:	return sizeof(uint32);
  case RATIONAL: return 2 * sizeof(uint32);
    /* Types version 6 des specifications TIFF */
  case SBYTE: return sizeof(byte);
  case UNDEFINED: return sizeof(byte);
  case SSHORT: return sizeof(uint16);
  case SLONG: return sizeof(uint32);
  case SRATIONAL: return 2 * sizeof(uint32);
  case FLOAT: return sizeof(float);
  case DOUBLE: return sizeof(double);
    /* Types BigTiff */
  case LONG8: return sizeof(uint64);
  case SLONG8: return sizeof(uint64);
  case IFD8: return sizeof(uint64);
  }
  return 0;
}

//-----------------------------------------------------------------------------
// Renvoie les informations sur les strips
//-----------------------------------------------------------------------------
bool XBaseTiffReader::GetStripInfo(uint32* nbStrip, uint64** offset, uint64** count)
{
  if (m_nNbStripOffsets < 1)
    return false;
  if (m_nNbStripOffsets != m_nNbStripCounts)
    return false;
  *nbStrip = m_nNbStripOffsets;
  *offset = new uint64[m_nNbStripOffsets];
  if (*offset == NULL)
    return false;
  *count = new uint64[m_nNbStripCounts];
  if (*count == NULL) {
    delete[] * offset;
    return false;
  }
  ::memcpy(*offset, m_StripOffsets, m_nNbStripOffsets * sizeof(uint64));
  ::memcpy(*count, m_StripCounts, m_nNbStripCounts * sizeof(uint64));
  return true;
}

//-----------------------------------------------------------------------------
// Renvoie les informations sur les tiles
//-----------------------------------------------------------------------------
bool XBaseTiffReader::GetTileInfo(uint32* nbTile, uint64** offset, uint64** count)
{
  if (m_nNbTileOffsets < 1)
    return false;
  if (m_nNbTileOffsets != m_nNbTileCounts)
    return false;
  *nbTile = m_nNbTileOffsets;
  *offset = new uint64[m_nNbTileOffsets];
  if (*offset == NULL)
    return false;
  *count = new uint64[m_nNbTileCounts];
  if (*count == NULL) {
    delete[] * offset;
    return false;
  }
  ::memcpy(*offset, m_TileOffsets, m_nNbTileOffsets * sizeof(uint64));
  ::memcpy(*count, m_TileCounts, m_nNbTileCounts * sizeof(uint64));
  return true;
}

//-----------------------------------------------------------------------------
// Impression des informations sur l'image
//-----------------------------------------------------------------------------
void XBaseTiffReader::PrintInfo(std::ostream* out)
{
  *out << "Largeur : " << m_nWidth << "\tHauteur : " << m_nHeight << std::endl;
  *out << "Largeur des dalles " << m_nTileWidth << "\tHauteur des dalles : " << m_nTileHeight << std::endl;
  *out << "Nb de samples : " << m_nNbSample << "\tBits / sample : " << m_nNbBits << std::endl;
  *out << "Interpretation photometrique : " << PhotIntString(m_nPhotInt) << std::endl;
  *out << "Configuration planaire : " << m_nPlanarConfig << std::endl;
  *out << "Compression : " << m_nCompression << " " << CompressionString(m_nCompression) << std::endl;
  *out << "RowPerStrip : " << m_nRowsPerStrip << std::endl;
  *out << "Nb de strips : " << m_nNbStripOffsets << "\tNb de dalles : " << m_nNbTileOffsets << std::endl;
  if (m_nNbStripOffsets > 0) {
    for (uint32 i = 0; i < XMin((uint32)5, m_nNbStripOffsets); i++)
      *out << "StripOffset[" << i << "] = " << m_StripOffsets[i] << "\t" << m_StripCounts[i] << std::endl;
  }
  if (m_nNbTileOffsets > 0) {
    for (uint32 i = 0; i < XMin((uint32)5, m_nNbTileOffsets); i++)
      *out << "TileOffset[" << i << "] = " << m_TileOffsets[i] << "\t" << m_TileCounts[i] << std::endl;
  }
  *out << "Resolution terrain : " << m_dGSD << std::endl;
  *out << "X0 : " << m_dX0 << "\tY0 : " << m_dY0 << std::endl;
}

//-----------------------------------------------------------------------------
// Type de compression
//-----------------------------------------------------------------------------
std::string XBaseTiffReader::CompressionString(uint16 compression)
{
  switch ((eCompression)compression) {
  case UNCOMPRESSED1 :
  case UNCOMPRESSED2 :
    return "Sans compression";
  case CCITT1D: return "CCITT1D";
  case CCITTGROUP3: return "CCITTGROUP3";
  case CCITTGROUP4: return "CCITTGROUP4";
  case LZW: return "LZW";
  case JPEG: return "JPEG";
  case JPEGv2: return "JPEGv2";
  case PACKBITS: return "PACKBITS";
  case DEFLATE: return "DEFLATE";
  }
  return "Inconnue";
}

//-----------------------------------------------------------------------------
// Type d'interpretation photometrique
//-----------------------------------------------------------------------------
std::string XBaseTiffReader::PhotIntString(uint16 photint)
{
  switch ((ePhotInt)photint) {
  case WHITEISZERO: return "White is 0";
  case BLACKISZERO: return "Black is 0";
  case RGBPHOT: return "RGB";
  case RGBPALETTE: return "RGB Palette";
  case TRANSPARENCYMASK: return "Transparency Mask";
  case CMYKPHOT: return "CYMK";
  case YCBCR: return "YCbCr";
  case CIELAB: return "CieLAB";
  }
  return "Inconnue";
}

//-----------------------------------------------------------------------------
// Renvoie la table des couleurs pour les images palettes
//-----------------------------------------------------------------------------
bool XBaseTiffReader::GetColorMap(uint16** map, uint16* mapsize)
{
  if ((m_ColorMap == NULL)||(m_nColorMapSize == 0))
    return false;
  *map = new uint16[m_nColorMapSize];
  if (*map == NULL)
    return false;
  std::memcpy(*map, m_ColorMap, m_nColorMapSize * sizeof(uint16));
  *mapsize = m_nColorMapSize;
  return true;
}

//-----------------------------------------------------------------------------
// Renvoie les informations sur les tables JPEG
//-----------------------------------------------------------------------------
bool XBaseTiffReader::GetJpegTablesInfo(byte** tables, uint32* size)
{
  if ((m_nJpegTablesSize == 0) || (m_JpegTables == NULL))
    return false;
  *tables = new byte[m_nJpegTablesSize];
  if (*tables == NULL)
    return false;
  std::memcpy(*tables, m_JpegTables, m_nJpegTablesSize);
  *size = m_nJpegTablesSize;
  return true;
}


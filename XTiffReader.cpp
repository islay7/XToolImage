//-----------------------------------------------------------------------------
//								XTiffReader.cpp
//								===============
//
// Auteur : F.Becirspahic - IGN / DSTI / SIMV
//
// Date : 19/05/2021
//-----------------------------------------------------------------------------

#include <cstring>
#include "XTiffReader.h"

//-----------------------------------------------------------------------------
// Lecture de l'entete du fichier TIFF
//-----------------------------------------------------------------------------
bool XTiffReader::ReadHeader(std::istream* in)
{
  uint16 identifier, version;
  in->read((char*)&identifier, sizeof(uint16));
  if ((identifier != 0x4949) && (identifier != 0x4D4D))
    return false;
  if (identifier == 0x4949) 
    m_bByteOrder = true; // LSB_FIRST;
  if (identifier == 0x4D4D)
    m_bByteOrder = false; // MSB_FIRST;

  m_Endian.Read(in, m_bByteOrder, &version, sizeof(version));
  m_Endian.Read(in, m_bByteOrder, &m_nIFDOffset, sizeof(m_nIFDOffset));

  if (version != 42)
    return false;
  return true;
}

//-----------------------------------------------------------------------------
// Lecture d'un IFD
//-----------------------------------------------------------------------------
bool XTiffReader::ReadIFD(std::istream* in, uint32 offset, TiffIFD* ifd)
{
  uint16 nb_entries;
  in->seekg(offset);
  m_Endian.Read(in, m_bByteOrder, &nb_entries, sizeof(nb_entries));
 
  for (uint16 i = 0; i < nb_entries; i++) {
    TiffTag tag;
    m_Endian.Read(in, m_bByteOrder, &tag.TagId, sizeof(tag.TagId));
    m_Endian.Read(in, m_bByteOrder, &tag.DataType, sizeof(tag.DataType));
    m_Endian.Read(in, m_bByteOrder, &tag.DataCount, sizeof(tag.DataCount));
    m_Endian.Read(in, m_bByteOrder, &tag.DataOffset, sizeof(tag.DataOffset));
    ifd->TagList.push_back(tag);
  }
  m_Endian.Read(in, m_bByteOrder, &ifd->NextIFDOffset, sizeof(ifd->NextIFDOffset));
  return in->good();
}

//-----------------------------------------------------------------------------
// Lecture
//-----------------------------------------------------------------------------
bool XTiffReader::Read(std::istream* in)
{
  if (!ReadHeader(in))
    return false;
  m_IFD.clear();
  uint32 offset = m_nIFDOffset;
  while (offset != 0) {
    TiffIFD ifd;
    if (!ReadIFD(in, offset, &ifd))
      break;
    m_IFD.push_back(ifd);
    offset = ifd.NextIFDOffset;
  }
  m_nActiveIFD = 0;
  return in->good();
}

//-----------------------------------------------------------------------------
// Nettoyage des donnees
//-----------------------------------------------------------------------------
void XTiffReader::Clear()
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
// Reherche d'un tag
//-----------------------------------------------------------------------------
bool XTiffReader::FindTag(eTagID id, TiffTag* T)
{
  for (uint32 i = 0; i < m_IFD[m_nActiveIFD].TagList.size(); i++) {
    if (m_IFD[m_nActiveIFD].TagList[i].TagId == id) {
      *T = m_IFD[m_nActiveIFD].TagList[i];
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
// Taille des types TIFF
//-----------------------------------------------------------------------------
uint32 XTiffReader::TypeSize(eDataType type)
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
  }
  return 0;
}

//-----------------------------------------------------------------------------
// Taille des donnees contenues dans un tag
//-----------------------------------------------------------------------------
uint32 XTiffReader::DataSize(TiffTag* T)
{
  return T->DataCount * TypeSize((eDataType)T->DataType);
}

//-----------------------------------------------------------------------------
// Lecture des donnees stockees dans un tag
//-----------------------------------------------------------------------------
uint32 XTiffReader::ReadDataInTag(TiffTag* T)
{
  uint16* retour;

  if (DataSize(T) <= 4) {
    switch (T->DataType) {
    case BYTE: return T->DataOffset;
    case ASCII:	return T->DataOffset;
    case SHORT:
      retour = (uint16*)&T->DataOffset;
      if (m_Endian.ByteOrder() == m_bByteOrder)
        return retour[0];
      else
        return retour[1];
      break;
      return T->DataOffset;
    case LONG: return T->DataOffset;
    } /* endswitch */
  }
  return 0;
}

//-----------------------------------------------------------------------------
// Lecture des donnees stockees dans un tag
//-----------------------------------------------------------------------------
uint32 XTiffReader::ReadIdTag(eTagID id, uint32 defaut)
{
  TiffTag T;
  if (!FindTag(id, &T))
    return defaut;
  return ReadDataInTag(&T);
}

//-----------------------------------------------------------------------------
// Lecture d'un tableau de donnees
//-----------------------------------------------------------------------------
bool XTiffReader::ReadDataArray(std::istream* in, eTagID id, void* V, int size)
{
  TiffTag T;
  if (!FindTag(id, &T))
    return false;
  uint32 datasize = DataSize(&T);
  if (datasize <= 4) {
    ::memcpy(V, &T.DataOffset, datasize);
    return true;
  }
  in->seekg(T.DataOffset);
  m_Endian.ReadArray(in, m_bByteOrder, V, size, T.DataCount);
 
  return in->good();
}

//-----------------------------------------------------------------------------
// Lecture d'un tableau de SHORT ou LONG
//-----------------------------------------------------------------------------
uint32* XTiffReader::ReadUintArray(std::istream* in, eTagID id, uint32* nb_elt)
{
  TiffTag T;
  if (!FindTag(id, &T))
    return NULL;
  uint32 datasize = DataSize(&T);
  if ((T.DataType != SHORT) && (T.DataType != LONG))
    return NULL;
  uint32* V = new uint32[T.DataCount];
  if (V == NULL)
    return NULL;
  *nb_elt = T.DataCount;
  if (datasize <= 4) {  // Cas ou le DataOffset contient le tableau
    if (T.DataType == LONG) { V[0] = T.DataOffset; return V; }
    uint16* ptr = (uint16*)&(T.DataOffset);
    V[0] = *ptr; ptr++;
    if (T.DataCount > 1) V[1] = *ptr;
    return V;
  }
  in->seekg(T.DataOffset);
  if (T.DataType == LONG) {
    m_Endian.ReadArray(in, m_bByteOrder, V, sizeof(uint32), T.DataCount);
    return V;
  }
  // Cas SHORT
  uint16* data = new uint16[T.DataCount];
  if (data == NULL) {
    delete[] V;
    return NULL;
  }
  m_Endian.ReadArray(in, m_bByteOrder, data, sizeof(uint16), T.DataCount);
  for (uint32 i = 0; i < T.DataCount; i++)
    V[i] = data[i];
  delete[] data;
  return V;
}

//-----------------------------------------------------------------------------
// Analyse d'un IFD
//-----------------------------------------------------------------------------
bool XTiffReader::AnalyzeIFD(std::istream* in)
{
  TiffTag T;
  Clear();

  m_nWidth = ReadIdTag(IMAGEWIDTH);
  m_nHeight = ReadIdTag(IMAGEHEIGHT);
  m_nRowsPerStrip = ReadIdTag(ROWSPERSTRIP);
  m_nTileWidth = ReadIdTag(TILEWIDTH);
  m_nTileHeight = ReadIdTag(TILELENGTH);

  m_nNbBits = m_nNbSample = m_nCompression = m_nPredictor = m_nPhotInt = m_nPlanarConfig = 0;
  m_nNbSample = (uint16)ReadIdTag(SAMPLESPERPIXEL, 1);
  m_nCompression = (uint16)ReadIdTag(COMPRESSION, 1);
  m_nPredictor = (uint16)ReadIdTag(PREDICTOR, 1);
  m_nPhotInt = (uint16)ReadIdTag(PHOTOMETRICINTERPRETATION);
  m_nPlanarConfig = (uint16)ReadIdTag(PLANARCONFIGURATION, 1);

  // Lecture du nombre de bits / sample
  if (m_nNbSample == 1)
    m_nNbBits = (uint16)ReadIdTag(BITSPERSAMPLE, 1);
  else {
    uint32 nbBits;
    uint32* bits = ReadUintArray(in, BITSPERSAMPLE, &nbBits);
    if (bits != NULL) {
      m_nNbBits = bits[0];
      delete[] bits;
    }
  }

  // Lecture des tableaux
  m_StripOffsets = ReadUintArray(in, STRIPOFFSETS, &m_nNbStripOffsets);
  m_StripCounts = ReadUintArray(in, STRIPBYTECOUNTS, &m_nNbStripCounts);
  m_TileOffsets = ReadUintArray(in, TILEOFFSETS, &m_nNbTileOffsets);
  m_TileCounts = ReadUintArray(in, TILEBYTECOUNTS, &m_nNbTileCounts);

  // Lecture de la table des couleurs (image palette)
  if (m_nPhotInt == (uint16)RGBPALETTE) {
    if (FindTag(COLORMAP, &T)) {
      m_ColorMap = new uint16[T.DataCount];
      if (!ReadDataArray(in, COLORMAP, m_ColorMap, sizeof(uint16))) {
        delete[] m_ColorMap;
        m_ColorMap = NULL;
      }
    }
  }

  // Lecture des tables JPEG
  if (FindTag(JPEGTABLES, &T)) {
    m_JpegTables = new byte[T.DataCount];
    if (!ReadDataArray(in, JPEGTABLES, m_JpegTables, sizeof(byte))) {
      delete[] m_JpegTables;
      m_JpegTables = NULL;
    }
    else
      m_nJpegTablesSize = T.DataCount;
  }
  if (FindTag(YCBCRSUBSAMPLING, &T)) {
    if (T.DataCount == 2)
      ReadDataArray(in, YCBCRSUBSAMPLING, m_YCbCrSub, sizeof(uint16));
  }
  if (FindTag(REFERENCEBLACKWHITE, &T)) {
    if (T.DataCount == 6) {
      ReadDataArray(in, REFERENCEBLACKWHITE, m_ReferenceBlack, 2*sizeof(uint32));
    }
  }

  // Lecture du GEOTIFF
  m_dGSD = 0.;
  m_dX0 = 0.;
  m_dY0 = 0.;
  double* data = NULL;
  
  if (FindTag(ModelPixelScaleTag, &T)) {
    if (T.DataCount == 3) {
      data = new double[T.DataCount];
      if (ReadDataArray(in, ModelPixelScaleTag, data, sizeof(double))) {
        m_dGSD = data[0];
      }
      delete[] data;
    }
  }
  if (FindTag(ModelTiepointTag, &T)) {
    if (T.DataCount >= 6) {
      data = new double[T.DataCount];
      if (ReadDataArray(in, ModelTiepointTag, data, sizeof(double))) {
        m_dX0 = data[3] - data[0] * m_dGSD;
        m_dY0 = data[4] - data[1] * m_dGSD;
      }
      delete[] data;
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
// Impression des informations sur l'image
//-----------------------------------------------------------------------------
void XTiffReader::PrintInfo(std::ostream* out)
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
// Impression des informations brutes des IFD
//-----------------------------------------------------------------------------
void XTiffReader::PrintIFDTag(std::ostream* out)
{
  for (uint32 i = 0; i < m_IFD.size(); i++) {
    *out << "*********" << std::endl << "IFD " << i << "\tType\tCount\tOffset" << std::endl;
    for (uint32 j = 0; j < m_IFD[i].TagList.size(); j++) {
      *out << m_IFD[i].TagList[j].TagId << "\t" << m_IFD[i].TagList[j].DataType << "\t"
        << m_IFD[i].TagList[j].DataCount << "\t" << m_IFD[i].TagList[j].DataOffset << std::endl;
    }
  }
}

//-----------------------------------------------------------------------------
// Type de compression
//-----------------------------------------------------------------------------
std::string XTiffReader::CompressionString(uint16 compression)
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
std::string XTiffReader::PhotIntString(uint16 photint)
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
// Renvoie les informations sur les strips
//-----------------------------------------------------------------------------
bool XTiffReader::GetStripInfo(uint32* nbStrip, uint32** offset, uint32** count)
{
  if (m_nNbStripOffsets < 1)
    return false;
  if (m_nNbStripOffsets != m_nNbStripCounts)
    return false;
  *nbStrip = m_nNbStripOffsets;
  *offset = new uint32[m_nNbStripOffsets];
  if (*offset == NULL)
    return false;
  *count = new uint32[m_nNbStripCounts];
  if (*count == NULL) {
    delete[] *offset;
    return false;
  }
  ::memcpy(*offset, m_StripOffsets, m_nNbStripOffsets * sizeof(uint32));
  ::memcpy(*count, m_StripCounts, m_nNbStripCounts * sizeof(uint32));
  return true;
}

//-----------------------------------------------------------------------------
// Renvoie les informations sur les tiles
//-----------------------------------------------------------------------------
bool XTiffReader::GetTileInfo(uint32* nbTile, uint32** offset, uint32** count)
{
  if (m_nNbTileOffsets < 1)
    return false;
  if (m_nNbTileOffsets != m_nNbTileCounts)
    return false;
  *nbTile = m_nNbTileOffsets;
  *offset = new uint32[m_nNbTileOffsets];
  if (*offset == NULL)
    return false;
  *count = new uint32[m_nNbTileCounts];
  if (*count == NULL) {
    delete[] * offset;
    return false;
  }
  ::memcpy(*offset, m_TileOffsets, m_nNbTileOffsets * sizeof(uint32));
  ::memcpy(*count, m_TileCounts, m_nNbTileCounts * sizeof(uint32));
  return true;
}

//-----------------------------------------------------------------------------
// Renvoie les informations sur les tables JPEG
//-----------------------------------------------------------------------------
bool XTiffReader::GetJpegTablesInfo(byte** tables, uint32* size)
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


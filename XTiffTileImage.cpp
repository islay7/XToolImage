//-----------------------------------------------------------------------------
//								XTiffTileImage.cpp
//								==================
//
// Auteur : F.Becirspahic - IGN / DSTI / SIMV
//
// Date : 09/06/2021
//-----------------------------------------------------------------------------

#include <cstring>
#include <sstream>
#include "XTiffTileImage.h"
#include "XLzwCodec.h"
#include "XZlibCodec.h"
#include "XJpegCodec.h"
#include "XPackBitsCodec.h"
#include "XPredictor.h"

// Gestion de la memoire partagee
byte*     XTiffTileImage::m_gBuffer = NULL;   // Buffer global de lecture
uint32    XTiffTileImage::m_gBufSize = 0;     // Taille du buffer
byte*     XTiffTileImage::m_gTile = NULL;     // Tile globale
uint32    XTiffTileImage::m_gTileSize = 0;    // Taille de la tile globale
XTiffTileImage* XTiffTileImage::m_gLastImage = NULL; // Derniere image utilisee

//-----------------------------------------------------------------------------
// Constructeur
//-----------------------------------------------------------------------------
XTiffTileImage::XTiffTileImage()
{
	m_TileOffsets = m_TileCounts = NULL;
	m_ColorMap = NULL;
  m_Tile = NULL;
	m_JpegTables = NULL;
	Clear();
}

//-----------------------------------------------------------------------------
// Destructeur
//-----------------------------------------------------------------------------
void XTiffTileImage::Clear()
{
	if (m_TileOffsets != NULL)
		delete[] m_TileOffsets;
	if (m_TileOffsets != NULL)
		delete[] m_TileCounts;
	if (m_ColorMap != NULL)
		delete[] m_ColorMap;
	if (m_JpegTables != NULL)
		delete[] m_JpegTables;
	m_TileOffsets = NULL;
	m_TileCounts = NULL;
	m_ColorMap = NULL;
  m_Tile = NULL;
	m_JpegTables = NULL;

	m_nW = m_nH = m_nTileWidth = m_nTileHeight = m_nNbTile = 0;
  m_nPixSize = m_nPhotInt = m_nCompression = m_nPredictor = m_nColorMapSize = 0;
	m_dX0 = m_dY0 = m_dGSD = 0.;
	m_nLastTile = 0xFFFFFFFF;
	m_nJpegTablesSize = 0;
  if (m_gLastImage == this) m_gLastImage = NULL;
}

//-----------------------------------------------------------------------------
// Metadonnees de l'image sous forme de cles / valeurs
//-----------------------------------------------------------------------------
std::string XTiffTileImage::Metadata()
{
  std::ostringstream out;
  out << XBaseImage::Metadata();
  out << "Nb Tiles:" << m_nNbTile << ";TileW:" << m_nTileWidth << ";TileH:" << m_nTileHeight
    << ";Compression:" << XTiffReader::CompressionString(m_nCompression) << ";Predictor:" << m_nPredictor
    << ";PhotInt:" << XTiffReader::PhotIntString(m_nPhotInt) << ";";
  return out.str();
}

//-----------------------------------------------------------------------------
// Fixe les caracteristiques de l'image
//-----------------------------------------------------------------------------
bool XTiffTileImage::SetTiffReader(XBaseTiffReader* reader)
{
	if ((reader->TileWidth() < 1) || (reader->TileHeight() < 1))
		return false;
	Clear();

	if (!reader->GetTileInfo(&m_nNbTile, &m_TileOffsets, &m_TileCounts))
		return false;
	reader->GetJpegTablesInfo(&m_JpegTables, &m_nJpegTablesSize);

	m_nW = reader->Width();
	m_nH = reader->Height();
	m_nTileWidth = reader->TileWidth();
	m_nTileHeight = reader->TileHeight();

	m_nNbBits = reader->NbBits();
	m_nNbSample = reader->NbSample();
	m_nPixSize = PixSize();
	if (m_nPixSize == 0)
		return false;
	m_nPhotInt = reader->PhotInt();
	m_nCompression = reader->Compression();
	m_nPredictor = reader->Predictor();

	m_dX0 = reader->X0();
	m_dY0 = reader->Y0();
	m_dGSD = reader->GSD();

	return AllocBuffer();
}

//-----------------------------------------------------------------------------
// Allocation des buffers de lecture
//-----------------------------------------------------------------------------
bool XTiffTileImage::AllocBuffer()
{
	if (m_nNbTile < 1)
		return false;
	uint32 maxsize = 0;
	for (uint32 i = 0; i < m_nNbTile; i++)
		if (m_TileCounts[i] > maxsize)
			maxsize = m_TileCounts[i];

  if (maxsize > m_gBufSize) {
    if (m_gBuffer != NULL) delete[] m_gBuffer;
    m_gBuffer = new byte[maxsize];
    if (m_gBuffer == NULL)
      return false;
    m_gBufSize = maxsize;
  }
  uint32 tileSize = m_nTileWidth * m_nTileHeight * m_nPixSize;
  if (tileSize > m_gTileSize) {
    if (m_gTile != NULL) delete[] m_gTile;
    m_gTile = new byte[tileSize];
    if (m_gTile == NULL)
      return false;
    m_gTileSize = tileSize;
  }

	return true;
}

//-----------------------------------------------------------------------------
// Chargement d'une Tile
//-----------------------------------------------------------------------------
bool XTiffTileImage::LoadTile(XFile* file, uint32 x, uint32 y)
{
	uint32 nbTileW = (uint32)ceil((double)m_nW / (double)m_nTileWidth);
	uint32 nbTileH = (uint32)ceil((double)m_nH / (double)m_nTileHeight);

	if ((x > nbTileW) || (y > nbTileH))
		return false;
	uint32 numTile = y * nbTileW + x;
	if (numTile > m_nNbTile)
		return false;
  if ((numTile == m_nLastTile)&&(m_gLastImage == this))	// La Tile est deja chargee
		return true;
	file->Seek(m_TileOffsets[numTile]);
  uint32 nBytesRead = file->Read((char*)m_gBuffer, m_TileCounts[numTile]);
	if (nBytesRead != m_TileCounts[numTile])
		return false;
	m_nLastTile = numTile;
  m_gLastImage = this;
	if (!Decompress())
		return false;
	return PostProcess();
}

//-----------------------------------------------------------------------------
// Decompression d'une Tile
//-----------------------------------------------------------------------------
bool XTiffTileImage::Decompress()
{
	if ((m_nCompression == XTiffReader::UNCOMPRESSED1) || (m_nCompression == XTiffReader::UNCOMPRESSED2)) {
    ::memcpy(m_Tile, m_gBuffer, m_TileCounts[m_nLastTile]);
		return true;
	}
	if (m_nCompression == XTiffReader::PACKBITS) {
		XPackBitsCodec codec;
    return codec.Decompress(m_gBuffer, m_TileCounts[m_nLastTile], m_Tile, m_nTileWidth * m_nTileHeight * m_nPixSize);
	}
	if (m_nCompression == XTiffReader::LZW) {
		XLzwCodec codec;
    codec.SetDataIO(m_gBuffer, m_Tile, m_nTileHeight*m_nTileWidth*m_nPixSize);
		codec.Decompress();
    //Predictor();
    XPredictor predictor;
    predictor.Decode(m_Tile, m_nTileWidth, m_nTileHeight, m_nPixSize, m_nNbBits, m_nPredictor);
		return true;
	}
	if (m_nCompression == XTiffReader::DEFLATE) {
		XZlibCodec codec;
    bool flag = codec.Decompress(m_gBuffer, m_TileCounts[m_nLastTile], m_Tile, m_nTileWidth * m_nTileHeight * m_nPixSize);
    //Predictor();
    XPredictor predictor;
    predictor.Decode(m_Tile, m_nTileWidth, m_nTileHeight, m_nPixSize, m_nNbBits, m_nPredictor);
    return flag;
	}
	if ((m_nCompression == XTiffReader::JPEG)||(m_nCompression == XTiffReader::JPEGv2)) {
		XJpegCodec codec;
		if (m_nPhotInt == XTiffReader::YCBCR)
      return codec.DecompressRaw(m_gBuffer, m_TileCounts[m_nLastTile], m_Tile, m_nTileWidth * m_nTileHeight * m_nPixSize,
																	m_JpegTables, m_nJpegTablesSize);
    return codec.Decompress(m_gBuffer, m_TileCounts[m_nLastTile], m_Tile, m_nTileWidth * m_nTileHeight * m_nPixSize,
														m_JpegTables, m_nJpegTablesSize);
	}

	return false;
}

//-----------------------------------------------------------------------------
// Applique le predicteur si necessaire
//-----------------------------------------------------------------------------
void XTiffTileImage::Predictor()
{
	if (m_nPredictor == 1) return;
	if (m_nPredictor == 2) { // PREDICTOR_HORIZONTAL
		uint32 lineW = m_nTileWidth * m_nPixSize;
		for (uint32 i = 0; i < m_nTileHeight; i++)
			for (uint32 j = i * lineW; j < (i + 1) * lineW - m_nPixSize; j++)
				m_Tile[j + m_nPixSize] += m_Tile[j];
		return;
	}
	if (m_nPredictor == 3) { // PREDICTOR_FLOATINGPOINT
		uint32 lineW = m_nTileWidth * m_nPixSize;
		byte* buf = new byte[lineW];
		for (uint32 i = 0; i < m_nTileHeight; i++) {
			for (uint32 j = i * lineW; j < (i + 1) * lineW - 1; j++)
				m_Tile[j + 1] += m_Tile[j];
			std::memcpy(buf, &m_Tile[i * lineW], lineW);
			byte* ptr_0 = buf;
			byte* ptr_1 = &buf[m_nTileWidth];
			byte* ptr_2 = &buf[m_nTileWidth*2];
			byte* ptr_3 = &buf[m_nTileWidth*3];
			for (uint32 j = i * lineW; j < (i + 1) * lineW; j += 4) {
				m_Tile[j] = *ptr_3; ptr_3++;
				m_Tile[j + 1] = *ptr_2; ptr_2++;
				m_Tile[j + 2] = *ptr_1; ptr_1++;
				m_Tile[j + 3] = *ptr_0; ptr_0++;
			}
		}
		delete[] buf;
		return;
	}
}

//-----------------------------------------------------------------------------
// Applique un post-processing sur la derniere strip chargee si necessaire
//-----------------------------------------------------------------------------
bool XTiffTileImage::PostProcess()
{
	// Cas des images 1 bit
	if ((m_nNbBits == 1) && (m_nNbSample == 1)) {
		int byteW = m_nTileWidth / 8L;
		if ((m_nTileWidth % 8L) != 0)
			byteW++;
		byte* tmpTile = new byte[m_nTileWidth * m_nTileHeight * m_nPixSize];
		if (tmpTile == NULL)
			return false;
		bool negatif = false;
		if ((m_nPhotInt == XTiffReader::WHITEISZERO))
			negatif = true;
		for (uint32 num_line = 0; num_line < m_nTileHeight; num_line++) {
			byte* line = &tmpTile[m_nTileWidth * m_nPixSize * num_line];
			byte* bit = &m_Tile[byteW * num_line];
			int n = 0;
			if (negatif) {
				for (int i = 0; i < (int)(byteW); i++)
					for (int j = 7; (j >= 0); j--)
						line[n++] = (1 - ((bit[i] >> j) & 1)) * 255;
			}
			else {
				for (int i = 0; i < (int)(byteW); i++)
					for (int j = 7; (j >= 0); j--)
						line[n++] = ((bit[i] >> j) & 1) * 255;
			}
		}
		byte* oldTile = m_Tile;
		m_Tile = tmpTile;
		delete[] oldTile;
		return true;
	}

	// Cas des images WHITEISZERO
	if ((m_nPhotInt == XTiffReader::WHITEISZERO) && (m_nNbSample == 1)) {
		;
	}

	// Cas des images YCBCR)
	if (m_nPhotInt == XTiffReader::YCBCR) {
		double R, G, B, Y, Cb, Cr;
		byte* ptr_in = m_Tile, * ptr_out = m_Tile;
		for (uint32 i = 0; i < m_nTileHeight; i++) {
			for (uint32 j = 0; j < m_nTileWidth; j++) {
				Y = *ptr_in; ptr_in++;
				Cb = *ptr_in; ptr_in++;
				Cr = *ptr_in; ptr_in++;
				R = XMin( XMax(Y + 1.402 * (Cr - 128.), 0.) , 255.);
				G = XMin(XMax(Y - 0.34414 * (Cb - 128.) - 0.71414 * (Cr - 128.), 0.) , 255.);
				B = XMin(XMax(Y + 1.772 * (Cb - 128.), 0.) , 255.);
				*ptr_out = (byte)R; ptr_out++;
				*ptr_out = (byte)G; ptr_out++;
				*ptr_out = (byte)B; ptr_out++;
			}
		}
		return true;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Recuperation d'une ROI
//-----------------------------------------------------------------------------
bool XTiffTileImage::GetArea(XFile* file, uint32 x, uint32 y, uint32 w, uint32 h, byte* area)
{
	if ((x + w > m_nW) || (y + h > m_nH))
		return false;

	uint32 startX = (uint32)floor((double)x / (double)m_nTileWidth);
	uint32 startY = (uint32)floor((double)y / (double)m_nTileHeight);
	uint32 endX = (uint32)floor((double)(x + w - 1) / (double)m_nTileWidth);
	uint32 endY = (uint32)floor((double)(y + h - 1) / (double)m_nTileHeight);

  m_Tile = m_gTile;
  //m_nLastTile = 0xFFFFFFFF;
	for (uint32 i = startY; i <= endY; i++) {
		for (uint32 j = startX; j <= endX; j++) {
			if (!LoadTile(file, j, i))
				return false;
			if (!CopyTile(j, i, x, y, w, h, area))
				return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Copie les pixels d'une tile dans une ROI
//-----------------------------------------------------------------------------
bool XTiffTileImage::CopyTile(uint32 tX, uint32 tY, uint32 x, uint32 y, uint32 w, uint32 h, byte* area)
{
	// Intersection dans la tile en X
	uint32 startX = tX * m_nTileWidth;
	if (startX >= x)
		startX = 0;
	else
		startX = x - startX;
	uint32 endX = (tX + 1) * m_nTileWidth;
	if (endX <= x + w)
		endX = m_nTileWidth;
	else
		endX = m_nTileWidth - (endX - (x + w));
	// Intersection dans la tile en Y
	uint32 startY = tY * m_nTileHeight;
	if (startY >= y)
		startY = 0;
	else
		startY = y - startY;
	uint32 endY = (tY + 1) * m_nTileHeight;
	if (endY <= y + h)
		endY = m_nTileHeight;
	else
		endY = m_nTileHeight - (endY - (y + h));

	// Debut dans la ROI
	uint32 X0 = 0;
	uint32 Y0 = 0;
	if (tX * m_nTileWidth > x)
		X0 = tX * m_nTileWidth - x;
	if (tY * m_nTileHeight > y)
		Y0 = tY * m_nTileHeight - y;

	// Copie dans la ROI
	uint32 lineSize = (endX - startX) * m_nPixSize;
	uint32 nbline = endY - startY;
	for (uint32 i = 0; i < nbline; i++) {
		byte* source = &m_Tile[((i + startY) * m_nTileWidth + startX) * m_nPixSize];
		byte* dest = &area[(Y0 * w + i * w + X0) * m_nPixSize];
		if (((Y0 * w + i * w + X0) * m_nPixSize + lineSize) > (w * h * m_nPixSize))
			return false;
    ::memcpy(dest, source, lineSize);
	}
	return true;
}

//-----------------------------------------------------------------------------
// Recuperation d'une ligne de pixels
//-----------------------------------------------------------------------------
bool XTiffTileImage::GetLine(XFile* file, uint32 num, byte* area)
{
	return GetArea(file, 0, num, m_nW, 1, area);
}

//-----------------------------------------------------------------------------
// Recuperation d'une ROI avec un facteur de zoom
//-----------------------------------------------------------------------------
bool XTiffTileImage::GetZoomArea(XFile* file, uint32 x, uint32 y, uint32 w, uint32 h, byte* area, uint32 factor)
{
	if (factor == 0) return false;
	if (factor == 1) return GetArea(file, x, y, w, h, area);
	if ((x + w > m_nW) || (y + h > m_nH))
		return false;

	uint32 startX = (uint32)floor((double)x / (double)m_nTileWidth);
	uint32 startY = (uint32)floor((double)y / (double)m_nTileHeight);
	uint32 endX = (uint32)floor((double)(x + w - 1) / (double)m_nTileWidth);
	uint32 endY = (uint32)floor((double)(y + h - 1) / (double)m_nTileHeight);

  m_Tile = m_gTile;
  //m_nLastTile = 0xFFFFFFFF;
  for (uint32 i = startY; i <= endY; i++) {
		for (uint32 j = startX; j <= endX; j++) {
			if (!LoadTile(file, j, i))
				return false;
			if (!CopyZoomTile(j, i, x, y, w, h, area, factor))
				return false;
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Copie les pixels d'une tile dans une ROI avec un facteur de zoom
//-----------------------------------------------------------------------------
bool XTiffTileImage::CopyZoomTile(uint32 tX, uint32 tY, uint32 x, uint32 y, uint32 w, uint32 h, 
																	byte* area, uint32 factor)
{
	uint32 wout = w / factor;
	uint32 hout = h / factor;

	// Copie dans la ROI
	uint32 ycur = y;
	for (uint32 numli = 0; numli < hout; numli++) {
		if ((ycur < tY * m_nTileHeight) || (ycur > (tY + 1) * m_nTileHeight)) {
			ycur += factor;
			continue;
		}
		uint32 numTileLine = (ycur - tY * m_nTileHeight);
    if (numTileLine >= m_nTileHeight)
      break;
		uint32 xcur = x;
		for (uint32 numco = 0; numco < wout; numco++) {
			if ((xcur < tX * m_nTileWidth) || (xcur > (tX + 1) * m_nTileWidth)) {
				xcur += factor;
				continue;
			}
			uint32 numTileCol = (xcur - tX * m_nTileWidth);
      if (numTileCol >= m_nTileWidth)
        break;
			::memcpy(&area[(numli * wout + numco) * m_nPixSize], &m_Tile[(numTileLine* m_nTileWidth + numTileCol)* m_nPixSize], m_nPixSize);
			xcur += factor;
		}
		ycur += factor;
	}

	return true;
}

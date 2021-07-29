//-----------------------------------------------------------------------------
//								XTiffStripImage.cpp
//								===================
//
// Auteur : F.Becirspahic - IGN / DSTI / SIMV
//
// Date : 15/06/2021
//-----------------------------------------------------------------------------

#include <cstring>
#include <sstream>
#include "XTiffStripImage.h"
#include "XLzwCodec.h"
#include "XZlibCodec.h"
#include "XJpegCodec.h"
#include "XPackBitsCodec.h"

//-----------------------------------------------------------------------------
// Constructeur
//-----------------------------------------------------------------------------
XTiffStripImage::XTiffStripImage()
{
	m_StripOffsets = m_StripCounts = NULL;
	m_ColorMap = NULL;
	m_Buffer = m_Strip = NULL;
	m_JpegTables = NULL;
	Clear();
}

//-----------------------------------------------------------------------------
// Destructeur
//-----------------------------------------------------------------------------
void XTiffStripImage::Clear()
{
	if (m_StripOffsets != NULL)
		delete[] m_StripOffsets;
	if (m_StripOffsets != NULL)
		delete[] m_StripCounts;
	if (m_ColorMap != NULL)
		delete[] m_ColorMap;
	if (m_Buffer != NULL)
		delete[] m_Buffer;
	if (m_Strip != NULL)
		delete[] m_Strip;
	if (m_JpegTables != NULL)
		delete[] m_JpegTables;
	m_StripOffsets = NULL;
	m_StripCounts = NULL;
	m_ColorMap = NULL;
	m_Buffer = m_Strip = NULL;
	m_JpegTables = NULL;

	m_nW = m_nH = m_nRowsPerStrip = m_nNbStrip = 0;
  m_nPixSize = m_nPhotInt = m_nCompression = m_nPredictor = m_nColorMapSize = 0;
	m_dX0 = m_dY0 = m_dGSD = 0.;
	m_nLastStrip = 0xFFFFFFFF;
	m_nJpegTablesSize = 0;
}

//-----------------------------------------------------------------------------
// Metadonnees de l'image sous forme de cles / valeurs
//-----------------------------------------------------------------------------
std::string XTiffStripImage::Metadata()
{
  std::ostringstream out;
  out << XBaseImage::Metadata();
  out << "Nb Strip:" << m_nNbStrip << ";RowsPerStrip:" << m_nRowsPerStrip
    << ";Compression:" << XTiffReader::CompressionString(m_nCompression)
    << ";PhotInt:" << XTiffReader::PhotIntString(m_nPhotInt) << ";";
  return out.str();
}

//-----------------------------------------------------------------------------
// Fixe les caracteristiques de l'image
//-----------------------------------------------------------------------------
bool XTiffStripImage::SetTiffReader(XBaseTiffReader* reader)
{
	if (reader->RowsPerStrip() < 1)
		return false;
	Clear();

	if (!reader->GetStripInfo(&m_nNbStrip, &m_StripOffsets, &m_StripCounts))
		return false;
	reader->GetJpegTablesInfo(&m_JpegTables, &m_nJpegTablesSize);
  reader->GetColorMap(&m_ColorMap, &m_nColorMapSize);

	m_nW = reader->Width();
	m_nH = reader->Height();
	m_nRowsPerStrip = reader->RowsPerStrip();

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
bool XTiffStripImage::AllocBuffer()
{
	if (m_nNbStrip < 1)
		return false;
	uint32 maxsize = 0;
	for (uint32 i = 0; i < m_nNbStrip; i++)
		if (m_StripCounts[i] > maxsize)
			maxsize = m_StripCounts[i];
	m_Buffer = new byte[maxsize];
	if (m_Buffer == NULL)
		return false;
	m_Strip = new byte[m_nRowsPerStrip * m_nW * m_nPixSize];
	if (m_Strip == NULL) {
		delete[] m_Buffer;
		m_Buffer = NULL;
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
// Chargement d'une Strip
//-----------------------------------------------------------------------------
bool XTiffStripImage::LoadStrip(XFile* file, uint32 num)
{
	if (num >= m_nNbStrip)
		return false;
	if (num == m_nLastStrip)	// La Strip est deja chargee
		return true;
	file->Seek(m_StripOffsets[num]);
	uint32 nBytesRead = file->Read((char*)m_Buffer, m_StripCounts[num]);
	if (nBytesRead != m_StripCounts[num])
		return false;
	m_nLastStrip = num;
	if (!Decompress())
		return false;
	return PostProcess();
}

//-----------------------------------------------------------------------------
// Decompression d'une Tile
//-----------------------------------------------------------------------------
bool XTiffStripImage::Decompress()
{
	if ((m_nCompression == XTiffReader::UNCOMPRESSED1) || (m_nCompression == XTiffReader::UNCOMPRESSED2)) {
		std::memcpy(m_Strip, m_Buffer, m_StripCounts[m_nLastStrip]);
		return true;
	}
	if (m_nCompression == XTiffReader::PACKBITS) {
		XPackBitsCodec codec;
		return codec.Decompress(m_Buffer, m_StripCounts[m_nLastStrip], m_Strip, m_nW * m_nRowsPerStrip * m_nPixSize);
	}
	if (m_nCompression == XTiffReader::LZW) {
		XLzwCodec codec;
		codec.SetDataIO(m_Buffer, m_Strip, m_nRowsPerStrip * m_nW * m_nPixSize);
		codec.Decompress();
		Predictor();
		return true;
	}
	if (m_nCompression == XTiffReader::DEFLATE) {
		XZlibCodec codec;
		bool flag = codec.Decompress(m_Buffer, m_StripCounts[m_nLastStrip], m_Strip, m_nW * m_nRowsPerStrip * m_nPixSize);
		Predictor();
		return flag;
	}
	if ((m_nCompression == XTiffReader::JPEG) || (m_nCompression == XTiffReader::JPEGv2)) {
		XJpegCodec codec;
		if (m_nPhotInt == XTiffReader::YCBCR)
			return codec.DecompressRaw(m_Buffer, m_StripCounts[m_nLastStrip], m_Strip, m_nW * m_nRowsPerStrip * m_nPixSize,
				m_JpegTables, m_nJpegTablesSize);
		return codec.Decompress(m_Buffer, m_StripCounts[m_nLastStrip], m_Strip, m_nW * m_nRowsPerStrip * m_nPixSize,
														m_JpegTables, m_nJpegTablesSize);
	}

	return false;
}

//-----------------------------------------------------------------------------
// Applique le predicteur sur la derniere strip chargee si necessaire
//-----------------------------------------------------------------------------
void XTiffStripImage::Predictor()
{
	if (m_nPredictor == 1) return;
	if (m_nPredictor == 2) {
		uint32 lineW = m_nW * m_nPixSize;
		for (uint32 i = 0; i < m_nRowsPerStrip; i++)
			for (uint32 j = i * lineW; j < (i + 1) * lineW - m_nPixSize; j++)
				m_Strip[j + m_nPixSize] += m_Strip[j];
		return;
	}
}

//-----------------------------------------------------------------------------
// Applique un post-processing sur la derniere strip chargee si necessaire
//-----------------------------------------------------------------------------
bool XTiffStripImage::PostProcess()
{
	// Cas des images 1 bit
	if ((m_nNbBits == 1) && (m_nNbSample == 1)) {
		int byteW = m_nW / 8L;
		if ((m_nW % 8L) != 0)
			byteW++;
		byte* tmpStrip = new byte[m_nRowsPerStrip * m_nW * m_nPixSize];
		if (tmpStrip == NULL)
			return false;
		bool negatif = false;
		if ((m_nPhotInt == XTiffReader::WHITEISZERO))
			negatif = true;
		for (uint32 num_line = 0; num_line < m_nRowsPerStrip; num_line++) {
			byte* line = &tmpStrip[m_nW * m_nPixSize * num_line];
			byte* bit = &m_Strip[byteW * num_line];
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
		byte* oldStrip = m_Strip;
		m_Strip = tmpStrip;
		delete[] oldStrip;
		return true;
	}

	// Case des images WHITEISZERO
	if ((m_nPhotInt == XTiffReader::WHITEISZERO) && (m_nNbSample == 1)) {
		;
	}

	// Cas des images YCBCR)
	if ((m_nPhotInt == XTiffReader::YCBCR) && (m_nNbSample == 3)) {
		double R, G, B, Y, Cb, Cr;
		byte* ptr_in = m_Strip, * ptr_out = m_Strip;
		for (uint32 i = 0; i < m_nRowsPerStrip; i++) {
			for (uint32 j = 0; j < m_nW; j++) {
				Y = *ptr_in; ptr_in++;
				Cb = *ptr_in; ptr_in++;
				Cr = *ptr_in; ptr_in++;
				R = XMin(XMax(Y + 1.402 * (Cr - 128.), 0.), 255.);
				G = XMin(XMax(Y - 0.34414 * (Cb - 128.) - 0.71414 * (Cr - 128.), 0.), 255.);
				B = XMin(XMax(Y + 1.772 * (Cb - 128.), 0.), 255.);
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
bool XTiffStripImage::GetArea(XFile* file, uint32 x, uint32 y, uint32 w, uint32 h, byte* area)
{
	if ((x + w > m_nW) || (y + h > m_nH))
		return false;

	uint32 startRow = (uint32) floor((double)y / (double)m_nRowsPerStrip);
	uint32 endRow = (uint32)floor((double)(y + h - 1) / (double)m_nRowsPerStrip);

	for (uint32 i = startRow; i <= endRow; i++) {
		if (!LoadStrip(file, i))
			return false;
		if (!CopyStrip(i, x, y, w, h, area))
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Copie les pixels d'une tile dans une ROI
//-----------------------------------------------------------------------------
bool XTiffStripImage::CopyStrip(uint32 numStrip, uint32 x, uint32 y, uint32 w, uint32 h, byte* area)
{
	// Interesection dans la tile en Y
	uint32 startY = numStrip * m_nRowsPerStrip;
	if (startY >= y)
		startY = 0;
	else
		startY = y - startY;
	uint32 endY = (numStrip + 1) * m_nRowsPerStrip;
	if (endY <= y + h)
		endY = m_nRowsPerStrip;
	else
		endY = m_nRowsPerStrip - (endY - (y + h));

	// Debut dans la ROI
	uint32 Y0 = 0;
	if (numStrip * m_nRowsPerStrip > y)
		Y0 = numStrip * m_nRowsPerStrip - y;

	// Copie dans la ROI
	uint32 lineSize = w * m_nPixSize;
	uint32 nbline = endY - startY;
	for (uint32 i = 0; i < nbline; i++) {
		byte* source = &m_Strip[((i + startY) * m_nW + x) * m_nPixSize];
		byte* dest = &area[(Y0 * w + i * w) * m_nPixSize];
		::memcpy(dest, source, lineSize);
	}

	return true;
}

//-----------------------------------------------------------------------------
// Recuperation d'une ligne de pixels
//-----------------------------------------------------------------------------
bool XTiffStripImage::GetLine(XFile* file, uint32 num, byte* area)
{
	if (num >= m_nH)
		return false;
	uint32 numStrip = num / m_nRowsPerStrip;
	if (!LoadStrip(file, numStrip))
		return false;
	uint32 numLine = num % m_nRowsPerStrip;
	::memcpy(area, &m_Strip[numLine * m_nW * m_nPixSize], m_nW * m_nPixSize);
	return true;
}

//-----------------------------------------------------------------------------
// Recuperation d'une ROI avec un facteur de zoom
//-----------------------------------------------------------------------------
bool XTiffStripImage::GetZoomArea(XFile* file, uint32 x, uint32 y, uint32 w, uint32 h, byte* area, uint32 factor)
{
	if (factor == 0) return false;
	if (factor == 1) return GetArea(file, x, y, w, h, area);
	if ((x + w > m_nW) || (y + h > m_nH))
		return false;

	byte* line = AllocArea(m_nW, 1);
	if (line == NULL)
		return false;

	uint32 xpos = x * m_nPixSize;
	uint32 wout = w / factor;
	uint32 hout = h / factor;
	uint32 maxW = XMin(xpos + w * m_nPixSize, m_nPixSize * (m_nW - 1));
	uint32 maxH = XMin(y + h, m_nH - 1);

	uint32 i = y;
	for (uint32 numli = 0; numli < hout; numli++) {
		GetLine(file, i, line);
		uint32 j = xpos;
		byte* buf = &area[numli * (m_nPixSize * wout)];
		for (uint32 numpx = 0; numpx < wout; numpx++) {
			::memcpy(buf, &line[j], m_nPixSize);
			buf += m_nPixSize;
			j += (factor * m_nPixSize);
			if (j > maxW)
				break;
		};
		i += factor;
		if (i > maxH)
			break;
	}
	delete[] line;
	return true;
}

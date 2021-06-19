//-----------------------------------------------------------------------------
//								XTiffReader.h
//								=============
//
// Auteur : F.Becirspahic - IGN / DSTI / SIMV
//
// Date : 19/05/2021
//-----------------------------------------------------------------------------

#ifndef XTIFFREADER_H
#define XTIFFREADER_H

#include <istream>
#include <vector>
#include "../XTool/XEndian.h"

class XTiffReader {
public:

	enum eTagID { BITSPERSAMPLE = 258, COLORMAP = 320, COMPRESSION = 259, IMAGEHEIGHT = 257, IMAGEWIDTH	= 256,
								NEWSUBFILETYPE = 254, PHOTOMETRICINTERPRETATION	= 262, PLANARCONFIGURATION = 284,
								RESOLUTIONUNIT = 296, ROWSPERSTRIP = 278, SAMPLESPERPIXEL = 277, STRIPBYTECOUNTS = 279,
								STRIPOFFSETS = 273, TILEBYTECOUNTS = 325, TILELENGTH = 323, TILEOFFSETS = 324, TILEWIDTH = 322,
								XRESOLUTION = 282, YRESOLUTION = 283, PREDICTOR = 317, JPEGTABLES = 347, YCBCRSUBSAMPLING = 530,
								REFERENCEBLACKWHITE = 532,
								ModelPixelScaleTag = 33550, ModelTiepointTag = 33922};

	enum eDataType { BYTE = 1, ASCII = 2, SHORT = 3, LONG = 4, RATIONAL = 5, SBYTE = 6, UNDEFINED = 7, SSHORT = 8,
									 SLONG = 9, SRATIONAL = 10, FLOAT = 11, DOUBLE = 12};

	enum eCompression { UNCOMPRESSED1 = 1, CCITT1D = 2, CCITTGROUP3 = 3, CCITTGROUP4 = 4, LZW = 5, JPEG = 6, JPEGv2 = 7,
											DEFLATE = 8, UNCOMPRESSED2	= 32771, PACKBITS = 32773};

	enum ePhotInt { WHITEISZERO = 0, BLACKISZERO = 1, RGBPHOT = 2, RGBPALETTE = 3, TRANSPARENCYMASK = 4,
									CMYKPHOT = 5, YCBCR = 6, CIELAB = 8 };

protected:
	/* Structure TiffTag	: tag TIFF de 12 bytes	*/
	typedef struct _TiffTag {
		uint16		TagId;
		uint16		DataType;
		uint32		DataCount;
		uint32		DataOffset;
	} TiffTag;

	/* Structure TiffIFD : Image File Directory	*/
	typedef struct _TiffIFD {
		std::vector<TiffTag>	TagList;       /* Tableau de Tags   		*/
		uint32								NextIFDOffset; /* Offset vers le prochain IFD	*/
	} TiffIFD;

	bool ReadHeader(std::istream* in);
	bool ReadIFD(std::istream* in, uint32 offset, TiffIFD* ifd);
	void Clear();

	bool FindTag(eTagID id, TiffTag* T);
	uint32 TypeSize(eDataType type);
	uint32 DataSize(TiffTag* T);
	uint32 ReadDataInTag(TiffTag* T);
	uint32 ReadIdTag(eTagID id, uint32 defaut = 0);
	bool ReadDataArray(std::istream* in, eTagID id, void* V, int size);
	uint32* ReadUintArray(std::istream* in, eTagID id, uint32* nb_elt);

	static std::string CompressionString(uint16 compression);
	static std::string PhotIntString(uint16 photint);

	XEndian								m_Endian;
	bool									m_bByteOrder;		// Byte order du fichier
	uint32								m_nIFDOffset;		// Offset du premier IFD
	std::vector<TiffIFD>	m_IFD;					// Liste des IFD
	uint32								m_nActiveIFD;		// IFD actif

	uint32	m_nWidth;
	uint32	m_nHeight;
	uint32	m_nRowsPerStrip;
	uint32	m_nTileWidth;
	uint32	m_nTileHeight;
	uint32	m_nNbStripOffsets;
	uint32	m_nNbStripCounts;
	uint32	m_nNbTileOffsets;
	uint32	m_nNbTileCounts;
	uint16	m_nNbBits;
	uint16	m_nNbSample;
	uint16	m_nCompression;
	uint16	m_nPredictor;
	uint16	m_nPhotInt;
	uint16	m_nPlanarConfig;
	uint32* m_StripOffsets;
	uint32* m_StripCounts;
	uint32* m_TileOffsets;
	uint32* m_TileCounts;
	uint16* m_ColorMap;
	byte*		m_JpegTables;
	uint32	m_nJpegTablesSize;
	uint16	m_YCbCrSub[2];
	uint32	m_ReferenceBlack[12];
	double	m_dX0;
	double	m_dY0;
	double	m_dGSD;

public:
	XTiffReader() {
		m_nActiveIFD = 0; m_StripOffsets = m_StripCounts = m_TileOffsets = m_TileCounts = NULL; 
		m_ColorMap = NULL; m_JpegTables = NULL; m_nJpegTablesSize = 0;
	}
	virtual ~XTiffReader() { Clear(); }

	bool Read(std::istream* in);
	uint32 NbIFD() { return m_IFD.size(); }
	bool SetActiveIFD(uint32 i) { if (i >= m_IFD.size()) return false; m_nActiveIFD = i; return true;}
	bool AnalyzeIFD(std::istream* in);

	inline uint32 Width() { return m_nWidth; }
	inline uint32 Height() { return m_nHeight; }
	inline uint32 RowsPerStrip() { return m_nRowsPerStrip; }
	inline uint32 TileWidth() { return m_nTileWidth; }
	inline uint32 TileHeight() { return m_nTileHeight; }
	bool GetStripInfo(uint32* nbStrip, uint32** offset, uint32** count);
	bool GetTileInfo(uint32* nbTile, uint32** offset, uint32** count);
	inline uint16	NbBits() { return m_nNbBits; }
	inline uint16	NbSample() { return m_nNbSample; }
	inline uint16	Compression() { return m_nCompression; }
	inline uint16	Predictor() { return m_nPredictor; }
	inline uint16	PhotInt() { return m_nPhotInt; }
	inline uint16	PlanarConfig() { return m_nPlanarConfig; }
	inline double X0() { return m_dX0; }
	inline double Y0() { return m_dY0; }
	inline double GSD() { return m_dGSD; }
	bool GetJpegTablesInfo(byte** tables, uint32* size);

	void PrintInfo(std::ostream* out);
	void PrintIFDTag(std::ostream* out);
};


#endif // XTIFFREADER_H
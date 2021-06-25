//-----------------------------------------------------------------------------
//								XTiffTileImage.h
//								================
//
// Auteur : F.Becirspahic - IGN / DSTI / SIMV
//
// Date : 09/06/2021
//-----------------------------------------------------------------------------

#ifndef XTIFFTILEIMAGE_H
#define XTIFFTILEIMAGE_H

#include "XTiffReader.h"
#include "XBaseImage.h"

class XTiffTileImage : public XBaseImage {
public:
	XTiffTileImage();
	virtual ~XTiffTileImage() { Clear(); }

	bool SetTiffReader(XTiffReader* reader);

	virtual bool GetArea(XFile* file, uint32 x, uint32 y, uint32 w, uint32 h, byte* area);
	virtual bool GetLine(XFile* file, uint32 num, byte* area);
	virtual bool GetZoomArea(XFile* file, uint32 x, uint32 y, uint32 w, uint32 h, byte* area, uint32 factor);

protected:
	void		Clear();
	bool		AllocBuffer();
	bool		LoadTile(XFile* file, uint32 x, uint32 y);
	bool		Decompress();
	void		Predictor();
	bool		PostProcess();
	bool		CopyTile(uint32 tX, uint32 tY, uint32 x, uint32 y, uint32 w, uint32 h, byte* area);
	bool		CopyZoomTile(uint32 tX, uint32 tY, uint32 x, uint32 y, uint32 w, uint32 h, byte* area, uint32 factor);

	uint32		m_nTileWidth;
	uint32		m_nTileHeight;
	uint32		m_nNbTile;
	uint16		m_nPixSize;
	uint16		m_nPhotInt;
	uint16		m_nCompression;
	uint16		m_nPredictor;
	uint32*		m_TileOffsets;
	uint32*		m_TileCounts;
	uint16*		m_ColorMap;
	byte*			m_JpegTables;
	uint32		m_nJpegTablesSize;

	byte*			m_Tile;		// Derniere tile chargee
	uint32		m_nLastTile;	// Numero de la derniere tile chargee

  // Gestion de la memoire partagee
  static byte*     m_gBuffer;   // Buffer global de lecture
  static uint32    m_gBufSize;  // Taille du buffer
  static byte*     m_gTile;     // Tile globale
  static uint32    m_gTileSize; // Taille de la tile globale
};

#endif //XTIFFTILEIMAGE_H

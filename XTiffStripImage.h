//-----------------------------------------------------------------------------
//								XTiffStripImage.h
//								=================
//
// Auteur : F.Becirspahic - IGN / DSTI / SIMV
//
// Date : 15/06/2021
//-----------------------------------------------------------------------------

#ifndef XTIFFSTRIPIMAGE_H
#define XTIFFSTRIPIMAGE_H

#include "XTiffReader.h"
#include "XBaseImage.h"

class XTiffStripImage : public XBaseImage {
public:
	XTiffStripImage();
	virtual ~XTiffStripImage() { Clear(); }

	bool SetTiffReader(XTiffReader* reader);

	virtual bool GetArea(XFile* file, uint32 x, uint32 y, uint32 w, uint32 h, byte* area);
	virtual bool GetLine(XFile* file, uint32 num, byte* area);
	virtual bool GetZoomArea(XFile* file, uint32 x, uint32 y, uint32 w, uint32 h, byte* area, uint32 factor);

protected:
	void		Clear();
	bool		AllocBuffer();
	bool		LoadStrip(XFile* file, uint32 num);
	bool		Decompress();
	void		Predictor();
	bool		PostProcess();
	bool		CopyStrip(uint32 numStrip, uint32 x, uint32 y, uint32 w, uint32 h, byte* area);

	uint32		m_nRowsPerStrip;
	uint32		m_nNbStrip;
	uint16		m_nPixSize;
	uint16		m_nPhotInt;
	uint16		m_nCompression;
	uint16		m_nPredictor;
	uint32*		m_StripOffsets;
	uint32*		m_StripCounts;
	uint16*		m_ColorMap;
	byte*			m_JpegTables;
	uint32		m_nJpegTablesSize;

	byte*			m_Buffer;			// Buffer de lecture
	byte*			m_Strip;			// Derniere strip chargee
	uint32		m_nLastStrip;	// Numero de la derniere strip chargee
};

#endif //XTIFFSTRIPIMAGE_H

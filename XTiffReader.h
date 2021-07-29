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

#include "XBaseTiffReader.h"

class XTiffReader : public XBaseTiffReader {
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

	bool FindTag(eTagID id, TiffTag* T);
	uint32 DataSize(TiffTag* T);
	uint32 ReadDataInTag(TiffTag* T);
	uint32 ReadIdTag(eTagID id, uint32 defaut = 0);
	bool ReadDataArray(std::istream* in, eTagID id, void* V, int size);
	uint32* ReadUintArray(std::istream* in, eTagID id, uint32* nb_elt);

	uint32								m_nIFDOffset;		// Offset du premier IFD
	std::vector<TiffIFD>	m_IFD;					// Liste des IFD

public:
	XTiffReader() : XBaseTiffReader() { ; }
	virtual ~XTiffReader() { Clear(); }

	virtual bool Read(std::istream* in);
	virtual uint32 NbIFD() { return m_IFD.size(); }
	virtual bool SetActiveIFD(uint32 i) { if (i >= m_IFD.size()) return false; m_nActiveIFD = i; return true;}
	virtual bool AnalyzeIFD(std::istream* in);

	virtual void PrintIFDTag(std::ostream* out);
};


#endif // XTIFFREADER_H

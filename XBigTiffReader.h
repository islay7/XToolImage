//-----------------------------------------------------------------------------
//								XBigTiffReader.h
//								================
//
// Auteur : F.Becirspahic - IGN / DSTI / SIMV
//
// Date : 21/07/2021
//-----------------------------------------------------------------------------

#ifndef XBIGTIFFREADER_H
#define XBIGTIFFREADER_H

#include "XBaseTiffReader.h"

class XBigTiffReader : public XBaseTiffReader {
protected:
	/* Structure BigTiffTag	: tag BigTIFF de 20 bytes	*/
	typedef struct _BigTiffTag {
		uint16		TagId;
		uint16		DataType;
		uint64		DataCount;
		uint64		DataOffset;
	} BigTiffTag;

	/* Structure TiffIFD : Image File Directory	*/
	typedef struct _BigTiffIFD {
		std::vector<BigTiffTag>	TagList;       /* Tableau de Tags   		*/
		uint64									NextIFDOffset; /* Offset vers le prochain IFD	*/
	} BigTiffIFD;

	bool ReadHeader(std::istream* in);
	bool ReadIFD(std::istream* in, uint64 offset, BigTiffIFD* ifd);

	bool FindTag(eTagID id, BigTiffTag* T);
	uint32 DataSize(BigTiffTag* T);
	uint32 ReadDataInTag(BigTiffTag* T);
	uint32 ReadIdTag(eTagID id, uint32 defaut = 0);
	bool ReadDataArray(std::istream* in, eTagID id, void* V, int size);
	uint32* ReadUintArray(std::istream* in, eTagID id, uint32* nb_elt);
	uint64* ReadUint64Array(std::istream* in, eTagID id, uint32* nb_elt);

	uint64									m_nIFDOffset;		// Offset du premier IFD
	std::vector<BigTiffIFD>	m_IFD;					// Liste des IFD

public:
	XBigTiffReader() : XBaseTiffReader() {;}
	virtual ~XBigTiffReader() { Clear(); }

	virtual bool Read(std::istream* in);
	virtual uint32 NbIFD() { return m_IFD.size(); }
	virtual bool SetActiveIFD(uint32 i) { if (i >= m_IFD.size()) return false; m_nActiveIFD = i; return true;}
	virtual bool AnalyzeIFD(std::istream* in);
	
	void PrintIFDTag(std::ostream* out);

};


#endif // XTIFFREADER_H

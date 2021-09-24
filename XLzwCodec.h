//-----------------------------------------------------------------------------
//								XLzwCodec.h
//								===========
//
// Auteur : F.Becirspahic - MODSP
//
// 23/08/2009
//
// Classe XLzwCodec : compresseur / Decompresseur LZW
//-----------------------------------------------------------------------------

#ifndef XLZWCODEC_H
#define XLZWCODEC_H

#include "../XTool/XBase.h"

class XLzwString {
  enum { eAlloc = 1};
protected:
  uint32	m_nLength;
  uint32	m_nAlloc;
  byte*		m_String;
public:
  //XLzwString() : m_nLength(0) { m_nAlloc = eAlloc; m_String = new byte[m_nAlloc];}
  XLzwString() : m_nLength(0) { m_nAlloc = 0; m_String = NULL;}
  XLzwString(byte c);
  XLzwString(int n, byte* str);
  ~XLzwString();

  inline uint32 length(void) const {return m_nLength;}
  inline byte* string(void) const {return m_String;}

  XLzwString& operator=(const XLzwString&);
  void merge(const XLzwString& A, const XLzwString& B);
  void set(int n, byte* str);
};

class XLzwCodec {
protected:
  XLzwString*	m_table;
  byte*	m_lzw;		// Donnees compressees
  byte*	m_out;		// Donnees decompressees
  byte*	m_outpos;	// Position courante dans le buffer de sortie
  int		m_nbbit;	// Nombre de bits a lire
  int		m_bitpos;	// Position dans le buffer de lecture (en bit)
  int		m_next;		// Position du prochain code
  int		m_mask[14];		// Masque (pour les decalages vers la droite)
  uint32  m_size;     // Taille maximum du buffer de sortie

  void InitializeTable();
  void AddStringToTable(int, byte*);
  void AddStringToTable(uint32 oldcode, uint32 code);
  void WriteString(int code);
  void WriteString(XLzwString* S);
  int GetNextCode();

public:
  XLzwCodec();
  virtual ~XLzwCodec();

  void SetDataIO(byte* lzw, byte* out, uint32 size);
  void Decompress();
};

#endif // XLZWCODEC_H

//-----------------------------------------------------------------------------
//								XBaseImage.cpp
//								==============
//
// Auteur : F.Becirspahic - IGN / DSTI / SIMV
//
// Date : 18/06/2021
//-----------------------------------------------------------------------------

#include "XBaseImage.h"

//-----------------------------------------------------------------------------
// Constructeur
//-----------------------------------------------------------------------------
XBaseImage::XBaseImage()
{
	m_nW = m_nH = 0;
	m_dX0 = m_dY0 = m_dGSD = 0.;
	m_nNbBits = m_nNbSample = 0;
}

//-----------------------------------------------------------------------------
// Destructeur
//-----------------------------------------------------------------------------
XBaseImage::~XBaseImage()
{

}

//-----------------------------------------------------------------------------
// Renvoie la taille d'un pixel en octet
//-----------------------------------------------------------------------------
uint32 XBaseImage::PixSize()
{
	if ((m_nNbBits % 8) == 0) // Images 8 bits, 16 bits, ...
		return m_nNbSample * (m_nNbBits / 8);
	if (m_nNbBits == 1)	// Les images 1 bit sont renvoyees en 8 bits
		return m_nNbSample;
	return 0;		// On ne gere pas en standard les images 4 bits, 12 bits ...
}

//-----------------------------------------------------------------------------
// Allocation d'une zone de pixel
//-----------------------------------------------------------------------------
byte* XBaseImage::AllocArea(uint32 w, uint32 h)
{
	return new byte[w * h * PixSize()];
}

//-----------------------------------------------------------------------------
// Conversion CMYK -> RGB
//-----------------------------------------------------------------------------
bool XBaseImage::CMYK2RGB(byte* buffer, uint32 w, uint32 h)
{
	double R, G, B, C, M, Y, K;
	byte* ptr_in = buffer, * ptr_out = buffer;
	for (uint32 i = 0; i < h; i++) {
		for (uint32 j = 0; j < w; j++) {
			C = *ptr_in / 255.; ptr_in++;
			M = *ptr_in / 255.; ptr_in++;
			Y = *ptr_in / 255.; ptr_in++;
			K = *ptr_in / 255.; ptr_in++;
			C = (C * (1. - K) + K);
			M = (M * (1. - K) + K);
			Y = (Y * (1. - K) + K);
			*ptr_out = (byte)((1 - C)*255); ptr_out++;
			*ptr_out = (byte)((1 - M)*255); ptr_out++;
			*ptr_out = (byte)((1 - Y)*255); ptr_out++;
		}
	}
	return true;
}
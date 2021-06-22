//-----------------------------------------------------------------------------
//								XBaseImage.cpp
//								==============
//
// Auteur : F.Becirspahic - IGN / DSTI / SIMV
//
// Date : 18/06/2021
//-----------------------------------------------------------------------------

#include <cstring>
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
  double C, M, Y, K;
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

//-----------------------------------------------------------------------------
// Zoom sur un buffer de pixels
//-----------------------------------------------------------------------------
bool XBaseImage::ZoomArea(byte* in, byte* out, uint32 win, uint32 hin, uint32 wout, uint32 hout, uint32 nbbyte)
{
	uint32* lut = new uint32[wout];
	for (uint32 i = 0; i < wout; i++)
		lut[i] = nbbyte * (i * win / wout);

	for (uint32 i = 0; i < hout; i++) {
		byte* line = &out[i * (nbbyte * wout)];
		byte* src = &in[nbbyte * win * (i * hin / hout)];
		for (uint32 k = 0; k < wout; k++) {
      ::memcpy(line, &src[lut[k]], nbbyte);
			line += nbbyte;
		}
	}

	delete[] lut;
	return true;
}

//-----------------------------------------------------------------------------
// Inversion de triplet RGB en triplet BGR : Windows travaille en BGR
//-----------------------------------------------------------------------------
void XBaseImage::SwitchRGB2BGR(byte* buf, uint32 buf_size)
{
	byte r;
	for (uint32 i = 0; i < buf_size; i += 3) {
		r = buf[i + 2];
		buf[i + 2] = buf[i];
		buf[i] = r;
	}
}

//-----------------------------------------------------------------------------
// Rotation d'une zone de pixels : 0->0° , 1->90°, 2->180°, 3->270°
//-----------------------------------------------------------------------------
bool XBaseImage::RotateArea(byte* in, byte* out, uint32 win, uint32 hin, uint32 nbbyte, uint32 rot)
{
	if (rot == 0)		// Pas de rotation
		return true;
	if (rot > 3)
		return false;

	byte* forw;
	byte* back;
	uint32 lineW = win * nbbyte;

	if (rot == 2) {	// Rotation a 180 degres
		for (uint32 i = 0; i < hin; i++) {
			forw = &in[i * lineW];
			back = &out[(hin - 1 - i) * lineW + (win - 1) * nbbyte];
			for (uint32 j = 0; j < win; j++) {
        ::memcpy(back, forw, nbbyte);
				forw += nbbyte;
				back -= nbbyte;
			}
		}
		return true;
	}

	if (rot == 1) { // Rotation a 90 degres
		for (uint32 i = 0; i < win; i++) {
			forw = &in[i * nbbyte];
			back = &out[(win - 1 - i) * hin * nbbyte];
			for (uint32 j = 0; j < hin; j++) {
        ::memcpy(back, forw, nbbyte);
				back += nbbyte;
				forw += lineW;
			}
		}
		return true;
	}

	if (rot == 3) { // Rotation a 270 degres
		for (uint32 i = 0; i < win; i++) {
			forw = &in[i * nbbyte];
			back = &out[i * hin * nbbyte + nbbyte * (hin - 1)];
			for (uint32 j = 0; j < hin; j++) {
        ::memcpy(back, forw, nbbyte);
				forw += lineW;
				back -= nbbyte;
			}
		}
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
//								XBaseImage.cpp
//								==============
//
// Auteur : F.Becirspahic - IGN / DSTI / SIMV
//
// Date : 18/06/2021
//-----------------------------------------------------------------------------

#include <cstring>
#include <sstream>
#include "XBaseImage.h"

//-----------------------------------------------------------------------------
// Constructeur
//-----------------------------------------------------------------------------
XBaseImage::XBaseImage()
{
	m_nW = m_nH = 0;
	m_dX0 = m_dY0 = m_dGSD = 0.;
	m_nNbBits = m_nNbSample = 0;
  m_ColorMap = NULL;
  m_nColorMapSize = 0;
}

//-----------------------------------------------------------------------------
// Destructeur
//-----------------------------------------------------------------------------
XBaseImage::~XBaseImage()
{
  if (m_ColorMap != NULL)
    delete[] m_ColorMap;
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
// Metadonnees de l'image sous forme de cles / valeurs
//-----------------------------------------------------------------------------
std::string XBaseImage::Metadata()
{
	std::ostringstream out;
	out << "Largeur:" << m_nW << ";Hauteur:" << m_nH << ";Nb Bits:" << m_nNbBits
		<< ";Nb Sample:" << m_nNbSample << ";Xmin:" << m_dX0 << ";Ymax:" << m_dY0
    << ";GSD:" << m_dGSD <<";";
	return out.str();
}

//-----------------------------------------------------------------------------
// Allocation d'une zone de pixel
//-----------------------------------------------------------------------------
byte* XBaseImage::AllocArea(uint32 w, uint32 h)
{
	return new byte[w * h * PixSize()];
}

//-----------------------------------------------------------------------------
// Recuperation des valeurs brutes des pixels sur une ROI
//-----------------------------------------------------------------------------
bool XBaseImage::GetRawArea(XFile* file, uint32 x, uint32 y, uint32 w, uint32 h, float* pix,
                            uint32* nb_sample, uint32 factor)
{
  if (factor == 0)
    return false;
  uint32 wout = w / factor, hout = h / factor;
  byte* area = AllocArea(wout, hout);
  if (area == NULL)
    return false;
  if (!GetZoomArea(file, x, y, w, h, area, factor)) {
    delete[] area;
    return false;
  }
  *nb_sample = NbSample();
  float* pix_ptr = pix;

  if (NbBits() <= 8) {
    byte* pix_area = area;
    for (uint32 i = 0; i < wout * hout * NbSample(); i++) {
      *pix_ptr = *pix_area;
      pix_ptr++;
      pix_area++;
    }
  }
  if (NbBits() == 16) {
    uint16* pix_area = (uint16*)area;
    for (uint32 i = 0; i < wout * hout * NbSample(); i++) {
      *pix_ptr = *pix_area;
      pix_ptr++;
      pix_area++;
    }
  }
  if (NbBits() == 32)
    ::memcpy(pix, area, wout * hout * NbSample() * sizeof(float));

  delete[] area;
  return true;
}

//-----------------------------------------------------------------------------
// Recuperation des valeurs brutes des pixels autour d'une position
//-----------------------------------------------------------------------------
bool XBaseImage::GetRawPixel(XFile* file, uint32 x, uint32 y, uint32 win, double* pix, uint32* nb_sample)
{
  if ((win > x)||(win > y))
    return false;
  if ((x+win >= m_nW)||(y+win >= m_nH))
    return false;
  float* area = new float[(2*win+1)*(2*win+1)* NbSample()];
  if (area == NULL)
    return false;
  if (!GetRawArea(file, x - win, y - win, (2*win+1), (2*win+1),area, nb_sample)) {
    delete[] area;
    return false;
  }
  for (uint32 i = 0; i < (2*win+1) * (2*win+1) * NbSample(); i++)
    pix[i] = area[i];
  delete[] area;
  return true;
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
// Conversion 16bits -> 8bits
//-----------------------------------------------------------------------------
bool XBaseImage::Uint16To8bits(byte* buffer, uint32 w, uint32 h, uint16 min, uint16 max)
{
	// Recherche du min / max
	uint16 val_min = min, val_max = max;
	if ((min == 0) && (max == 0)) {
		val_min = 0xFFFF;
		val_max = 0;
		uint16* ptr = (uint16*)buffer;
		for (uint32 i = 0; i < w * h; i++) {
			val_min = XMin(*ptr, val_min);
			val_max = XMax(*ptr, val_max);
			ptr++;
		}
	}
	if (val_max == 0)
		return true;
	// Application de la transformation
	uint16* ptr_val = (uint16*)buffer;
	byte* ptr_buf = buffer;
	for (uint32 i = 0; i < w * h; i++) {
		*ptr_buf = (*ptr_val - val_min) * 255 / val_max;
		ptr_buf++;
		ptr_val++;
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

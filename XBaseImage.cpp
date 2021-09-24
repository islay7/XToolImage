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
// Calcul les statistiques de l'image
//-----------------------------------------------------------------------------
bool XBaseImage::GetStat(XFile* file, double minVal[4], double maxVal[4], double meanVal[4], uint32 noData[4],
                         uint16* nb_sample, double no_data)
{
  uint32 row = RowH();
  byte* line = AllocArea(m_nW, row);
  if (line == NULL)
    return false;
  uint32 nbRow = m_nH / row;
  if ((m_nH % row) != 0) nbRow++;
  *nb_sample = NbSample();
  for (uint32 k = 0; k < NbSample(); k++) {
    noData[k] = 0;
    meanVal[k] = 0.;
    if (NbBits() <= 16)
      maxVal[k] = 0.;
    else
      maxVal[k] = -1.e31;
    if (NbBits() == 8) minVal[k] = 255.;
    if (NbBits() == 16) minVal[k] = 255. * 255.;
    if (NbBits() == 32) minVal[k] = 1.e31;
  }
  for (uint32 i = 0; i < nbRow; i++) {
    uint32 rowH = row;
    if (i == (nbRow - 1)) // derniere bande
      rowH = (m_nH % row);
    GetArea(file, 0, i*row, m_nW, rowH, line);
    if (NbBits() <= 8) {
      byte* ptr = line;
      for (uint32 j = 0; j < m_nW * rowH; j++) {
        for (uint32 k = 0; k < NbSample(); k++) {
          minVal[k] = XMin(minVal[k], (double)*ptr);
          maxVal[k] = XMax(maxVal[k], (double)*ptr);
          meanVal[k] += (double)*ptr;
          ptr++;
        }
      }
      continue;
    }
    if (NbBits() == 16) {
      uint16* ptr = (uint16*)line;
      for (uint32 j = 0; j < m_nW * rowH; j++) {
        for (uint32 k = 0; k < NbSample(); k++) {
          minVal[k] = XMin(minVal[k], (double)*ptr);
          maxVal[k] = XMax(maxVal[k], (double)*ptr);
          meanVal[k] += (double)*ptr;
          ptr++;
        }
      }
      continue;
    }
    if (NbBits() == 32) {
      float* ptr = (float*)line;
      for (uint32 j = 0; j < m_nW * rowH; j++) {
        for (uint32 k = 0; k < NbSample(); k++) {
          if (*ptr > no_data) {
            minVal[k] = XMin(minVal[k], (double)*ptr);
            maxVal[k] = XMax(maxVal[k], (double)*ptr);
            meanVal[k] += (double)*ptr;
          }
          else
            noData[k] += 1;
          ptr++;
        }
      }
      continue;
    }
  }
  for (uint32 k = 0; k < NbSample(); k++)
    meanVal[k] /= XMax((m_nW * m_nH - noData[k]), (uint32)1);
  delete[] line;
  return true;
}

//-----------------------------------------------------------------------------
// Application d'une palette de couleurs
//-----------------------------------------------------------------------------
bool XBaseImage::ApplyColorMap(byte* in, byte* out, uint32 w, uint32 h)
{
  if ((m_ColorMap == NULL)||(m_nColorMapSize < 3*256))
    return false;
  byte *ptr_in = in, *ptr_out = out;
  for (uint32 i = 0; i < h; i++) {
    for (uint32 j = 0; j < w; j++) {
      ptr_out[0] = m_ColorMap[*ptr_in] / 256;
      ptr_out[1] = m_ColorMap[*ptr_in + 256] / 256;
      ptr_out[2] = m_ColorMap[*ptr_in + 512] / 256;
      ptr_out += 3;
      ptr_in++;
    }
  }
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
// Extraction d'une zone de pixels
//-----------------------------------------------------------------------------
bool XBaseImage::ExtractArea(byte* in, byte* out, uint32 win, uint32 hin, uint32 wout, uint32 hout,
                             uint32 x0, uint32 y0)
{
  if (x0 + wout > win) return false;
  if (y0 + hout > hin) return false;

  byte* buf_in = &in[y0 * win + x0];
  byte* buf_out= out;
  for (uint32 i = 0; i < hout; i++) {
    ::memcpy(buf_out, buf_in, wout);
    buf_in += win;
    buf_out+= wout;
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

//-----------------------------------------------------------------------------
// Normalisation d'une zone de pixels et recuperation des statistiques
//-----------------------------------------------------------------------------
void XBaseImage::Normalize(byte* pix_in, double* pix_out, uint32 nb_pixel, double* mean, double* std_dev)
{
  *mean = 0.0;
  *std_dev = 0.0;
  for (uint32 i = 0;  i < nb_pixel; i++)
    (*mean) += (double)pix_in[i];
  (*mean) /= (double)nb_pixel;
  for (uint32 i = 0;  i < nb_pixel; i++) {
    pix_out[i] = (double)pix_in[i] - (*mean);
    (*std_dev) += (pix_out[i] * pix_out[i]);
  }
  (*std_dev) /= (double)nb_pixel;
}

//-----------------------------------------------------------------------------
// Covariance entre deux zones de pixels
//-----------------------------------------------------------------------------
double XBaseImage::Covariance(double* pix1, double* pix2, uint32 nb_pixel)
{
  double cov = 0.0;
  for (uint32 i = 0; i < nb_pixel; i++)
    cov += (pix1[i] * pix2[i]);
  return (cov / (double)nb_pixel);
}

//-----------------------------------------------------------------------------
// Fonction de correlation
//-----------------------------------------------------------------------------
bool XBaseImage::Correlation(byte* pix1, uint32 w1, uint32 h1,
                             byte* pix2, uint32 w2, uint32 h2,
                             double* u, double* v, double* pic)
{
  double *win1, *win2;	// Fenetres de correlation
  double *correl;				// Valeurs de correlation
  double mean1, dev1, mean2, dev2, cov, a, b;
  uint32 k = 0, n = 0, lin = 0, col = 0;
  byte *buf;

  win1 = new double[w1 * h1];
  win2 = new double[w1 * h1];
  correl = new double[(h2 - h1 + 1)*(w2 - w1 + 1)];
  buf = new byte[w1 * h1];
  if ((win1 == NULL)||(win2 == NULL)||(correl == NULL)||(buf == NULL)){
    delete[] win1; delete[] win2; delete[] correl; delete[] buf;
    return false;
  }
  Normalize(pix1, win1, w1 * h1, &mean1, &dev1);
  if (dev1 == 0.0) {
    delete[] win1; delete[] win2; delete[] correl; delete[] buf;
    return false;
  }

  *pic = 0.0;
  for (uint32 i = 0; i < h2 - h1; i++)
    for (uint32 j = 0; j < w2 - w1; j++) {
      ExtractArea(pix2, buf, w2, h2, w1, h1, j, i);
      Normalize(buf, win2,  w1 * h1, &mean2, &dev2);
      if (dev2 == 0.0)
        continue;
      cov = Covariance(win1, win2, w1 * h1);
      correl[n] = cov / sqrt(dev1 * dev2);
      if (correl[n] > *pic) {
        k = n;
        *pic = correl[n];
        lin = i;
        col = j;
      }
      n++;
    }

  delete[] win1;
  delete[] win2;
  delete[] buf;

  if ((k == 0)||(k == ((h2 - h1 + 1)*(w2 - w1 + 1)-1))) {
    delete[] correl;
    return false;
  }

  a = correl[k-1] - *pic;
  b = correl[k+1] - *pic;
  a = (a - b) / (2.0 * (a + b));
  *u = (double)col + a + (double)w1 * 0.5;
  uint32 wK = w2 - w1 + 1;
  a = correl[k-wK] - *pic;
  b = correl[k+wK] - *pic;
  a = (a - b) / (2.0 * (a + b));
  *v = (double)lin + a + (double)h1 * 0.5;

  delete[] correl;
  return true;
}

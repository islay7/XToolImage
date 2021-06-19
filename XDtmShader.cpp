//-----------------------------------------------------------------------------
//								XDtmShader.cpp
//								==============
//
// Auteur : F.Becirspahic - IGN / DSI / SIMV
//
// 18/06/2021
//
// Shader pour convertir un MNT 32bits en image RGB
//-----------------------------------------------------------------------------

#include "XDtmShader.h"
#include <cmath>

// Preferences d'affichage
double XDtmShader::m_dNoData = -999.;
double XDtmShader::m_dZ0 = 0.;
double XDtmShader::m_dZ1 = 200.;
double XDtmShader::m_dZ2 = 400.;
double XDtmShader::m_dZ3 = 600.;
double XDtmShader::m_dZ4 = 5500.;
int XDtmShader::m_nMode = 1;

byte XDtmShader::m_HZColor[3] = { 255, 0, 0 };
byte XDtmShader::m_Z0Color[3] = { 3, 34, 76 };
byte XDtmShader::m_ZColor[15] = { 64, 128, 128, 255, 255, 0, 255, 128, 0,
                                   128, 64, 0, 240, 240, 240 };

void XDtmShader::SetPref32Bits(double nodata, double z0, double z1, double z2,
  double z3, double z4, int mode)
{
  m_dNoData = nodata;
  m_dZ0 = z0;
  m_dZ1 = z1;
  m_dZ2 = z2;
  m_dZ3 = z3;
  m_dZ4 = z4;
  m_nMode = mode;
}

void XDtmShader::SetCol32Bits(byte* Color, bool set)
{
  if (set) {
    memcpy(m_HZColor, Color, 3 * sizeof(byte));
    memcpy(m_Z0Color, &Color[3], 3 * sizeof(byte));
    memcpy(m_ZColor, &Color[6], 15 * sizeof(byte));
  }
  else {
    memcpy(Color, m_HZColor, 3 * sizeof(byte));
    memcpy(&Color[3], m_Z0Color, 3 * sizeof(byte));
    memcpy(&Color[6], m_ZColor, 15 * sizeof(byte));
  }
}

//-----------------------------------------------------------------------------
// Fonction d'estompage d'un pixel à partir de l'altitude du pixel, de l'altitude du pixel juste au dessus,
// de l'altitude du pixel juste à sa droite
//-----------------------------------------------------------------------------
double XDtmShader::CoefEstompage(float altC, float altH, float altD, float angleH, float angleV)
{
  double deltaX = m_dGSD, deltaY = m_dGSD; // angleH = 135, angleV = 45;
  if (m_dGSD <= 0.)
    deltaX = deltaY = 25.;
  if ((m_dGSD > 0.) && (m_dGSD < 0.1))    // Donnees en geographiques
    deltaX = deltaY = m_dGSD * 111319.49;  // 1 degre a l'Equateur

  // Recherche de la direction de la normale a la surface du mnt
  double dY[3], dX[3], normale[3];

  // Initialisation des vecteurs de la surface du pixel considéré
  dY[0] = 0;
  dY[1] = deltaY;
  dY[2] = (double)(altH - altC);

  dX[0] = deltaX;
  dX[1] = 0;
  dX[2] = (double)(altD - altC);

  // Direction de la normale a la surface
  normale[0] = -(dY[1] * dX[2]);
  normale[1] = -(dX[0] * dY[2]);
  normale[2] = dX[0] * dY[1];

  // Determination de l'angle entre la normale et la direction de la lumière
  // Modification des teintes en fonction de cet angle
  double correction;
  double DirLum[3];

  // Passage d'une représentation angulaire de la direction de la lumiere en représentation cartésienne
  DirLum[0] = cos(XPI / 180 * angleV * -1) * sin(XPI / 180 * angleH);
  DirLum[1] = cos(XPI / 180 * angleV * -1) * cos(XPI / 180 * angleH);
  DirLum[2] = sin(XPI / 180 * angleV * -1);

  // Correction correspond au cosinus entre la normale et la lumiere
  double scalaire;
  scalaire = (normale[0] * DirLum[0]) + (normale[1] * DirLum[1]) + (normale[2] * DirLum[2]);

  double normenormale;
  double normeDirLum;
  normenormale = (normale[0] * normale[0]) + (normale[1] * normale[1]) + (normale[2] * normale[2]);

  normeDirLum = (DirLum[0] * DirLum[0]) + (DirLum[1] * DirLum[1]) + (DirLum[2] * DirLum[2]);
  if (!normenormale == 0 && !normeDirLum == 0)
    correction = scalaire / sqrt(normenormale * normeDirLum);
  else correction = 0;

  // Traduction de cette correction en correction finale
  if (correction >= 0)
    correction = 0;
  else correction = -correction;

  return correction;
}

//-----------------------------------------------------------------------------
// Calcul de la pente
//-----------------------------------------------------------------------------
double XDtmShader::CoefPente(float altC, float altH, float altD)
{
  double d = m_dGSD;
  if (d <= 0.) d = 25;
  double costheta = d / sqrt((altD - altC) * (altD - altC) + (altH - altC) * (altH - altC) + d * d);
  return costheta;
}

//-----------------------------------------------------------------------------
// Indique si l'on a une isohypse
//-----------------------------------------------------------------------------
int XDtmShader::Isohypse(float altC, float altH, float altD)
{
  double step = m_dZ1 - m_dZ0;
  int nb_isoC = ceil(altC / step);
  int nb_isoH = ceil(altH / step);
  int nb_isoD = ceil(altD / step);
  int nb_iso = XRint(altC / step);

  if (nb_isoC != nb_isoH)
    return nb_iso;
  if (nb_isoC != nb_isoD)
    return nb_iso;

  return -9999;
}

//-----------------------------------------------------------------------------
// Calcul de l'estompage sur une ligne
//-----------------------------------------------------------------------------
bool XDtmShader::EstompLine(float* lineR, float* lineS, float* lineT, uint32 W, byte* rgb, uint32 num)
{
  float* ptr = lineS;
  double coef, r, g, b, val;
  int index = 0, nb_iso;
  for (uint32 i = 0; i < W; i++) {
    val = *ptr;
    r = g = b = 255;
    if (val <= m_dNoData) { // Hors zone
      ::memcpy(&rgb[3 * i], m_HZColor, 3 * sizeof(byte));
      ptr++;
      continue;
    }
    if (val <= m_dZ0) { // Sous la mer
      coef = (m_dZ0 - val);
      index = -1;
      r = m_Z0Color[0];
      g = m_Z0Color[1];
      b = m_Z0Color[2];
    }

    if ((val > m_dZ0) && (val < m_dZ1)) {
      coef = (m_dZ1 - val) / (m_dZ1 - m_dZ0);
      index = 0;
      r = m_ZColor[0] * coef + m_ZColor[3] * (1 - coef);
      g = m_ZColor[1] * coef + m_ZColor[4] * (1 - coef);
      b = m_ZColor[2] * coef + m_ZColor[5] * (1 - coef);
    }
    if ((val >= m_dZ1) && (val < m_dZ2)) {
      coef = (m_dZ2 - val) / (m_dZ2 - m_dZ1);
      index = 3;
      r = m_ZColor[3] * coef + m_ZColor[6] * (1 - coef);
      g = m_ZColor[4] * coef + m_ZColor[7] * (1 - coef);
      b = m_ZColor[5] * coef + m_ZColor[8] * (1 - coef);
    }
    if ((val >= m_dZ2) && (val < m_dZ3)) {
      coef = (m_dZ3 - val) / (m_dZ3 - m_dZ2);
      index = 6;
      r = m_ZColor[6] * coef + m_ZColor[9] * (1 - coef);
      g = m_ZColor[7] * coef + m_ZColor[10] * (1 - coef);
      b = m_ZColor[8] * coef + m_ZColor[11] * (1 - coef);
    }
    if ((val >= m_dZ3) && (val < m_dZ4)) {
      coef = (m_dZ4 - val) / (m_dZ4 - m_dZ3);
      index = 9;
      r = m_ZColor[9] * coef + m_ZColor[12] * (1 - coef);
      g = m_ZColor[10] * coef + m_ZColor[13] * (1 - coef);
      b = m_ZColor[11] * coef + m_ZColor[14] * (1 - coef);
    }

    if (val >= m_dZ4) {
      index = 12;
    }

    switch (m_nMode) {
    case 0: coef = 1.;
      break;
    case 1: // Estompage
      if (num == 0) {
        if (i < (W - 1))
          coef = CoefEstompage(lineT[i], val, lineT[i + 1]);
        else
          coef = CoefEstompage(lineS[i - 1], lineR[i - 1], val);
      }
      else {
        if (i < (W - 1))
          coef = CoefEstompage(val, lineR[i], lineS[i + 1]);
        else
          coef = CoefEstompage(lineS[i - 1], lineR[i - 1], val);
      }
      break;
    case 2: // Pente
      if (num == 0) {
        if (i < (W - 1))
          coef = CoefPente(lineT[i], val, lineT[i + 1]);
        else
          coef = CoefPente(lineS[i - 1], lineR[i - 1], val);
      }
      else {
        if (i < (W - 1))
          coef = CoefPente(val, lineR[i], lineS[i + 1]);
        else
          coef = CoefPente(lineS[i - 1], lineR[i - 1], val);
      }
      break;
    case 3: // Aplat de couleurs
      coef = 1.;
      if (index >= 0) {
        r = m_ZColor[index];
        g = m_ZColor[index + 1];
        b = m_ZColor[index + 2];
      }
      break;
    case 4: // Aplat + estompage
      if (num == 0) {
        if (i < (W - 1))
          coef = CoefEstompage(lineT[i], val, lineT[i + 1]);
        else
          coef = CoefEstompage(lineS[i - 1], lineR[i - 1], val);
      }
      else {
        if (i < (W - 1))
          coef = CoefEstompage(val, lineR[i], lineS[i + 1]);
        else
          coef = CoefEstompage(lineS[i - 1], lineR[i - 1], val);
      }
      if (index >= 0) {
        r = m_ZColor[index];
        g = m_ZColor[index + 1];
        b = m_ZColor[index + 2];
      }
      break;
    case 5: // Isohypses
      coef = 1.;
      r = g = b = 255;
      if (num == 0) {
        if (i < (W - 1))
          nb_iso = Isohypse(lineT[i], val, lineT[i + 1]);
        else
          nb_iso = Isohypse(lineS[i - 1], lineR[i - 1], val);
      }
      else {
        if (i < (W - 1))
          nb_iso = Isohypse(val, lineR[i], lineS[i + 1]);
        else
          nb_iso = Isohypse(lineS[i - 1], lineR[i - 1], val);
      }

      if (nb_iso > -9999) {
        double cote = nb_iso * (m_dZ1 - m_dZ0);
        if (cote > m_dZ4) index = 12;
        if (cote <= m_dZ4) index = 9;
        if (cote <= m_dZ3) index = 6;
        if (cote <= m_dZ2) index = 3;
        if (cote <= m_dZ1) index = 0;
        if (cote <= m_dZ0) index = -1;

        if (index >= 0) {
          r = m_ZColor[index];
          g = m_ZColor[index + 1];
          b = m_ZColor[index + 2];
        }
        else {
          r = m_Z0Color[index];
          g = m_Z0Color[index + 1];
          b = m_Z0Color[index + 2];
        }
      }

      break;
    case 6: // Estompage leger
      if (num == 0) {
        if (i < (W - 1))
          coef = CoefEstompage(lineT[i], val, lineT[i + 1], 135., 65.);
        else
          coef = CoefEstompage(lineS[i - 1], lineR[i - 1], val, 135., 65.);
      }
      else {
        if (i < (W - 1))
          coef = CoefEstompage(val, lineR[i], lineS[i + 1], 135., 65.);
        else
          coef = CoefEstompage(lineS[i - 1], lineR[i - 1], val, 135., 65.);
      }
      break;
    }

    rgb[3 * i] = r * coef;
    rgb[3 * i + 1] = g * coef;
    rgb[3 * i + 2] = b * coef;
    ptr++;
  }

  return true;
}


//-----------------------------------------------------------------------------
// Calcul de l'estompage
//-----------------------------------------------------------------------------
bool XDtmShader::ConvertArea(float* area, uint32 w, uint32 h, byte* rgb)
{
  EstompLine(area, area, &area[w], w, rgb, 0);
  for (uint32 i = 1; i < h - 1; i++) {
    EstompLine(&area[(i - 1) * w], &area[i * w], &area[(i + 1) * w], w, &rgb[i * w * 3L], i);
  }
  EstompLine(&area[(h - 2) * w], &area[(h - 1) * w], &area[(h - 1) * w], w, &rgb[(h-1) * w * 3L], h-1);
  return true;
}

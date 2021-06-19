//-----------------------------------------------------------------------------
//								XDtmShader.h
//								============
//
// Auteur : F.Becirspahic - IGN / DSI / SIMV
//
// 18/06/2021
//
// Shader pour convertir un MNT 32bits en image RGB
//-----------------------------------------------------------------------------

#ifndef XDTMSHADER_H
#define XDTMSHADER_H

#include "../XTool/XBase.h"

class XDtmShader {
protected:
  double CoefEstompage(float altC, float altH, float altD, float angleH = 135., float angleV = 45.);
  double CoefPente(float altC, float altH, float altD);
  int Isohypse(float altC, float altH, float altD);

  bool EstompLine(float* lineR, float* lineS, float* lineT, uint32 w, byte* rgb, uint32 num);

  static double m_dNoData;
  static double m_dZ0;
  static double m_dZ1;
  static double m_dZ2;
  static double m_dZ3;
  static double m_dZ4;
  static int    m_nMode;
  static byte   m_HZColor[3];
  static byte   m_Z0Color[3];
  static byte   m_ZColor[15];

  double  m_dGSD;   // Pas terrain du MNT

public:
  XDtmShader(double gsd = 25.) { m_dGSD = gsd; }

  static void SetPref32Bits(double nodata, double z0, double z1, double z2,
    double z3, double z4, int mode);
  static void SetCol32Bits(byte* Color, bool set = true);

  bool ConvertArea(float* area, uint32 w, uint32 h, byte* rgb);

};

#endif // XDTMSHADER_H
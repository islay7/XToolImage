// XImageViewer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
//#include "XJp2Image.h"
#include "../XTool/XTiffWriter.h"
#include "XTiffTileImage.h"
#include "XTiffStripImage.h"
#include "XDtmShader.h"

/*
int TestJP2()
{
  std::string filename;
  std::cout << "Nom du fichier : ";
  std::cin >> filename;

  XJp2Image* image = new XJp2Image(filename.c_str());
  if (image->IsValid() != true) {
    delete image;
    return -1;
  }
  std::cout << "Largeur : " << image->Width() << std::endl;
  std::cout << "Hauteur : " << image->Height() << std::endl;
  double xmin = 0., ymax = 0., gsd = 0.;
  image->GetGeoref(&xmin, &ymax, &gsd);
  std::cout << "Xmin : " << xmin << std::endl;
  std::cout << "Ymax : " << ymax << std::endl;
  std::cout << "Resolution : " << gsd << std::endl;

  XFile file;
  file.Open(filename.c_str(), std::ios_base::binary);
  byte* area = new byte[200 * 200 * image->NbByte()];
  image->GetArea(&file, 0, 0, 200, 200, area);
  XTiffWriter tiff;
  tiff.Write("test.tif", 200, 200, 3, 8, area);

  delete[] area;
  delete image;
  return 0;
}
*/
void TestTif()
{
  std::string filename;
  std::cout << "Nom du fichier : ";
  std::cin >> filename;
  if (filename.size() < 2)
    //   filename = "D:\\Data\\SkySat_Freeport_s03_20170831T162740Z3.tif";
    filename = "D:\\Data\\Images_Test\\COG\\74-2020-0916-6550-LA93-0M20-RVB-E100_cog90.tif";
    //filename = "D:\\mire_cog.tif";

  XFile file;
  if (!file.Open(filename.c_str(), std::ios::in | std::ios::binary)) {
    std::cerr << "Impossible d'ouvrir le fichier" << std::endl;
    return;
  }
  XTiffReader reader;
  if (!reader.Read(file.IStream())) {
    std::cerr << "Lecture TIFF impossible" << std::endl;
    return;
  }
  if (!reader.AnalyzeIFD(file.IStream())) {
    std::cerr << "Analyse de l'IFD impossible" << std::endl;
    return;
  }

  XTiffTileImage image;
  image.SetTiffReader(&reader);

  uint32 W = 5000, H = 5000, X0 = 0, Y0 = 0;
  byte* area = new byte[W * H * 3L];
  image.GetArea(&file, X0, Y0, W, H, area);
  file.Close();
 
  double R, G, B, Y, Cb, Cr;
  for (uint32 i = 0; i < H; i++) {
    for (uint32 j = 0; j < W; j++) {
      Y = area[(i * W + j) * 3L];
      Cb = (double)area[(i * W + j) * 3L + 1];
      Cr = (double)area[(i * W + j) * 3L + 2];
      R = Y + 1.402 * (Cr - 128.);
      G = Y - 0.34414 * (Cb - 128.) - 0.71414 * (Cr - 128.);
      B = Y + 1.772 * (Cb - 128.);
      if (R < 0) R = 0;
      if (R > 255) R = 255;
      if (G < 0) G = 0;
      if (G > 255) G = 255;
      if (B < 0) B = 0;
      if (B > 255) B = 255;
      area[(i * W + j) * 3L] = R;
      area[(i * W + j) * 3L + 1] = G;
      area[(i * W + j) * 3L + 2] = B;
    }
  }
  
  XTiffWriter tiff;
  tiff.Write("test.tif", W, H, 3, 8, area);

  delete[] area;
}

void TestTifStrip()
{
  std::string filename;
  std::cout << "Nom du fichier : ";
  std::cin >> filename;
  if (filename.size() < 2)
    //   filename = "D:\\Data\\SkySat_Freeport_s03_20170831T162740Z3.tif";
    filename = "D:\\Data\\Images_Test\\TIFF\\TIF_JPEG.tif";

  XFile file;
  if (!file.Open(filename.c_str(), std::ios::in | std::ios::binary)) {
    std::cerr << "Impossible d'ouvrir le fichier" << std::endl;
    return;
  }
  XTiffReader reader;
  if (!reader.Read(file.IStream())) {
    std::cerr << "Lecture TIFF impossible" << std::endl;
    return;
  }
  if (!reader.AnalyzeIFD(file.IStream())) {
    std::cerr << "Analyse de l'IFD impossible" << std::endl;
    return;
  }

  XTiffStripImage image;
  image.SetTiffReader(&reader);

  byte* area = new byte[1000 * 1000 * 3L];
  image.GetArea(&file, 49, 49, 1000, 1000, area);
  file.Close();
  XTiffWriter tiff;
  tiff.Write("test.tif", 1000, 1000, 3, 8, area);

  delete[] area;
}

void PrintTifInfo()
{
  std::string filename;
  std::cout << "Nom du fichier : ";
  std::cin >> filename;
  if (filename.size() < 2)
    //  filename = "D:\\Data\\\\Images_Test\\COG\\SkySat_Freeport_s03_20170831T162740Z3.tif";
      filename = "D:\\Data\\Images_Test\\COG\\74-2020-0916-6550-LA93-0M20-RVB-E100_cog90.tif";
    //filename = "D:\\mire_cog.tif";

  XFile file;
  if (!file.Open(filename.c_str(), std::ios::in | std::ios::binary)) {
    std::cerr << "Impossible d'ouvrir le fichier" << std::endl;
    return;
  }
  XTiffReader reader;
  if (!reader.Read(file.IStream())) {
    std::cerr << "Lecture TIFF impossible" << std::endl;
    return;
  }
  reader.PrintIFDTag(&std::cout);
  for (uint32 i = 0; i < reader.NbIFD(); i++) {
    std::cout << "***** IFD " << i << std::endl;
    reader.SetActiveIFD(i);
    reader.AnalyzeIFD(file.IStream());
    reader.PrintInfo(&std::cout);
  }
}

void BuildTestImage()
{
  uint32 W = 2000, H = 2000, X0 = 0, Y0 = 0;
  byte* area = new byte[W * H * 3L];
  std::memset(area, 0, W * H * 3L);

  // Blanc
  for (uint32 i = 0; i < 256; i++) {
    for (uint32 j = 0; j < 2000; j++) {
      area[(i * W + j) * 3L] = 255;
      area[(i * W + j) * 3L+1] = 255;
      area[(i * W + j) * 3L+2] = 255;
    }
  }

  // Gris
  for (uint32 i = 256; i < 512; i++) {
    for (uint32 j = 0; j < 2000; j++) {
      area[(i * W + j) * 3L] = 127;
      area[(i * W + j) * 3L + 1] = 127;
      area[(i * W + j) * 3L + 2] = 127;
    }
  }

  // Rouge
  for (uint32 i = 512; i < 768; i++) {
    for (uint32 j = 0; j < 2000; j++) {
      area[(i * W + j) * 3L] = 255;
      area[(i * W + j) * 3L + 1] = 0;
      area[(i * W + j) * 3L + 2] = 0;
    }
  }

  // Vert
  for (uint32 i = 768; i < 1024; i++) {
    for (uint32 j = 0; j < 2000; j++) {
      area[(i * W + j) * 3L] = 0;
      area[(i * W + j) * 3L + 1] = 255;
      area[(i * W + j) * 3L + 2] = 0;
    }
  }

  // Bleu
  for (uint32 i = 1024; i < 1280; i++) {
    for (uint32 j = 0; j < 2000; j++) {
      area[(i * W + j) * 3L] = 0;
      area[(i * W + j) * 3L + 1] = 0;
      area[(i * W + j) * 3L + 2] = 255;
    }
  }

  // Bleu
  for (uint32 i = 1024; i < 1280; i++) {
    for (uint32 j = 0; j < 2000; j++) {
      area[(i * W + j) * 3L] = 0;
      area[(i * W + j) * 3L + 1] = 0;
      area[(i * W + j) * 3L + 2] = 255;
    }
  }

  XTiffWriter tiff;
  tiff.Write("test.tif", 2000, 2000, 3, 8, area);

  delete[] area;
}

// Test de la lecture d'une image TIFF
// On lit l'image integralement et on cree un fichier TIFF non compresse avec le contenu
bool TestTifImage(std::string file_in, std::string file_out)
{
  XFile file;
  if (!file.Open(file_in.c_str(), std::ios::in | std::ios::binary)) {
    std::cerr << "Impossible d'ouvrir le fichier " << file_in << std::endl;
    return false;
  }
  XTiffReader reader;
  if (!reader.Read(file.IStream())) {
    std::cerr << "Lecture TIFF impossible de " << file_in << std::endl;
    return false;
  }
  if (!reader.AnalyzeIFD(file.IStream())) {
    std::cerr << "Analyse de l'IFD impossible de " << file_in << std::endl;
    return false;
  }

  XBaseImage* image;
  if (reader.RowsPerStrip() == 0) { // Tile
    XTiffTileImage* tile_image = new XTiffTileImage;
    if (!tile_image->SetTiffReader(&reader)) {
      std::cerr << "Lecture de l'image TIFF tile impossible de " << file_in << std::endl;
      return false;
    }
    image = tile_image;
  }
  else {  // Strip
    XTiffStripImage* strip_image = new XTiffStripImage;
    if (!strip_image->SetTiffReader(&reader)) {
      std::cerr << "Lecture de l'image TIFF strip impossible de " << file_in << std::endl;
      return false;
    }
    image = strip_image;
  }

  uint32 W = XMin(image->W(), (uint32)2000), H = XMin(image->H(), (uint32)2000), X0 = 0, Y0 = 0;
  byte* area = image->AllocArea(W, H);
  std::memset(area, 255, W * H * image->PixSize());
  if (!image->GetArea(&file, X0, Y0, W, H, area)) {
    delete[] area;
    file.Close();
    std::cerr << "GetArea impossible de " << file_in << std::endl;
    return false;
  }
  file.Close();

  XTiffWriter tiff;
  uint16 nbSample = image->NbSample(), nbBits = image->NbBits();
  if (nbSample == 4) { // CMYK
    image->CMYK2RGB(area, W, H);
    nbSample = 3;
  }
  if ((nbSample == 1)&&(nbBits == 32)){ // MNT 32 bits
    XDtmShader dtm(image->GSD());
    byte* rgb = new byte[W * H * 3L];
    dtm.ConvertArea((float*)area, W, H, rgb);
    delete[] area;
    area = rgb;
    
    nbSample = 3;
    nbBits = 8;
  }


  tiff.Write(file_out.c_str(), W, H, nbSample, nbBits, area);

  delete[] area;
  delete[] image;
  std::cout << "Traitement reussi de " << file_in << std::endl;
  return true;
}

int main()
{
  //BuildTestImage();
  std::cout << "Hello World!\n";
    
  TestTifImage("D:\\Data\\Images_Test\\TIFF\\TIF_DEFLATE.tif", "TIF_DEFLATE.tif");
  TestTifImage("D:\\Data\\Images_Test\\TIFF\\TIF_DEFLATE_8bits.tif", "TIF_DEFLATE_8bits.tif");
  TestTifImage("D:\\Data\\Images_Test\\TIFF\\TIF_JPEG.tif", "TIF_JPEG.tif");
  TestTifImage("D:\\Data\\Images_Test\\TIFF\\TIF_LZW.tif", "TIF_LZW.tif");
  TestTifImage("D:\\Data\\Images_Test\\TIFF\\TIF_LZW_8bits.tif", "TIF_LZW_8bits.tif");
  TestTifImage("D:\\Data\\Images_Test\\TIFF\\TIF_PACKBIT.tif", "TIF_PACKBIT.tif");
  TestTifImage("D:\\Data\\Images_Test\\TIFF\\TIF_PACKBIT_8bits.tif", "TIF_PACKBIT_8bits.tif");
  TestTifImage("D:\\Data\\Images_Test\\TIFF\\TIF_SANS_COMPRESSION.tif", "TIF_SANS_COMPRESSION.tif");
  TestTifImage("D:\\Data\\Images_Test\\TIFF\\TIF_SANS_COMPRESSION_8bits.tif", "TIF_SANS_COMPRESSION_8bits.tif");
  TestTifImage("D:\\Data\\Images_Test\\TIFF\\1BIT_DEFLATE.tif", "1BIT_DEFLATE.tif");
  TestTifImage("D:\\Data\\Images_Test\\TIFF\\1BIT_LZW.tif", "1BIT_LZW.tif");
  TestTifImage("D:\\Data\\Images_Test\\TIFF\\1BIT_PACKBIT.tif", "1BIT_PACKBIT.tif");
  TestTifImage("D:\\Data\\Images_Test\\TIFF\\1BIT_SANS_COMPRESSION.tif", "1BIT_SANS_COMPRESSION.tif");
  TestTifImage("D:\\Data\\Images_Test\\COG\\74-2020-0916-6550-LA93-0M20-RVB-E100_cog90.tif", "COG1.tif");
  TestTifImage("D:\\Data\\Images_Test\\COG\\SkySat_Freeport_s03_20170831T162740Z3.tif", "COG2.tif");
  TestTifImage("D:\\Data\\Images_Test\\TIFF\\TIF_DEFLATE_cmyk.tif", "TIF_DEFLATE_cmyk.tif");
  TestTifImage("D:\\Data\\Images_Test\\MNT\\Litto3D_MNT_Lamb93_IGN69_0707_6157.tif", "Litto3D_MNT_Lamb93_IGN69_0707_6157.tif");

  return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
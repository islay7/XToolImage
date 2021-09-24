// XImageViewer.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "XJpeg2000Image.h"
#include "../XTool/XTiffWriter.h"
#include "XTiffTileImage.h"
#include "XTiffStripImage.h"
#include "XCogImage.h"
#include "XDtmShader.h"
#include "XBigTiffReader.h"
#include "XFileImage.h"

int TestEnergyLevel()
{
  std::string filename;
  std::cout << "Nom de l'image : ";
  std::cin >> filename;
  if (filename.size() < 2)
    filename = "D:\\Temp\\MNS\\Ortho_RGB.jp2";

  XFileImage image;
  if (!image.AnalyzeImage(filename)) {
    std::cerr << "Impossible d'ouvrir l'image" << std::endl;
    return -1;
  }

  std::cout << "Nom du fichier de sortie : ";
  std::cin >> filename;
  if (filename.size() < 2)
    filename = "D:\\TestEnergyLevel.tif";

  uint32 W = image.Width();
  uint32 H = image.Height();
  float* lineF = new float[W];
  uint32 win = 5; // La fenetre a une taille de 2 x win + 1

  XTiffWriter writer;
  double xmin, ymax, gsd;
  image.GetGeoref(&xmin, &ymax, &gsd);
  writer.SetGeoTiff(xmin, ymax, gsd);
  writer.Write(filename.c_str(), W, H, 1, 32);
  std::ofstream file;
  file.open(filename.c_str(), std::ios::out | std::ios::binary | std::ios::app);

  byte* area = new byte[W * (2 * win + 1) * image.NbByte()];
  image.GetArea(0, 0, W, (2 * win + 1), area);

  byte* pix_in = new byte[(2 * win + 1) * (2 * win + 1)];
  double* pix_out = new double[(2 * win + 1) * (2 * win + 1)];
  double mean, std_dev;
  uint32 offset = 0;
  if (image.NbByte() > 1)
    offset = 1;

  for (uint32 i = 0; i < H; i++) {
    ::memset(lineF, 0, W * sizeof(float));
    if ((i <= win) || (i >= H - win)) {
      file.write((char*)lineF, W * sizeof(float));
      continue;
    }
    for (uint32 j = 0; j < W; j++) {
      if ((j <= win) || (j >= H - win)) {
        lineF[j] = 0.;
        continue;
      }
      
      for (uint32 k = 0; k < (2 * win + 1); k++)
        for (uint32 p = 0; p < (2 * win + 1); p++)
          pix_in[k * (2 * win + 1) + p] = area[k * W * image.NbByte() + (j - win + p) * image.NbByte() + offset];

      XBaseImage::Normalize(pix_in, pix_out, (2 * win + 1) * (2 * win + 1), &mean, &std_dev);
      lineF[j] = (float)std_dev;
    }
    file.write((char*)lineF, W * sizeof(float));

    ::memmove(area, &area[W * image.NbByte()], W * (2 * win) * image.NbByte());
    image.GetArea(0, i + win + 1, W, 1, &area[W * (2 * win) * image.NbByte()]);
  }

  delete[] pix_out;
  delete[] pix_in;
  delete[] area;
  delete[] lineF;


  return 0;
}

int TestRotation()
{
  std::string filename;
  std::cout << "Nom du fichier : ";
  std::cin >> filename;
  if (filename.size() < 2)
    filename = "D:\\Data\\Images_Test\\TIFF\\TIF_JPEG.tif";

  XFile file;
  if (!file.Open(filename.c_str(), std::ios::in | std::ios::binary)) {
    std::cerr << "Impossible d'ouvrir le fichier" << std::endl;
    return -1;
  }
  XTiffReader reader;
  if (!reader.Read(file.IStream())) {
    std::cerr << "Lecture TIFF impossible" << std::endl;
    return -1;
  }
  if (!reader.AnalyzeIFD(file.IStream())) {
    std::cerr << "Analyse de l'IFD impossible" << std::endl;
    return -1;
  }

  XTiffStripImage image;
  if (!image.SetTiffReader(&reader)) {
    std::cerr << "Ce n'est pas une image TIFF Strip" << std::endl;
    return -1;
  }

  uint32 W = image.W();
  uint32 H = image.H();
  byte* area = new byte[W * H * image.NbSample()];
  byte* area_rot = new byte[W * H * image.NbSample()];
  image.GetArea(&file, 0, 0, W, H, area);
  file.Close();

  XTiffWriter tiff;
  XBaseImage::RotateArea(area, area_rot, W, H, image.NbSample(), 1);
  tiff.Write("test_rot90.tif", H, W, image.NbSample(), 8, area_rot);
  XBaseImage::RotateArea(area, area_rot, W, H, image.NbSample(), 2);
  tiff.Write("test_rot180.tif", W, H, image.NbSample(), 8, area_rot);
  XBaseImage::RotateArea(area, area_rot, W, H, image.NbSample(), 3);
  tiff.Write("test_rot270.tif", H, W, image.NbSample(), 8, area_rot);

  delete[] area;
  delete[] area_rot;
  return 1;
}

int TestJP2()
{
  std::string filename;
  std::cout << "Nom du fichier : ";
  std::cin >> filename;
  if (filename.size() < 2)
    filename = "D:\\Data\\PCRS\\Orthos\\56-2019-0229-6764-LA93-0M05-RVB-E100.jp2";

  XJpeg2000Image* image = new XJpeg2000Image(filename.c_str());
  if (image->IsValid() != true) {
    delete image;
    return -1;
  }
  std::cout << "Largeur : " << image->W() << std::endl;
  std::cout << "Hauteur : " << image->H() << std::endl;
  double xmin = 0., ymax = 0., gsd = 0.;
  
  std::cout << "Xmin : " << image->X0() << std::endl;
  std::cout << "Ymax : " << image->Y0() << std::endl;
  std::cout << "Resolution : " << image->GSD() << std::endl;

  uint32 W = XMin(image->W(), (uint32)2000), H = XMin(image->H(), (uint32)2000), X0 = 10000, Y0 = 10000;
  byte* area = image->AllocArea(W, H);
  std::memset(area, 255, W * H * image->PixSize());

  XFile file;
  file.Open(filename.c_str(), std::ios_base::binary);
  image->GetArea(&file, X0, Y0, W, H, area);
  XTiffWriter tiff;
  tiff.Write("test_JPEG2000.tif", W, H, image->NbSample(), 8, area);

  delete[] area;
  delete image;
  return 1;
}

void TestCog()
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
  XCogImage cog;
  if (!cog.Open(&file)) {
    std::cerr << "Impossible d'ouvrir l'image COG" << std::endl;
    return;
  }
 
  uint32 W = 2000, H = 2000, X0 = 0, Y0 = 0;
  byte* area = new byte[W * H * 3L];
  cog.GetArea(&file, X0, Y0, W, H, area);
  file.Close();

  XTiffWriter tiff;
  tiff.Write("testCOG.tif", W, H, 3, 8, area);

  delete[] area;
}

void TestZoomCog()
{
  std::string filename;
  std::cout << "Nom du fichier : ";
  std::cin >> filename;
  if (filename.size() < 2)
    //   filename = "D:\\Data\\SkySat_Freeport_s03_20170831T162740Z3.tif";
    //filename = "D:\\Data\\Images_Test\\COG\\74-2020-0916-6550-LA93-0M20-RVB-E100_cog90.tif";
  //filename = "D:\\mire_cog.tif";
    filename = "Z:\\proto_33\\DETECTION_DL\\blaye\\33_2015_urbain_b9.tif";

  XFile file;
  if (!file.Open(filename.c_str(), std::ios::in | std::ios::binary)) {
    std::cerr << "Impossible d'ouvrir le fichier" << std::endl;
    return;
  }
  XCogImage cog;
  if (!cog.Open(&file)) {
    std::cerr << "Impossible d'ouvrir l'image COG" << std::endl;
    return;
  }

  uint32 factor = 100;
  uint32 W = cog.W() / factor, H = cog.H() / factor;
  byte* area = new byte[W * H * cog.NbSample()];
  cog.GetZoomArea(&file, 0, 0, cog.W(), cog.H(), area, factor);
  file.Close();

  XTiffWriter tiff;
  tiff.Write("testZoomCOG.tif", W, H, cog.NbSample(), 8, area);

  delete[] area;
}

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

void PrintBigTifInfo()
{
  std::string filename;
  std::cout << "Nom du fichier : ";
  std::cin >> filename;
  if (filename.size() < 2)
    //filename = "D:\\Temp\\BigTiff\\arcachon_nord_modele_multiclasses_urbain_b1.tif";
    filename = "Z:\\proto_33\\DETECTION_DL\\blaye\\33_2015_urbain_b9.tif";

  XFile file;
  if (!file.Open(filename.c_str(), std::ios::in | std::ios::binary)) {
    std::cerr << "Impossible d'ouvrir le fichier" << std::endl;
    return;
  }
  XBigTiffReader reader;
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
      delete tile_image;
      return false;
    }
    image = tile_image;
  }
  else {  // Strip
    XTiffStripImage* strip_image = new XTiffStripImage;
    if (!strip_image->SetTiffReader(&reader)) {
      std::cerr << "Lecture de l'image TIFF strip impossible de " << file_in << std::endl;
      delete strip_image;
      return false;
    }
    image = strip_image;
  }

  // Lecture des statistiques
  double minVal[4], maxVal[4], meanVal[4];
  uint32 noData[4];
  uint16 nbSpl;
  image->GetStat(&file, minVal, maxVal, meanVal, noData, &nbSpl);
  for (uint16 i = 0; i < nbSpl; i++) {
    std::cout << "Canal " << i << " : " << "min = " << minVal[i] << "\tmax = " << maxVal[i]
      << "\tmean = " << meanVal[i] << std::endl;
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
  if ((nbSample == 1) && (nbBits == 1)) // Images 1 bit -> conversion en 8 bits
    nbBits = 8;
  if ((nbSample == 1) && (nbBits == 16)) { // Image 16 bits
    byte* gray = new byte[W * H];
    
    uint16* ptr = (uint16*)area;
    byte* ptr_gray = gray;
    for (uint32 i = 0; i < W * H; i++) {
      *ptr_gray = (*ptr) / 256;
      ptr++;
      ptr_gray++;
    }

    delete[] area;
    area = gray;

    nbSample = 1;
    nbBits = 8;
  }

  tiff.Write(file_out.c_str(), W, H, nbSample, nbBits, area);

  delete[] area;
  delete[] image;
  std::cout << "Traitement reussi de " << file_in << std::endl;
  return true;
}

// Test de la lecture d'une image TIFF
// On lit l'image et on prend une ROI avec un zoom et on cree un fichier TIFF non compresse avec le contenu
bool TestTifZoomImage(std::string file_in, std::string file_out)
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

  uint32 factor = 2;
  uint32 X0 = 200, Y0 = 200;
  uint32 W = XMin(image->W() / factor, (uint32)2000) - X0, H = XMin(image->H() / factor, (uint32)2000) - Y0;
  byte* area = image->AllocArea(W , H );
  std::memset(area, 255, W * H * image->PixSize());
  if (!image->GetZoomArea(&file, X0, Y0, W * factor, H * factor, area, factor)) {
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
  if ((nbSample == 1) && (nbBits == 32)) { // MNT 32 bits
    XDtmShader dtm(image->GSD());
    byte* rgb = new byte[W * H * 3L];
    dtm.ConvertArea((float*)area, W, H, rgb);
    delete[] area;
    area = rgb;

    nbSample = 3;
    nbBits = 8;
  }
  if ((nbSample == 1) && (nbBits == 1)) // Images 1 bit -> conversion en 8 bits
    nbBits = 8;
  if ((nbSample == 1) && (nbBits == 16)) { // Image 16 bits
    byte* gray = new byte[W * H];

    uint16* ptr = (uint16*)area;
    byte* ptr_gray = gray;
    for (uint32 i = 0; i < W * H; i++) {
      *ptr_gray = (*ptr) / 256;
      ptr++;
      ptr_gray++;
    }

    delete[] area;
    area = gray;

    nbSample = 1;
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

  //return TestEnergyLevel();
 
  //PrintBigTifInfo();

  //TestZoomCog();
  //TestTifImage("D:\\Temp\\MNT LZW\\MNE_18FD3325_365_6383.TIF", "MNE_18FD3325_365_6383.TIF");
  TestTifImage("C:\\Users\\FBecirspahic\\Downloads\\RGEALTI_FXX_073_675_MNT_LAMB93_IGN69.tif", "RGEALTI_FXX_073_675_MNT_LAMB93_IGN69.TIF");

  //return TestRotation();
  //return TestJP2();
  /*
  TestTifZoomImage("D:\\Data\\Images_Test\\TIFF\\TIF_DEFLATE.tif", "TIF_DEFLATE.tif");
  TestTifImage("D:\\Data\\Images_Test\\TIFF\\TIF_DEFLATE_8bits.tif", "TIF_DEFLATE_8bits.tif");
  TestTifImage("D:\\Data\\Images_Test\\TIFF\\TIF_JPEG.tif", "TIF_JPEG.tif");
  TestTifImage("D:\\Data\\Images_Test\\TIFF\\TIF_LZW.tif", "TIF_LZW.tif");
  TestTifImage("D:\\Data\\Images_Test\\TIFF\\TIF_LZW_8bits.tif", "TIF_LZW_8bits.tif");
  TestTifImage("D:\\Data\\Images_Test\\TIFF\\TIF_PACKBIT.tif", "TIF_PACKBIT.tif");
  TestTifImage("D:\\Data\\Images_Test\\TIFF\\TIF_PACKBIT_8bits.tif", "TIF_PACKBIT_8bits.tif");
  TestTifZoomImage("D:\\Data\\Images_Test\\TIFF\\TIF_SANS_COMPRESSION.tif", "TIF_SANS_COMPRESSION.tif");
  TestTifImage("D:\\Data\\Images_Test\\TIFF\\TIF_SANS_COMPRESSION_8bits.tif", "TIF_SANS_COMPRESSION_8bits.tif");
  TestTifImage("D:\\Data\\Images_Test\\TIFF\\1BIT_DEFLATE.tif", "1BIT_DEFLATE.tif");
  TestTifImage("D:\\Data\\Images_Test\\TIFF\\1BIT_LZW.tif", "1BIT_LZW.tif");
  TestTifImage("D:\\Data\\Images_Test\\TIFF\\1BIT_PACKBIT.tif", "1BIT_PACKBIT.tif");
  TestTifImage("D:\\Data\\Images_Test\\TIFF\\1BIT_SANS_COMPRESSION.tif", "1BIT_SANS_COMPRESSION.tif");
  TestTifImage("D:\\Data\\Images_Test\\COG\\74-2020-0916-6550-LA93-0M20-RVB-E100_cog90.tif", "COG1.tif");
  TestTifImage("D:\\Data\\Images_Test\\COG\\SkySat_Freeport_s03_20170831T162740Z3.tif", "COG2.tif");
  TestTifImage("D:\\Data\\Images_Test\\TIFF\\TIF_DEFLATE_cmyk.tif", "TIF_DEFLATE_cmyk.tif");
  TestTifImage("D:\\Data\\Images_Test\\MNT\\Litto3D_MNT_Lamb93_IGN69_0707_6157.tif", "Litto3D_MNT_Lamb93_IGN69_0707_6157.tif");
  TestTifZoomImage("D:\\Data\\Images_Test\\COG\\74-2020-0916-6550-LA93-0M20-RVB-E100_cog90.tif", "COGZoom.tif");
  */
  //TestTifImage("D:\\Data\\SENTINEL\\T31UDR\\SENTINEL2B_20190401-105716-455_L2A_T31UDR_C_V2-0\\SENTINEL2B_20190401-105716-455_L2A_T31UDR_C_V2-0_FRE_B2.tif", "SENTINEL.TIF");
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

#ifndef PIXEL_OCR_IDCARD_H_
#define PIXEL_OCR_IDCARD_H_

#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <atlstr.h>
#include <vector>
#include <opencv2/opencv.hpp>
#include <allheaders.h>
#include "baseapi.h"

using namespace cv;
using namespace std;

class OCRIDCard 
{
public:
	int init();
	int recognizeIDCard(Mat srcImage, vector<string>& resultStr);

private:
	tesseract::TessBaseAPI* chinese_handle;
	tesseract::TessBaseAPI* eng_handle;

	CString UTF82WCS(const char* szU8);

	vector<Mat> cutIDCard(Mat grayIDImage, CvRect refRect, int refX, int refY, vector<CvRect> candidateRects);

	vector<Mat> preProcess(vector<Mat> imageList);

	Mat thresholdProcess(Mat inputImage);

	int getThresholdValue(Mat inputImage, int& downMaxVal, int& upMaxVal, int& minVal);

	vector<Mat> seperateAddrStr(Mat srcAddrImg);
	string recognizeAddr(vector<Mat> addrImgList);

	vector<Mat> seperateBirthStr(Mat srcBirthImg);
	string recognizeBirth(vector<Mat> birthImgList);

	void adaptiveHistEqual(Mat &src, Mat &dst, double clipLimit);

	Mat adaptiveMediamBlur(Mat srcImage, int kernelRadius, float mediamRate);
};

bool compX(const CvRect &a, const CvRect &b);
bool compY(const CvRect &a, const CvRect &b);
bool compVecY(const vector<CvRect> &a, const vector<CvRect> &b);

#endif // PIXEL_OCR_IDCARD_H_
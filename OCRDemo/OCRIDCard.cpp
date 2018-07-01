#include "OCRIDCard.h"

int OCRIDCard::init() 
{
	int ret = 0;

	CString exe_path;
	::GetModuleFileName(NULL, exe_path.GetBuffer(MAX_PATH), MAX_PATH);
	exe_path.ReleaseBuffer();
	exe_path = exe_path.Left(exe_path.ReverseFind(_T('\\')));
	exe_path += _T("\\tessdata");
	USES_CONVERSION;
	char* tessdataPath = T2A(exe_path);

	chinese_handle = new tesseract::TessBaseAPI();
	if (chinese_handle == nullptr)
		return -1;
	ret = chinese_handle->Init(tessdataPath, "chi_sim", tesseract::OEM_TESSERACT_ONLY);
	if (ret != 0) return ret;
	chinese_handle->SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);//PSM_SINGLE_LINE PSM_SINGLE_BLOCK

	eng_handle = new tesseract::TessBaseAPI();
	if (eng_handle == nullptr)
		return -1;
	ret = eng_handle->Init(tessdataPath, "eng", tesseract::OEM_TESSERACT_ONLY);
	if (ret != 0) return ret;
	eng_handle->SetPageSegMode(tesseract::PSM_SINGLE_LINE);
	return 0;
}

CString OCRIDCard::UTF82WCS(const char* szU8)
{
	//预转换，得到所需空间的大小;
	int wcsLen = ::MultiByteToWideChar(CP_UTF8, NULL, szU8, strlen(szU8), NULL, 0);

	//分配空间要给'\0'留个空间，MultiByteToWideChar不会给'\0'空间
	wchar_t* wszString = new wchar_t[wcsLen + 1];

	//转换
	::MultiByteToWideChar(CP_UTF8, NULL, szU8, strlen(szU8), wszString, wcsLen);

	//最后加上'\0'
	wszString[wcsLen] = '\0';

	CString unicodeString(wszString);

	delete[] wszString;
	wszString = NULL;

	return unicodeString;
}

//直方图均衡
void OCRIDCard::adaptiveHistEqual(Mat &src, Mat &dst, double clipLimit)
{
	Ptr<cv::CLAHE> clahe = createCLAHE();
	clahe->setClipLimit(clipLimit);
	clahe->apply(src, dst);
}

vector<Mat> OCRIDCard::cutIDCard(Mat grayIDImage, CvRect refRect, int refX, int refY, vector<CvRect> candidateRects)
{
	vector<Mat> imgList;

	if (grayIDImage.channels() != 1)
		cvtColor(grayIDImage, grayIDImage, CV_RGB2GRAY);

	int width = grayIDImage.size().width;
	int height = grayIDImage.size().height;

	Mat nameImg, genderImg, nationImg, addrImg, idnumImg, birthImg;
	int gerderFlag = -1, nationFlag = -1, addrFlag = -1, idnumFlag = -1, birthFlag = -1;

	int cutX, cutY, cutWidth, cutHeight;
	int extendX, extendY;
	float extendScale = 6;

	//裁剪姓名
	extendX = max(1, int(round(min(refRect.width, refRect.height) / extendScale)));
	extendY = extendX;
	cutX = max(0, refRect.x - extendX);
	cutY = max(0, refRect.y - extendY);
	cutWidth = min(grayIDImage.cols - 1 - cutX, refRect.width + 2 * extendX);
	cutHeight = min(grayIDImage.rows - 1 - cutY, refRect.height + 2 * extendY);
	nameImg = grayIDImage(Rect(cutX, cutY, cutWidth, cutHeight));
	imgList.push_back(nameImg);

	int YDiff = round(grayIDImage.size().height / 15.0);
	int XDiff = round(grayIDImage.size().width / 18.0);
	for (int i = 0; i < candidateRects.size(); i++)
	{
		//裁剪性别
		if (abs(candidateRects.at(i).x - refRect.x) < XDiff&&abs(candidateRects.at(i).y - (refRect.y + refRect.height*7.0 / 3.0)) < YDiff)
		{
			extendX = max(5, int(round(min(candidateRects.at(i).width, candidateRects.at(i).height) / extendScale)));
			extendY = extendX;
			cutX = max(0, candidateRects.at(i).x - extendX);
			cutY = max(0, candidateRects.at(i).y - extendY);
			cutWidth = min(grayIDImage.cols - 1 - cutX, candidateRects.at(i).width + 2 * extendX);
			cutHeight = min(grayIDImage.rows - 1 - cutY, candidateRects.at(i).height + 2 * extendY);
			genderImg = grayIDImage(Rect(cutX, cutY, cutWidth, cutHeight));
			gerderFlag = 1;
		}

		//裁剪民族
		if (abs(((refRect.x + refX) / 2.0 + refRect.height*190.0 / 30.0) - (candidateRects.at(i).x + candidateRects.at(i).width)) < XDiff&&
			abs((refRect.y + refRect.height*105.0 / 33.0) - (candidateRects.at(i).y + candidateRects.at(i).height)) < YDiff)
		{
			cutX = candidateRects.at(i).x + candidateRects.at(i).width - candidateRects.at(i).height*5.0 / 4.0;
			cutY = candidateRects.at(i).y - candidateRects.at(i).height / 4.0;
			cutWidth = min(grayIDImage.cols - 1 - cutX, int(round(candidateRects.at(i).height*3.0 / 2.0)));
			cutHeight = min(grayIDImage.rows - 1 - cutY, int(round(candidateRects.at(i).height*3.0 / 2.0)));
			nationImg = grayIDImage(Rect(cutX, cutY, cutWidth, cutHeight));
			nationFlag = 1;
		}

		//裁剪住址
		if (abs(candidateRects.at(i).x - refRect.x) < XDiff&&
			abs(refRect.y + refRect.height*216.0 / 35.0 - candidateRects.at(i).y) < YDiff)
		{
			extendX = max(5, int(round(min(candidateRects.at(i).width, candidateRects.at(i).height) / extendScale)));
			extendY = extendX;
			cutX = max(0, candidateRects.at(i).x - extendX);
			cutY = max(0, candidateRects.at(i).y - extendY);
			cutWidth = min(grayIDImage.cols - 1 - cutX, candidateRects.at(i).width + 2 * extendX);
			cutHeight = min(grayIDImage.rows - 1 - cutY, candidateRects.at(i).height + 2 * extendY);
			addrImg = grayIDImage(Rect(cutX, cutY, cutWidth, cutHeight));
			addrFlag = 1;
		}

		//裁剪身份证号码
		if (float(candidateRects.at(i).height) / candidateRects.at(i).width > 0.053&&
			float(candidateRects.at(i).height) / candidateRects.at(i).width < 0.077&&
			abs(refRect.y + refRect.height*390.0 / 35.0 - candidateRects.at(i).y)<YDiff&&
			abs(refRect.x + refRect.height*140.0 / 37.0 - candidateRects.at(i).x)<XDiff)
		{
			extendX = max(3, int(round(candidateRects.at(i).width / 35)));
			extendY = extendX;
			cutX = max(0, candidateRects.at(i).x - extendX);
			cutY = max(0, candidateRects.at(i).y - extendY);
			cutWidth = min(grayIDImage.cols - 1 - cutX, candidateRects.at(i).width + 2 * extendX);
			cutHeight = min(grayIDImage.rows - 1 - cutY, candidateRects.at(i).height + 2 * extendY);
			idnumImg = grayIDImage(Rect(cutX, cutY, cutWidth, cutHeight));
			idnumFlag = 1;
		}

		//裁剪出生日期
		if (abs(refRect.x - candidateRects.at(i).x) < XDiff&&
			abs(refRect.y + refRect.height*137.0 / 30.0 - candidateRects.at(i).y) < YDiff)
		{
			extendX = max(2, int(round(min(candidateRects.at(i).width, candidateRects.at(i).height) / extendScale)));
			extendY = extendX;
			cutX = max(0, candidateRects.at(i).x - extendX);
			cutY = max(0, candidateRects.at(i).y - extendY);
			cutWidth = min(grayIDImage.cols - 1 - cutX, int(round(candidateRects.at(i).height* 295.0 / 23.0)));
			cutHeight = min(grayIDImage.rows - 1 - cutY, candidateRects.at(i).height + 2 * extendY);
			birthImg= grayIDImage(Rect(cutX, cutY, cutWidth, cutHeight));
			birthFlag = 1;
		}
	}

	//裁剪性别
	if (gerderFlag == -1)
	{
		cutX = int(round(width * 150 / 887));
		cutY = int(round(height * 150 / 568));
		cutWidth = int(round(width * 75 / 887));
		cutHeight = int(round(height * 50 / 568));
		genderImg = grayIDImage(Rect(cutX, cutY, cutWidth, cutHeight));
	}
	imgList.push_back(genderImg);

	//裁剪民族
	if (nationFlag == -1)
	{
		cutX = int(round(width * 335 / 887));
		cutY = int(round(height * 150 / 568));
		cutWidth = int(round(width * 115 / 887));
		cutHeight = int(round(height * 50 / 568));
		nationImg = grayIDImage(Rect(cutX, cutY, cutWidth, cutHeight));
	}
	imgList.push_back(nationImg);

	//裁剪住址
	if (addrFlag == -1)
	{
		cutX = int(round(width * 150 / 887));
		cutY = int(round(height * 275 / 568));
		cutWidth = int(round(width * 385 / 887));
		cutHeight = int(round(height * 165 / 568));
		addrImg = grayIDImage(Rect(cutX, cutY, cutWidth, cutHeight));
	}
	imgList.push_back(addrImg);

	//裁剪身份证号码
	if (idnumFlag == -1)
	{
		cutX = int(round(width * 285 / 887));
		cutY = int(round(height * 440 / 568));
		cutWidth = int(round(width * 540 / 887));
		cutHeight = int(round(height * 90 / 568));
		idnumImg = grayIDImage(Rect(cutX, cutY, cutWidth, cutHeight));
	}
	imgList.push_back(idnumImg);

	//裁剪出生日期
	if (birthFlag == -1)
	{
		cutX = int(round(width * 150 / 887));
		cutY = int(round(height * 210 / 568));
		cutWidth = int(round(width * 270 / 887));
		cutHeight = int(round(height * 50 / 568));
		birthImg = grayIDImage(Rect(cutX, cutY, cutWidth, cutHeight));
	}
	imgList.push_back(birthImg);

	return imgList;
}

Mat OCRIDCard::thresholdProcess(Mat inputImage)
{
	Mat outputImage = Mat::zeros(inputImage.size(), CV_8UC1);//黑底白字图片

	int downMaxVal, upMaxVal, minVal;
	getThresholdValue(inputImage, downMaxVal, upMaxVal, minVal);

	int thres = int(round((upMaxVal*0.35 + downMaxVal*0.65)*0.4 + minVal*0.6));//minVal + (0.5 - float(minVal - downMaxVal) / (upMaxVal - downMaxVal))*minVal/1.5;
	for (int i = 0; i < inputImage.rows; i++)
	{
		for (int j = 0; j < inputImage.cols; j++)
		{
			if (inputImage.at<uchar>(i, j) < thres)
				outputImage.at<uchar>(i, j) = 255;
		}
	}

	return outputImage;
}
int OCRIDCard::getThresholdValue(Mat inputImage, int& downMaxVal, int& upMaxVal, int& minVal)
{
	if (inputImage.channels() != 1)
		cvtColor(inputImage, inputImage, CV_BGR2GRAY);

	//直方图统计范围
	Mat hist;
	int histSize = 256;
	float range[] = { 0, 256 };
	const float* histRange = { range };
	bool uniform = true;
	bool accumulate = false;

	cv::calcHist(&inputImage, 1, 0, Mat(), hist, 1, &histSize, &histRange, uniform, accumulate);

	int *array = new int[256];
	float *smoothArray = new float[256];
	Mat smoothHist(256, 1, CV_32FC1);
	for (int i = 0; i < 256; i++)
	{
		array[i] = hist.at<float>(i);
		smoothArray[i] = hist.at<float>(i);
	}

	int ratio = 7;//直方图平滑半径
	for (int i = ratio; i < 256 - ratio; i++)
	{
		int sum = 0;
		for (int j = -ratio; j < ratio; j++)
		{
			sum = sum + array[i + j];
		}
		smoothArray[i] = sum / float(2 * ratio);
	}

	//计算smoothArray的总和与累计分布
	int smoothSum = 0;
	int cumIndex = 0;
	double *cumSmoothArray = new double[256];
	for (int i = 0; i < 256; i++)
	{
		cumSmoothArray[i] = smoothSum;
		smoothSum = smoothSum + smoothArray[i];
	}
	for (int i = 0; i < 256; i++)
	{
		cumSmoothArray[i] = cumSmoothArray[i] / double(smoothSum);
		if (cumSmoothArray[i] > 0.01&&cumIndex == 0)
			cumIndex = i;
	}

	////////////////////
	for (int i = 0; i < 256; i++)
	{
		smoothHist.at<float>(i) = smoothArray[i];
	}

	// 创建直方图画布  
	int hist_w = 256; int hist_h = 256;
	int bin_w = cvRound((double)hist_w / histSize);

	Mat histImage1(hist_w, hist_h, CV_8UC3, Scalar(0, 0, 0));
	Mat histImage2(hist_w, hist_h, CV_8UC3, Scalar(0, 0, 0));

	/// 将直方图归一化到范围 [ 0, histImage.rows ]  
	normalize(hist, hist, 0, histImage1.rows, NORM_MINMAX, -1, Mat());
	normalize(smoothHist, smoothHist, 0, histImage2.rows, NORM_MINMAX, -1, Mat());

	/// 在直方图画布上画出直方图  
	/*for (int i = 1; i < histSize; i++)
	{
		line(histImage1, Point(bin_w*(i - 1), hist_h - cvRound(hist.at<float>(i - 1))),
			Point(bin_w*(i), hist_h - cvRound(hist.at<float>(i))),
			Scalar(0, 0, 255), 1, 8, 0);
		line(histImage2, Point(bin_w*(i - 1), hist_h - cvRound(smoothHist.at<float>(i - 1))),
			Point(bin_w*(i), hist_h - cvRound(smoothHist.at<float>(i))),
			Scalar(0, 255, 0), 1, 8, 0);
	}*/
	//////////////////////////

	///////////////找出极大值点和极小值点///////////////
	vector<int> minMaxArray;
	int tempIndex = 0;
	float diff = 0.00;
	//确定[0]的极大极小属性，极大值以正数记录，极小值以负数记录
	if (smoothArray[0] > smoothArray[1] + diff)//[0]为极大值
	{
		minMaxArray.push_back(1);
	}
	else if (smoothArray[0] < smoothArray[1] - diff)//[0]为极小值
	{
		minMaxArray.push_back(-1);
	}
	else//smoothArray[0] == smoothArray[1]
	{
		for (int i = 1; i < 256; i++)
		{
			if (smoothArray[i] > smoothArray[i + 1] + diff)//[0]为极大值
			{
				minMaxArray.push_back(i);
				break;
			}
			if (smoothArray[i] < smoothArray[i + 1] - diff)//[0]为极小值
			{
				minMaxArray.push_back(-i);
				break;
			}
		}
	}

	//继续寻找极大极小值
	int medIndex = -1;
	for (int i = abs(minMaxArray.at(0)) + 1; i < 255; i++)
	{
		//极大值点
		if (((smoothArray[i] >= smoothArray[i - 1] + diff && smoothArray[i] > smoothArray[i + 1] + diff) ||
			(smoothArray[i] > smoothArray[i - 1] + diff && smoothArray[i] >= smoothArray[i + 1] + diff)) &&
			((smoothArray[i] >= smoothArray[i - 2] + diff && smoothArray[i] > smoothArray[i + 2] + diff) ||
				(smoothArray[i] > smoothArray[i - 2] + diff && smoothArray[i] >= smoothArray[i + 2] + diff)) &&
			((smoothArray[i] >= smoothArray[i - 3] + diff && smoothArray[i] > smoothArray[i + 3] + diff) ||
				(smoothArray[i] > smoothArray[i - 3] + diff && smoothArray[i] >= smoothArray[i + 3] + diff)) &&
			((smoothArray[i] >= smoothArray[i - 4] + diff && smoothArray[i] > smoothArray[i + 4] + diff) ||
				(smoothArray[i] > smoothArray[i - 4] + diff && smoothArray[i] >= smoothArray[i + 4] + diff)) &&
			((smoothArray[i] >= smoothArray[i - 5] + diff && smoothArray[i] > smoothArray[i + 5] + diff) ||
				(smoothArray[i] > smoothArray[i - 5] + diff && smoothArray[i] >= smoothArray[i + 5] + diff)))
		{
			if (minMaxArray.at(minMaxArray.size() - 1) > 0)
			{
				if (i - (minMaxArray.at(minMaxArray.size() - 1)) < 20)
				{
					tempIndex = int(round((i + minMaxArray.at(minMaxArray.size() - 1)) / 2.0));
					minMaxArray.pop_back();
					minMaxArray.push_back(tempIndex);
				}
				else
				{
					minMaxArray.push_back(i);
				}
			}
			else
			{
				minMaxArray.push_back(i);
			}
		}
		//极小值点
		if (((smoothArray[i] <= smoothArray[i - 1] - diff&& smoothArray[i] < smoothArray[i + 1] - diff) ||
			(smoothArray[i] < smoothArray[i - 1] - diff&& smoothArray[i] <= smoothArray[i + 1] - diff)) &&
			((smoothArray[i] <= smoothArray[i - 2] - diff&& smoothArray[i] < smoothArray[i + 2] - diff) ||
				(smoothArray[i] < smoothArray[i - 2] - diff&& smoothArray[i] <= smoothArray[i + 2] - diff)) &&
			((smoothArray[i] <= smoothArray[i - 3] - diff&& smoothArray[i] < smoothArray[i + 3] - diff) ||
				(smoothArray[i] < smoothArray[i - 3] - diff&& smoothArray[i] <= smoothArray[i + 3] - diff)) &&
			((smoothArray[i] <= smoothArray[i - 4] - diff&& smoothArray[i] < smoothArray[i + 4] - diff) ||
				(smoothArray[i] < smoothArray[i - 4] - diff&& smoothArray[i] <= smoothArray[i + 4] - diff)) &&
			((smoothArray[i] <= smoothArray[i - 5] - diff&& smoothArray[i] < smoothArray[i + 5] - diff) ||
				(smoothArray[i] < smoothArray[i - 5] - diff&& smoothArray[i] <= smoothArray[i + 5] - diff)))
		{
			if (minMaxArray.at(minMaxArray.size() - 1) < 0)
			{
				if ((i + minMaxArray.at(minMaxArray.size() - 1)) < 20)
				{
					tempIndex = -int(round((i - minMaxArray.at(minMaxArray.size() - 1)) / 2.0));
					minMaxArray.pop_back();
					minMaxArray.push_back(tempIndex);
				}
				else
				{
					minMaxArray.push_back(-i);
				}
			}
			else
			{
				minMaxArray.push_back(-i);
			}
		}
	}
	for (int i = 0; i < minMaxArray.size(); i++)
	{
		if (minMaxArray.at(i) > 128)
		{
			if (i >= 2 && minMaxArray.at(i - 2) > 0)
			{
				medIndex = minMaxArray.at(i - 2);
			}
			else
			{
				medIndex = minMaxArray.at(i);
			}
			break;
		}
	}

	//确定[255]的极大极小属性
	if (smoothArray[255] > smoothArray[254] + diff)//极大值
	{
		minMaxArray.push_back(255);
	}
	else if (smoothArray[255] < smoothArray[254] - diff)//极小值
	{
		minMaxArray.push_back(-255);
	}
	else//smoothArray[255] == smoothArray[254]
	{
		for (int i = 254; i>0; i--)
		{
			if (smoothArray[i] > smoothArray[i - 1] + diff)//极大值
			{
				if (minMaxArray.at(minMaxArray.size() - 1) > 0)
				{
					tempIndex = int(round((i + minMaxArray.at(minMaxArray.size() - 1)) / 2.0));
					minMaxArray.pop_back();
					minMaxArray.push_back(tempIndex);
				}
				else
				{
					minMaxArray.push_back(i);
				}
				break;
			}
			if (smoothArray[i] < smoothArray[i - 1] - diff)//极小值
			{
				if (minMaxArray.at(minMaxArray.size() - 1) < 0)
				{
					tempIndex = -int(round((i - minMaxArray.at(minMaxArray.size() - 1)) / 2.0));
					minMaxArray.pop_back();
					minMaxArray.push_back(tempIndex);
				}
				else
				{
					minMaxArray.push_back(-i);
				}
				break;
			}
		}
	}

	//////////计算极大值点处的斜率//////////
	double gradient = 0.0;
	vector<double> gradArray;
	int medGradIndex = -1, cumGradIndex = -1;

	//第一个点为极大值点
	if (minMaxArray.at(0) > 0)
	{
		int tempMin0 = 0;
		if (minMaxArray.at(1) < 0)
		{
			gradient = double(smoothArray[abs(minMaxArray.at(0))] - smoothArray[abs(minMaxArray.at(1))]) / (abs(minMaxArray.at(1)) - abs(minMaxArray.at(0)));
			gradArray.push_back(minMaxArray.at(0));
			gradArray.push_back(gradient);
		}
		else
		{
			for (int i = minMaxArray.at(0) + 1; i < minMaxArray.at(1); i++)
			{
				if ((smoothArray[i] <= smoothArray[i - 1] && smoothArray[i] < smoothArray[i + 1]) ||
					(smoothArray[i] < smoothArray[i - 1] && smoothArray[i] <= smoothArray[i + 1]))
				{
					tempMin0 = i;
					break;
				}
			}
			gradient = double(smoothArray[abs(minMaxArray.at(0))] - smoothArray[tempMin0]) / (tempMin0 - abs(minMaxArray.at(0)));
			gradArray.push_back(minMaxArray.at(0));
			gradArray.push_back(gradient);
		}
	}
	for (int i = 1; i < minMaxArray.size() - 1; i++)
	{
		if (minMaxArray.at(i) > 0)
		{
			int tempMinLft = 0, tempMinRgt = 0;
			if (minMaxArray.at(i - 1) < 0 && minMaxArray.at(i + 1) < 0)
			{
				gradient = max(double(smoothArray[abs(minMaxArray.at(i))] - smoothArray[abs(minMaxArray.at(i - 1))]) / (abs(minMaxArray.at(i)) - abs(minMaxArray.at(i - 1))),
					double(smoothArray[abs(minMaxArray.at(i))] - smoothArray[abs(minMaxArray.at(i + 1))]) / (abs(minMaxArray.at(i + 1)) - abs(minMaxArray.at(i))));
				gradArray.push_back(minMaxArray.at(i));
				gradArray.push_back(gradient);
			}
			else if (minMaxArray.at(i - 1) > 0 && minMaxArray.at(i + 1) < 0)//找左边极小值
			{
				for (int m = minMaxArray.at(i - 1) + 1; m < minMaxArray.at(i); m++)
				{
					if ((smoothArray[m] <= smoothArray[m - 1] && smoothArray[m] < smoothArray[m + 1]) ||
						(smoothArray[m] < smoothArray[m - 1] && smoothArray[m] <= smoothArray[m + 1]))
					{
						tempMinLft = m;
						break;
					}
				}
				gradient = max(double(smoothArray[abs(minMaxArray.at(i))] - smoothArray[tempMinLft]) / (abs(minMaxArray.at(i)) - tempMinLft),
					double(smoothArray[abs(minMaxArray.at(i))] - smoothArray[abs(minMaxArray.at(i + 1))]) / (abs(minMaxArray.at(i + 1)) - abs(minMaxArray.at(i))));
				gradArray.push_back(minMaxArray.at(i));
				gradArray.push_back(gradient);
			}
			else if (minMaxArray.at(i - 1) < 0 && minMaxArray.at(i + 1) > 0)//找右边极小值
			{
				for (int m = minMaxArray.at(i) + 1; m < minMaxArray.at(i + 1); m++)
				{
					if ((smoothArray[m] <= smoothArray[m - 1] && smoothArray[m] < smoothArray[m + 1]) ||
						(smoothArray[m] < smoothArray[m - 1] && smoothArray[m] <= smoothArray[m + 1]))
					{
						tempMinRgt = m;
						break;
					}
				}
				gradient = max(double(smoothArray[abs(minMaxArray.at(i))] - smoothArray[abs(minMaxArray.at(i - 1))]) / (abs(minMaxArray.at(i)) - abs(minMaxArray.at(i - 1))),
					double(smoothArray[abs(minMaxArray.at(i))] - smoothArray[tempMinRgt]) / (tempMinRgt - abs(minMaxArray.at(i))));
				gradArray.push_back(minMaxArray.at(i));
				gradArray.push_back(gradient);
			}
			else//找左边、右边极小值
			{
				for (int m = minMaxArray.at(i - 1) + 1; m < minMaxArray.at(i); m++)
				{
					if ((smoothArray[m] <= smoothArray[m - 1] && smoothArray[m] < smoothArray[m + 1]) ||
						(smoothArray[m] < smoothArray[m - 1] && smoothArray[m] <= smoothArray[m + 1]))
					{
						tempMinLft = m;
						break;
					}
				}
				for (int m = minMaxArray.at(i) + 1; m < minMaxArray.at(i + 1); m++)
				{
					if ((smoothArray[m] <= smoothArray[m - 1] && smoothArray[m] < smoothArray[m + 1]) ||
						(smoothArray[m] < smoothArray[m - 1] && smoothArray[m] <= smoothArray[m + 1]))
					{
						tempMinRgt = m;
						break;
					}
				}
				gradient = max(double(smoothArray[abs(minMaxArray.at(i))] - smoothArray[tempMinLft]) / (abs(minMaxArray.at(i)) - tempMinLft),
					double(smoothArray[abs(minMaxArray.at(i))] - smoothArray[tempMinRgt]) / (tempMinRgt - abs(minMaxArray.at(i))));
				gradArray.push_back(minMaxArray.at(i));
				gradArray.push_back(gradient);
			}

			if (minMaxArray.at(i) == medIndex)
				medGradIndex = gradArray.size() - 1;
			if (minMaxArray.at(i) > cumIndex&&cumGradIndex == -1)
				cumGradIndex = gradArray.size() - 1;
		}
	}
	//找出斜率最大的两个极大值点
	int maxGradIndexFst = max(medGradIndex, 1);
	int arrayCurIndex = 0, arrayExIndex = 0;
	int arrayCurSum = 0, arrayExSum = 0;
	for (int i = max(medGradIndex, 1); i < gradArray.size(); i += 2)
	{
		arrayCurIndex = gradArray.at(i - 1);
		arrayExIndex = gradArray.at(maxGradIndexFst - 1);
		if (gradArray.at(i) > gradArray.at(maxGradIndexFst))
		{
			arrayCurSum = smoothArray[arrayCurIndex] + smoothArray[arrayCurIndex - 1] + smoothArray[arrayCurIndex + 1] + smoothArray[arrayCurIndex - 2] + smoothArray[arrayCurIndex + 2];
			arrayExSum = smoothArray[arrayExIndex] + smoothArray[arrayExIndex - 1] + smoothArray[arrayExIndex + 1] + smoothArray[arrayExIndex - 2] + smoothArray[arrayExIndex + 2];
			if (arrayCurSum > arrayExSum)
				maxGradIndexFst = i;
		}
	}
	int secStart = 0, secEnd = 0;
	if (cumGradIndex == -1 && medGradIndex == -1)
	{
		secStart = 1;
		secEnd = gradArray.size();
	}
	else if (cumGradIndex == -1)
	{
		secStart = 1;
		secEnd = medGradIndex;
	}
	else if (medGradIndex == -1)
	{
		secStart = cumGradIndex;
		secEnd = gradArray.size();
	}
	else
	{
		secStart = min(cumGradIndex, medGradIndex);
		secEnd = max(cumGradIndex, medGradIndex);
	}
	int maxGradIndexSec = secStart;
	for (int i = secStart; i < secEnd; i += 2)
	{
		arrayCurIndex = gradArray.at(i - 1);
		arrayExIndex = gradArray.at(maxGradIndexSec - 1);
		if (gradArray.at(i) > gradArray.at(maxGradIndexSec) && gradArray.at(i) < gradArray.at(maxGradIndexFst))
		{
			arrayCurSum = smoothArray[arrayCurIndex] + smoothArray[arrayCurIndex - 1] + smoothArray[arrayCurIndex + 1] + smoothArray[arrayCurIndex - 2] + smoothArray[arrayCurIndex + 2];
			arrayExSum = smoothArray[arrayExIndex] + smoothArray[arrayExIndex - 1] + smoothArray[arrayExIndex + 1] + smoothArray[arrayExIndex - 2] + smoothArray[arrayExIndex + 2];
			if (arrayCurSum > arrayExSum)
				maxGradIndexSec = i;
		}
	}

	int maxIndexFst = gradArray.at(maxGradIndexFst - 1);
	int maxIndexSec = gradArray.at(maxGradIndexSec - 1);
	//找出[maxIndexSec,maxIndexFst]范围内的极小值
	int minIndex = 0;
	float minValue = 1000000;
	for (int i = maxIndexSec; i < maxIndexFst; i++)
	{
		if (smoothArray[i] < minValue)
		{
			minValue = smoothArray[i];
			minIndex = i;
		}
	}

	upMaxVal = maxIndexFst;
	downMaxVal = maxIndexSec;
	minVal = minIndex;

	return 0;
}

vector<Mat> OCRIDCard::seperateAddrStr(Mat srcAddrImg)
{
	//vector<Mat> tmpImgList;
	//vector<Point> pointsArray;

	//vector<vector<Mat>> tmpRowImg;
	//vector<vector<Point>> tmpRowPoint;

	//vector<vector<Mat>> finalImgList;
	//Mat cutAddrImage;

	//IplImage imgTmp = srcAddrImg;
	//IplImage *IplAddrImg = cvCloneImage(&imgTmp);//Mat->IplImage
	//IplImage *IplAddrImgClone = cvCloneImage(IplAddrImg);
	//IplImage *IplAddrImgTemp = cvCloneImage(IplAddrImg);

	//int kernelSize;
	//kernelSize = round(srcAddrImg.size().height / 8.0*(srcAddrImg.size().width / srcAddrImg.size().height) / 5);//13.0

	//int *value = new int[kernelSize*kernelSize];
	//for (int i = 0; i < kernelSize*kernelSize; i++)
	//{
	//	value[i] = 1;
	//}
	//IplConvKernel* element = cvCreateStructuringElementEx(kernelSize, kernelSize, (kernelSize - 1) / 2, (kernelSize - 1) / 2, CV_SHAPE_RECT, value);
	//cvMorphologyEx(IplAddrImgClone, IplAddrImgClone, IplAddrImgTemp, element, CV_MOP_CLOSE, 1);
	//cvReleaseStructuringElement(&element);

	//CvSeq* contours = NULL;
	//CvMemStorage* storage = cvCreateMemStorage(0);
	//int count = cvFindContours(IplAddrImgClone, storage, &contours, sizeof(CvContour), CV_RETR_CCOMP);

	//int heightLimit = round(min(srcAddrImg.size().height, srcAddrImg.size().width) / 10.0);
	//int widthLimit = round(heightLimit / 5.0);
	//int expLength = 3;//3

	////筛选符合大小的框
	//vector<CvRect> sepRect;
	//for (CvSeq* c = contours; c != NULL; c = c->h_next)
	//{
	//	CvRect rc = cvBoundingRect(c, 0);
	//	//以下为分割后图片框选&输出       
	//	IplImage* imgNo = cvCreateImage(cvSize(rc.width, rc.height), IPL_DEPTH_8U, 3);
	//	IplImage* imageresize = 0;
	//	if (rc.width > widthLimit && rc.height > heightLimit)
	//	{
	//		cvDrawRect(IplAddrImg, cvPoint(rc.x, rc.y), cvPoint(rc.x + rc.width, rc.y + rc.height), CV_RGB(128, 128, 128),2);

	//		sepRect.push_back(rc);
	//	}

	//	cvReleaseImage(&imgNo);
	//	cvReleaseImage(&imageresize);
	//}

	////排序
	//int flag = 0;
	//vector<vector<CvRect>> lineRect;
	//vector<CvRect> tmpLineRect;
	//tmpLineRect.push_back(sepRect.at(0));
	//lineRect.push_back(tmpLineRect);
	//for (int i = 1; i < sepRect.size(); i++)
	//{
	//	flag = 0;
	//	for (int j = 0; j < lineRect.size(); j++)
	//	{
	//		for (int k = 0; k < lineRect.at(j).size(); k++)
	//		{
	//			if (abs(sepRect.at(i).y - lineRect.at(j).at(k).y) < sepRect.at(i).height / 2.0)
	//			{
	//				lineRect.at(j).push_back(sepRect.at(i));
	//				flag = 1;
	//				break;
	//			}
	//		}
	//		if (flag == 1)
	//			break;
	//	}
	//	if (flag == 0)
	//	{
	//		tmpLineRect.swap((vector<CvRect>()));
	//		tmpLineRect.push_back(sepRect.at(i));
	//		lineRect.push_back(tmpLineRect);
	//	}
	//}

	//for (int i = 0; i < lineRect.size(); i++)
	//{
	//	sort(lineRect.at(i).begin(), lineRect.at(i).end(), compX);
	//}
	//sort(lineRect.begin(), lineRect.end(), compVecY);

	vector<Mat> fineLineMat;
	/*for (int i = 0; i < lineRect.size(); i++)
	{
		for (int j = 0; j < lineRect.at(i).size(); j++)
		{
			int cutX = max(0, lineRect.at(i).at(j).x - 2);
			int cutY = max(0, lineRect.at(i).at(j).y - 2);
			int cutWidth, cutHeight;

			int xStart = -1, xCount = 0;
			int Xflag = 0;
			for (int m = lineRect.at(i).at(j).x + lineRect.at(i).at(j).width; m < min(srcAddrImg.size().width - 1, lineRect.at(i).at(j).x + lineRect.at(i).at(j).width + lineRect.at(i).at(j).height); m++)
			{
				Xflag = 0;
				for (int n = lineRect.at(i).at(j).y; n < lineRect.at(i).at(j).y + lineRect.at(i).at(j).height; n++)
				{
					if (srcAddrImg.at<uchar>(n, m) == 255)
					{
						Xflag = 1;
						break;
					}
				}

				if (Xflag == 0)
				{
					if (xStart == -1)
					{
						xStart = m;
					}
					xCount++;
				}

				if (xStart != -1 && Xflag == 1)
					break;
			}
			if (xStart == -1)
				cutWidth = lineRect.at(i).at(j).width;
			else
				cutWidth = min(srcAddrImg.size().width - 1 - cutX, xStart + int(round(xCount / 2.0)) - cutX);

			int yStart = -1, yCount = 0;
			int Yflag = 0;
			for (int m = lineRect.at(i).at(j).y + lineRect.at(i).at(j).height; m < min(srcAddrImg.size().height - 1, lineRect.at(i).at(j).y + lineRect.at(i).at(j).height * 2); m++)
			{
				Yflag = 0;
				for (int n = lineRect.at(i).at(j).x; n < lineRect.at(i).at(j).x + lineRect.at(i).at(j).width; n++)
				{
					if (srcAddrImg.at<uchar>(m, n) == 255)
					{
						Yflag = 1;
						break;
					}
				}

				if (Yflag == 0)
				{
					if (yStart == -1)
					{
						yStart = m;
					}
					yCount++;
				}

				if (yStart != -1 && Yflag == 1)
					break;
			}
			if (yStart == -1)
				cutHeight = lineRect.at(i).at(j).height;
			else
				cutHeight = min(srcAddrImg.size().height - 1 - cutY, yStart + int(round(yCount / 2.0)) - cutY);

			Mat tmp = srcAddrImg(Rect(cutX, cutY, cutWidth, cutHeight)).clone();
			fineLineMat.push_back(srcAddrImg(Rect(cutX, cutY, cutWidth, cutHeight)));
		}
	}

	cvReleaseMemStorage(&storage);
	cvReleaseImage(&IplAddrImg);
	cvReleaseImage(&IplAddrImgClone);
	cvReleaseImage(&IplAddrImgTemp);
	
	delete value;*/

	fineLineMat.push_back(srcAddrImg);
	return fineLineMat;
}

string OCRIDCard::recognizeAddr(vector<Mat> addrImgList)
{
	string addrString;
	Mat recognImage;

	PIX *img;
	char *text;

	for (int i = 0; i < addrImgList.size(); i++)
	{
		recognImage = addrImgList.at(i);
		IplImage *IplRecognImg = &(IplImage)(recognImage);
		/*cv::imwrite(".\\temp.png", recognImage);
		img = pixRead(".\\temp.png");*/

		//中文
		chinese_handle->SetImage((unsigned char*)(IplRecognImg->imageData), IplRecognImg->width, IplRecognImg->height, IplRecognImg->nChannels, IplRecognImg->widthStep);//TessBaseAPISetImage2(chinese_handle, img);
		chinese_handle->SetSourceResolution(300);
		text = chinese_handle->GetUTF8Text();

		//字符转码
		CString csText;
		csText = UTF82WCS(text);

		USES_CONVERSION;
		string str(W2A(csText));

		addrString.append(str.substr(0, str.length() - 2));

		chinese_handle->Clear();
	}

	//将O修正为0
	for (int i = 0; i < addrString.length(); i++)
	{
		string tmpStr = addrString.substr(i, 1);
		if (addrString.substr(i, 1) == "O" || addrString.substr(i, 1) == "o")
		{
			addrString.replace(i, 1, "0");
		}
	}

	return addrString;
}

vector<Mat> OCRIDCard::seperateBirthStr(Mat srcBirthImg)
{
	vector<Mat> sepBirthImg;
	Mat yearImg, monthImg, dayImg;
	int anchor1st = -1, xCount = 0;
	int flag = 0;
	int yearStart, yearLength, monthStart = -1, monthLength = -1;
	int diff = srcBirthImg.size().height / 2.0;
	int extendSize = max(2, int(round(srcBirthImg.size().height / 15.0)));
	int numHeight = 0, startHeight = -1, endHeight = -1;
	int flagHeight = 0;

	for (int i = 0; i < srcBirthImg.size().width; i++)
	{
		flag = 0;
		for (int j = 0; j < srcBirthImg.size().height; j++)
		{
			if (srcBirthImg.at<uchar>(j, i) == 255)
			{
				flag = 1;
				break;
			}
		}

		if (flag == 0)
		{
			xCount++;
		}

		if (flag == 1)
		{
			if (anchor1st != -1)
				xCount = 0;
			if (anchor1st == -1)
				anchor1st = i;
		}

		if (flag == 0 && anchor1st != -1)
		{
			//年
			if ((i - anchor1st) > srcBirthImg.size().width / 4.0)
			{
				yearStart = anchor1st;
				yearLength = i - xCount - anchor1st;
				int cutX = max(0, anchor1st - extendSize);
				int cutY = 0;
				int cutWidth = min(srcBirthImg.size().width - 1 - cutX, i - xCount - cutX + extendSize * 2);
				int cutHeight = srcBirthImg.size().height;

				yearImg = srcBirthImg(Rect(cutX, cutY, cutWidth, cutHeight));
				sepBirthImg.push_back(yearImg);

				anchor1st = -1;
				xCount = 0;
				flag = 0;

				break;
			}	
		}
	}
	for (int i = 0; i < yearImg.size().height; i++)
	{
		flagHeight = 0;
		for (int j = 0; j < yearImg.size().width; j++)
		{
			if (yearImg.at<uchar>(i, j) == 255&& startHeight == -1)
			{
				startHeight = i;
				flagHeight = 1;
				break;
			}

			if (yearImg.at<uchar>(i, j) == 255 && startHeight != -1)
			{
				flagHeight = 1;
				break;
			}
		}

		if (flagHeight == 0&& startHeight != -1)
		{
			endHeight = i;
			break;
		}
	}
	numHeight = endHeight - startHeight;

	//月
	int lastFlag = 0, curFlag = 0;
	int cutXM, cutYM, cutWidthM, cutHeightM;

	for (int j = 0; j < srcBirthImg.size().height; j++)
	{
		if (srcBirthImg.at<uchar>(j, yearStart + yearLength*100.0 / 60.0) == 255)
		{
			lastFlag = 1;
			break;
		}
	}
	for (int i = yearStart + yearLength*100.0 / 60.0 + 1; i < srcBirthImg.size().width; i++)
	{
		curFlag = 0;
		for (int j = 0; j < srcBirthImg.size().height; j++)
		{
			if (srcBirthImg.at<uchar>(j, i) == 255)
			{
				curFlag = 1;
				break;
			}
		}

		if (lastFlag == 0 && curFlag == 1 && anchor1st == -1)
		{
			anchor1st = i;
		}
		if (lastFlag == 1 && curFlag == 0 && anchor1st != -1)
		{
			if (monthStart != -1 && monthLength != -1 && (i - monthStart) / float(monthLength)>(numHeight / float(monthLength)*1.4))
				break;

			if ((i - anchor1st) > yearLength / 10.0 && (i - anchor1st) < yearLength / 2.0&&
				monthStart == -1&& monthLength == -1)
			{
				monthStart = anchor1st;
				monthLength = i - anchor1st;
				cutXM = anchor1st - extendSize;
				cutYM = 0;
				cutWidthM = min(srcBirthImg.size().width - 1 - cutXM, i - anchor1st + extendSize * 2);
				cutHeightM = srcBirthImg.size().height;

				anchor1st = -1;

			}
			if ((i - anchor1st) > yearLength / 10.0 && (i - anchor1st) < yearLength / 2.0&&
				monthStart != -1 && monthLength != -1)
			{
				if ((i - monthStart) / float(monthLength) > (numHeight / float(monthLength)))
				{
					cutXM = monthStart - extendSize;
					cutYM = 0;
					cutWidthM = min(srcBirthImg.size().width - 1 - cutXM, i - monthStart + extendSize * 2);
					cutHeightM = srcBirthImg.size().height;

					break;
				}
			}
			if((i - anchor1st) < yearLength / 10.0)
			{
				lastFlag = 0;
				curFlag = 0;
				anchor1st = -1;
			}
		}
		lastFlag = curFlag;
	}
	monthImg = srcBirthImg(Rect(cutXM, cutYM, cutWidthM, cutHeightM));
	sepBirthImg.push_back(monthImg);

	//日
	anchor1st = -1;
	lastFlag = curFlag = 0;
	int cutXD, cutYD, cutWidthD, cutHeightD;
	int dayStart = -1, dayLength = -1;
	for (int j = 0; j < srcBirthImg.size().height; j++)
	{
		if (srcBirthImg.at<uchar>(j, yearStart + yearLength*110.0/36.0) == 255)
		{
			lastFlag = 1;
			break;
		}
	}
	for (int i = yearStart + yearLength*110.0 / 36.0 + 1; i < srcBirthImg.size().width; i++)
	{
		curFlag = 0;
		for (int j = 0; j < srcBirthImg.size().height; j++)
		{
			if (srcBirthImg.at<uchar>(j, i) == 255)
			{
				curFlag = 1;
				break;
			}
		}

		if (lastFlag == 0 && curFlag == 1 && anchor1st == -1)
		{
			anchor1st = i;
		}
		if (lastFlag == 1 && curFlag == 0 && anchor1st != -1)
		{
			if (dayStart != -1 && dayLength != -1 && (i - dayStart) / float(dayLength)>(numHeight / float(dayLength)*1.6))
				break;

			if ((i - anchor1st) > yearLength / 10.0 && (i - anchor1st) < yearLength / 2.0&&
				dayStart == -1 && dayLength == -1)
			{
				dayStart = anchor1st;
				dayLength = i - anchor1st;
				cutXD = anchor1st - extendSize;
				cutYD = 0;
				cutWidthD = min(srcBirthImg.size().width - 1 - cutXD, i - anchor1st + extendSize * 2);
				cutHeightD = srcBirthImg.size().height;

				anchor1st = -1;

			}
			if ((i - anchor1st) > yearLength / 10.0 && (i - anchor1st) < yearLength / 2.0&&
				dayStart != -1 && dayLength != -1)
			{
				if ((i - dayStart) / float(dayLength) > (numHeight / float(dayLength)))
				{
					cutXD = dayStart - extendSize;
					cutYD = 0;
					cutWidthD = min(srcBirthImg.size().width - 1 - cutXD, i - dayStart + extendSize * 2);
					cutHeightD = srcBirthImg.size().height;

					break;
				}
			}
			if ((i - anchor1st) < yearLength / 10.0)
			{
				lastFlag = 0;
				curFlag = 0;
				anchor1st = -1;
			}
		}
		lastFlag = curFlag;
	}
	dayImg = srcBirthImg(Rect(cutXD, cutYD, cutWidthD, cutHeightD));
	sepBirthImg.push_back(dayImg);

	return sepBirthImg;
}

string OCRIDCard::recognizeBirth(vector<Mat> birthImgList)
{
	string birthString;
	Mat recognImage;

	PIX *img;
	char *text;

	for (int i = 0; i < birthImgList.size(); i++)
	{
		recognImage = birthImgList.at(i);
		IplImage *IplRecognImg = &(IplImage)(recognImage);

		eng_handle->SetImage((unsigned char*)(IplRecognImg->imageData), IplRecognImg->width, IplRecognImg->height, IplRecognImg->nChannels, IplRecognImg->widthStep);
		text = eng_handle->GetUTF8Text();

		//字符转码
		CString csText;
		csText = UTF82WCS(text);

		USES_CONVERSION;
		string str(W2A(csText));

		if (i < birthImgList.size() - 1)
			birthString.append(str.substr(0, str.length() - 2) + '-');
		else
			birthString.append(str.substr(0, str.length() - 2));

		eng_handle->Clear();
	}

	return birthString;
}

vector<Mat> OCRIDCard::preProcess(vector<Mat> imageList)
{
	vector<Mat> dstImageList;
	Mat dstImage;

	for (int i = 0; i < imageList.size(); i++)
	{
		dstImage = thresholdProcess(imageList.at(i));
		dstImageList.push_back(dstImage);
	}

	return dstImageList;
}

bool compX(const CvRect &a, const CvRect &b)
{
	return a.x < b.x;
}

bool compY(const CvRect &a, const CvRect &b)
{
	return a.y < b.y;
}

bool compVecY(const vector<CvRect> &a, const vector<CvRect> &b)
{
	return a.at(0).y < b.at(0).y;
}

int OCRIDCard::recognizeIDCard(Mat srcImage, vector<string>& resultStr)
{
	Mat grayImg, dstNoise, histEqualImg, thresImg;

	//转换成灰度图
	cvtColor(srcImage, grayImg, CV_BGR2GRAY);

	bilateralFilter(grayImg, dstNoise, 5, 17, 13);//双边滤波去噪 5, 20, 20
	adaptiveHistEqual(dstNoise, histEqualImg, 0.08);//直方图均衡
	
	Mat sampleImg;
	int downMaxVal, upMaxVal, minVal;
	int cutX, cutY, cutWidth, cutHeight;
	int thresVal;
	//选取身份证地址区域作为取样区域
	cutX = int(round(grayImg.cols * 150 / 887));
	cutY = int(round(grayImg.rows * 275 / 568));
	cutWidth = int(round(grayImg.cols * 385 / 887));
	cutHeight = int(round(grayImg.rows * 165 / 568));
	sampleImg = histEqualImg(Rect(cutX, cutY, cutWidth, cutHeight));
	//获取取样区域的阈值
	getThresholdValue(sampleImg, downMaxVal, upMaxVal, minVal);
	//确定阈值
	thresVal = int(round(upMaxVal*0.3 + downMaxVal*0.7));
	thresImg = histEqualImg.clone();
	//二值化图像
	for (int i = 0; i < histEqualImg.rows; i++)
	{
		for (int j = 0; j < histEqualImg.cols; j++)
		{
			if (histEqualImg.at<uchar>(i, j) < thresVal)
				thresImg.at<uchar>(i, j) = 255;
			else
				thresImg.at<uchar>(i, j) = 0;
		}
	}

	//获取候选文字区域
	IplImage imgTmp = thresImg;
	IplImage *IplAddrImg = cvCloneImage(&imgTmp);//Mat->IplImage
	IplImage *IplAddrImgClone = cvCloneImage(IplAddrImg);
	IplImage *IplAddrImgTemp = cvCloneImage(IplAddrImg);

	int kernelSize;
	kernelSize = round(min(thresImg.size().height, thresImg.size().width) / 25.0);//13.0

	int *value = new int[kernelSize*kernelSize];
	for (int i = 0; i < kernelSize*kernelSize; i++)
	{
		value[i] = 1;
	}
	//形态学处理
	IplConvKernel* element = cvCreateStructuringElementEx(kernelSize, kernelSize, (kernelSize - 1) / 2, (kernelSize - 1) / 2, CV_SHAPE_RECT, value);
	cvMorphologyEx(IplAddrImgClone, IplAddrImgClone, IplAddrImgTemp, element, CV_MOP_CLOSE, 1);
	//寻找连通区域
	CvSeq* contours = NULL;
	CvMemStorage* storage = cvCreateMemStorage(0);
	int count = cvFindContours(IplAddrImgClone, storage, &contours, sizeof(CvContour), CV_RETR_CCOMP);

	int heightUp = round(thresImg.size().height / 3.0);
	int heightDown = round(thresImg.size().height / 40.0);
	int widthUp = round(thresImg.size().width / 1.333);
	int widthDown = round(thresImg.size().width / 40);

	//提取候选文字区域
	vector<CvRect> candidateRects;//候选框
	for (CvSeq* c = contours; c != NULL; c = c->h_next)
	{
		CvRect rc = cvBoundingRect(c, 0);     
		IplImage* imgNo = cvCreateImage(cvSize(rc.width, rc.height), IPL_DEPTH_8U, 3);
		IplImage* imageresize = 0;
		if (rc.width > widthDown && rc.width < widthUp &&
			rc.height > heightDown && rc.height < heightUp&&
			rc.x < thresImg.size().width*2.0 / 3.0)
		{
			//cvDrawRect(IplAddrImg, cvPoint(rc.x, rc.y), cvPoint(rc.x + rc.width, rc.y + rc.height), CV_RGB(128, 128, 128), 2);
			candidateRects.push_back(rc);
		}
	}

	//确定参考点的x坐标
	sort(candidateRects.begin(), candidateRects.end(), compX);

	int diff = int(round(thresImg.size().width / 60.0));
	int vecFlag = -1;
	vector<vector<CvRect>> xPoints;
	vector<CvRect> tmpVec;
	for (int i = 1; i < candidateRects.size(); i++)
	{
		if (abs(candidateRects.at(i).x - candidateRects.at(i - 1).x) <= diff)
		{
			if (vecFlag == 1)
			{
				tmpVec.push_back(candidateRects.at(i));
			}
			else if (vecFlag == -1)
			{
				tmpVec.push_back(candidateRects.at(i - 1));
				tmpVec.push_back(candidateRects.at(i));
				vecFlag = 1;
			}
		}
		else
		{
			if (vecFlag == 1)
			{
				if (tmpVec.size() > 2)
					xPoints.push_back(tmpVec);
				vecFlag = -1;
				tmpVec.swap((vector<CvRect>()));
			}
		}
	}

	if (xPoints.size() == 0)
	{
		MessageBox(NULL, _T("照片光照不均匀或照片位置不正，请重新选择照片！"), _T(""), MB_OK);
		return -1;
	}

	int mostPointIndex = 0;
	int mostPointNum = xPoints.at(0).size();
	for (int i = 1; i < xPoints.size(); i++)
	{
		if (i >= xPoints.size())
			break;

		if (xPoints.at(i).size()>mostPointNum)
			mostPointIndex = i;
	}

	sort(xPoints.at(mostPointIndex).begin(), xPoints.at(mostPointIndex).end(), compY);
	int minX = xPoints.at(mostPointIndex).at(0).x;
	int refX, refY, refHeight;
	CvRect refRect;
	refRect = xPoints.at(mostPointIndex).at(0);

	for (int i = 1; i < xPoints.at(mostPointIndex).size(); i++)
	{
		if (xPoints.at(mostPointIndex).at(i).x < minX)
			minX = xPoints.at(mostPointIndex).at(i).x;
	}
	refX = minX;
	refY = xPoints.at(mostPointIndex).at(0).y;

	int diffY = int(round(thresImg.size().height / 60.0));
	for (int i = 0; i < candidateRects.size(); i++)
	{
		if (xPoints.at(mostPointIndex).at(0).x == candidateRects.at(i).x&&xPoints.at(mostPointIndex).at(0).y == candidateRects.at(i).y)
			continue;

		if (abs(candidateRects.at(i).y - refY) < diffY&&
			abs(candidateRects.at(i).x - refX) / float(xPoints.at(mostPointIndex).at(0).height) < 4 &&
			abs(candidateRects.at(i).height - xPoints.at(mostPointIndex).at(0).height) < diffY)
		{
			refRect.x = refX;
			refRect.y = refY;
			refRect.height = max(candidateRects.at(i).height, xPoints.at(mostPointIndex).at(0).height);
			refRect.width = candidateRects.at(i).x + candidateRects.at(i).width - refX;
		}
	}

	//根据参考点裁剪身份证各文字区域
	vector<Mat> imageList;
	imageList = cutIDCard(histEqualImg, refRect, refX, refY, candidateRects);

	//对裁剪后的图片进行预处理
	vector<Mat> dstImageList;
	dstImageList = preProcess(imageList);

	//OCR识别
	Mat recognImage;
	PIX *img;
	char *text = new char;

	for (int i = 0; i < dstImageList.size(); i++)
	{
		recognImage = dstImageList.at(i);
		IplImage *IplRecognImg = &(IplImage)(recognImage);

		//对住址字符进行分割识别
		if (i == 3)
		{
			vector<Mat> addrImgList;
			string addrString;
			addrImgList = seperateAddrStr(dstImageList.at(3));
			addrString = recognizeAddr(addrImgList);
			resultStr.push_back(addrString);
			continue;
		}
		//API设置成英文状态，对数字进行识别
		else if (i == 4)
		{
			eng_handle->SetImage((unsigned char*)(IplRecognImg->imageData), IplRecognImg->width, IplRecognImg->height, IplRecognImg->nChannels, IplRecognImg->widthStep);
			eng_handle->SetVariable("tessedit_char_whitelist", "0123456789");
			eng_handle->SetPageSegMode(tesseract::PSM_SINGLE_LINE);
			eng_handle->SetVariable("load_system_dawg", "F");
			eng_handle->SetVariable("load_freq_dawg", "F");
			eng_handle->SetVariable("load_number_dawg", "T");
			eng_handle->SetVariable("classify_enable_learning", "F");
			eng_handle->SetVariable("classify_enable_adaptive_matcher", "F");
			text = eng_handle->GetUTF8Text();
		}
		//对出生日期进行分割识别
		else if (i == 5)
		{
			vector<Mat> birthImgList;
			string birthString;
			birthImgList = seperateBirthStr(dstImageList.at(5));
			birthString = recognizeBirth(birthImgList);
			resultStr.push_back(birthString);
			continue;
		}
		else
		{
			chinese_handle->SetImage((unsigned char*)(IplRecognImg->imageData), IplRecognImg->width, IplRecognImg->height, IplRecognImg->nChannels, IplRecognImg->widthStep);
			chinese_handle->SetSourceResolution(300);
			text = chinese_handle->GetUTF8Text();
		}
		
		//字符转码
		CString csText;
		csText = UTF82WCS(text);

		USES_CONVERSION;
		string str(W2A(csText));


		resultStr.push_back(str);
	}

	//清理resultStr
	for (int i = 0; i < resultStr.size(); i++)
	{
		string::iterator it;
		for (it = resultStr.at(i).begin(); it != resultStr.at(i).end(); )
		{
			if (*it == '\n')//清理回车字符
			{
				resultStr.at(i).erase(it);
				--it;
			}
			else if (*it == ' ')//清理空白字符
			{
				resultStr.at(i).erase(it);
			}
			else
			{
				++it;
			}
		}
	}

	//修正出生日期和身份证号码
	if (resultStr.at(4).length() >= 17)
	{
		//修正身份证号码
		string idNum = resultStr.at(4);
		int num;
		int identityNum = int(atof(idNum.substr(0, 1).c_str()) * 7 + atof(idNum.substr(1, 1).c_str()) * 9 + atof(idNum.substr(2, 1).c_str()) * 10 +
			atof(idNum.substr(3, 1).c_str()) * 5 + atof(idNum.substr(4, 1).c_str()) * 8 + atof(idNum.substr(5, 1).c_str()) * 4 +
			atof(idNum.substr(6, 1).c_str()) * 2 + atof(idNum.substr(7, 1).c_str()) * 1 + atof(idNum.substr(8, 1).c_str()) * 6 +
			atof(idNum.substr(9, 1).c_str()) * 3 + atof(idNum.substr(10, 1).c_str()) * 7 + atof(idNum.substr(11, 1).c_str()) * 9 +
			atof(idNum.substr(12, 1).c_str()) * 10 + atof(idNum.substr(13, 1).c_str()) * 5 + atof(idNum.substr(14, 1).c_str()) * 8 +
			atof(idNum.substr(15, 1).c_str()) * 4 + atof(idNum.substr(16, 1).c_str()) * 2) % 11;
		resultStr.at(4) = resultStr.at(4).substr(0, 17);
		if (identityNum == 0)
			resultStr.at(4).append("1");
		else if(identityNum == 1)
			resultStr.at(4).append("0");
		else if (identityNum == 2)
			resultStr.at(4).append("X");
		else if (identityNum == 3)
			resultStr.at(4).append("9");
		else if (identityNum == 4)
			resultStr.at(4).append("8");
		else if (identityNum == 5)
			resultStr.at(4).append("7");
		else if (identityNum == 6)
			resultStr.at(4).append("6");
		else if (identityNum == 7)
			resultStr.at(4).append("5");
		else if (identityNum == 8)
			resultStr.at(4).append("4");
		else if (identityNum == 9)
			resultStr.at(4).append("3");
		else if (identityNum == 10)
			resultStr.at(4).append("2");

		string yearNum, monthNum, dayNum;
		int genderNum;

		yearNum = resultStr.at(4).substr(6, 4).c_str();
		monthNum = resultStr.at(4).substr(10, 2).c_str();
		dayNum = resultStr.at(4).substr(12, 2).c_str();

		genderNum = atoi(resultStr.at(4).substr(16, 1).c_str());

		resultStr.pop_back();
		resultStr.push_back(yearNum);
		resultStr.push_back(monthNum);
		resultStr.push_back(dayNum);

		if (genderNum % 2 == 1)
			resultStr.at(1) = "男";
		else
			resultStr.at(1) = "女";
	}
	else
	{
		string predYearNum, predMonthNum, predDayNum;
		predYearNum = resultStr.at(5).substr(0, resultStr.at(5).find_first_of('-')).c_str();
		predMonthNum = resultStr.at(5).substr(resultStr.at(5).find_first_of('-') + 1, resultStr.at(5).find_last_of('-') - resultStr.at(5).find_first_of('-') - 1).c_str();
		predDayNum = resultStr.at(5).substr(resultStr.at(5).find_last_of('-') + 1, resultStr.at(5).size() - resultStr.at(5).find_last_of('-') - 1).c_str();

		resultStr.pop_back();
		resultStr.push_back(predYearNum);
		resultStr.push_back(predMonthNum);
		resultStr.push_back(predDayNum);
	}

	cvReleaseImage(&IplAddrImg);
	cvReleaseImage(&IplAddrImgClone);
	cvReleaseImage(&IplAddrImgTemp);
	cvReleaseStructuringElement(&element);
	cvReleaseMemStorage(&storage);
	eng_handle->Clear();
	eng_handle->End();
	chinese_handle->Clear();
	chinese_handle->End();

	return 0;
}

Mat OCRIDCard::adaptiveMediamBlur(Mat srcImage, int kernelRadius, float mediamRate)
{
	Mat medBlurImage(srcImage.size(), CV_8U, Scalar(0));
	if (kernelRadius == 0)
		return srcImage;
	if (srcImage.channels() != 1)
		cvtColor(srcImage, srcImage, CV_BGR2GRAY);

	int median = int(round(pow(2 * kernelRadius + 1, 2)*mediamRate));
	int count = 0;
	for (int i = kernelRadius; i < srcImage.rows - kernelRadius; i++)
	{
		for (int j = kernelRadius; j < srcImage.cols - kernelRadius; j++)
		{
			count = 0;
			for (int m = -kernelRadius; m <= kernelRadius; m++)
			{
				for (int n = -kernelRadius; n <= kernelRadius; n++)
				{
					if (srcImage.at<uchar>(i + m, j + n) == 0)
						count++;
				}
			}
			if (count >= median)
				medBlurImage.at<uchar>(i, j) = 0;
			else
				medBlurImage.at<uchar>(i, j) = 255;
		}
	}

	return medBlurImage;
}
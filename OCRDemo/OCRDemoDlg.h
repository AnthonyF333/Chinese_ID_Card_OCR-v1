
// OCRDemoDlg.h : 头文件
//

#pragma once
//#include <cv.h>
//#include <highgui.h>
#include <opencv2/opencv.hpp>;
#include "OCRIDCard.h";

using namespace std;
using namespace cv;

// COCRDemoDlg 对话框
class COCRDemoDlg : public CDialogEx
{
// 构造
public:
	COCRDemoDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_OCRDEMO_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	OCRIDCard ocr_;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnStnClickedTitleBirth2();
	afx_msg void OnBnClickedBtnOpen();

	Mat IDImage;
	afx_msg void OnBnClickedBtnRecognize();
	afx_msg void OnStnClickedTitleDay();
};

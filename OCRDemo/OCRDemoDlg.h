
// OCRDemoDlg.h : ͷ�ļ�
//

#pragma once
//#include <cv.h>
//#include <highgui.h>
#include <opencv2/opencv.hpp>;
#include "OCRIDCard.h";

using namespace std;
using namespace cv;

// COCRDemoDlg �Ի���
class COCRDemoDlg : public CDialogEx
{
// ����
public:
	COCRDemoDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_OCRDEMO_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	OCRIDCard ocr_;

	// ���ɵ���Ϣӳ�亯��
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

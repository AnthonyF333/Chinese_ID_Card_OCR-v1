
// OCRDemoDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "OCRDemo.h"
#include "OCRDemoDlg.h"
#include "afxdialogex.h"
#include <time.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// COCRDemoDlg �Ի���



COCRDemoDlg::COCRDemoDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_OCRDEMO_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void COCRDemoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(COCRDemoDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_OPEN, &COCRDemoDlg::OnBnClickedBtnOpen)
	ON_BN_CLICKED(IDC_BTN_RECOGNIZE, &COCRDemoDlg::OnBnClickedBtnRecognize)
	ON_STN_CLICKED(IDC_TITLE_DAY, &COCRDemoDlg::OnStnClickedTitleDay)
END_MESSAGE_MAP()


// COCRDemoDlg ��Ϣ�������

BOOL COCRDemoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������
	namedWindow("view");
	HWND hWnd = (HWND)cvGetWindowHandle("view");
	HWND hParent = ::GetParent(hWnd);
	::SetParent(hWnd, GetDlgItem(IDC_PICTURE)->m_hWnd);
	::ShowWindow(hParent, SW_HIDE);

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void COCRDemoDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR COCRDemoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void COCRDemoDlg::OnEnChangeEdit1()
{
	// TODO:  ����ÿؼ��� RICHEDIT �ؼ���������
	// ���ʹ�֪ͨ��������д CDialogEx::OnInitDialog()
	// ���������� CRichEditCtrl().SetEventMask()��
	// ͬʱ�� ENM_CHANGE ��־�������㵽�����С�

	// TODO:  �ڴ���ӿؼ�֪ͨ����������
}


void COCRDemoDlg::OnStnClickedTitleBirth2()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
}


void COCRDemoDlg::OnBnClickedBtnOpen()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CString picPath;   //����ͼƬ·������  
	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_ALLOWMULTISELECT,
		NULL, this);   //ѡ���ļ��Ի���  
	
	if (dlg.DoModal() != IDOK)
	{
		return;
	}
	picPath = dlg.GetPathName();  //��ȡͼƬ·��  

	//CString to string  ʹ����������ǵ��ַ���ѡ�á�ʹ�ö��ֽ��ַ�������Ȼ�ᱨ��
	USES_CONVERSION;
	string picpath(W2A(picPath));

	IDImage = imread(picpath);
	Mat imagedst;
	//���²�����ȡͼ�οؼ��ߴ粢�Դ˸ı�ͼƬ�ߴ�  
	CRect rect;
	GetDlgItem(IDC_PICTURE)->GetClientRect(&rect);
	Rect dst(rect.left, rect.top, rect.right, rect.bottom);
	resize(IDImage, imagedst, cv::Size(rect.Width(), rect.Height()));
	imshow("view", imagedst);

	GetDlgItem(IDC_RESULT_NAME)->SetWindowTextW(_T(""));
	GetDlgItem(IDC_RESULT_GENDER)->SetWindowTextW(_T(""));
	GetDlgItem(IDC_RESULT_NATION)->SetWindowTextW(_T(""));
	GetDlgItem(IDC_RESULT_YEAR)->SetWindowTextW(_T(""));
	GetDlgItem(IDC_RESULT_MONTH)->SetWindowTextW(_T(""));
	GetDlgItem(IDC_RESULT_DAY)->SetWindowTextW(_T(""));
	GetDlgItem(IDC_RESULT_ADDRESS)->SetWindowTextW(_T(""));
	GetDlgItem(IDC_RESULT_IDNUMBER)->SetWindowTextW(_T(""));
}


void COCRDemoDlg::OnBnClickedBtnRecognize()
{
	clock_t start, finish;
	double totaltime;
	start = clock();

	// TODO: �ڴ���ӿؼ�֪ͨ����������
	vector<string> resultStr;
	OCRIDCard op;
	int ret = op.init();
	ret = op.recognizeIDCard(IDImage, resultStr);
	if (ret != 0)
		return;

	CEdit* m_edit;
	CString csTemp;

	//����0
	m_edit = (CEdit*)GetDlgItem(IDC_RESULT_NAME);
	csTemp = resultStr.at(0).c_str();
	m_edit->SetWindowTextW(csTemp);

	//�Ա�1
	m_edit = (CEdit*)GetDlgItem(IDC_RESULT_GENDER);
	csTemp = resultStr.at(1).c_str();
	m_edit->SetWindowTextW(csTemp);

	//����2
	m_edit = (CEdit*)GetDlgItem(IDC_RESULT_NATION);
	csTemp = resultStr.at(2).c_str();
	m_edit->SetWindowTextW(csTemp);

	//���������� 567
	m_edit = (CEdit*)GetDlgItem(IDC_RESULT_YEAR);
	csTemp = resultStr.at(5).c_str();
	m_edit->SetWindowTextW(csTemp);

	m_edit = (CEdit*)GetDlgItem(IDC_RESULT_MONTH);
	csTemp = resultStr.at(6).c_str();
	m_edit->SetWindowTextW(csTemp);

	m_edit = (CEdit*)GetDlgItem(IDC_RESULT_DAY);
	csTemp = resultStr.at(7).c_str();
	m_edit->SetWindowTextW(csTemp);

	//סַ3
	m_edit = (CEdit*)GetDlgItem(IDC_RESULT_ADDRESS);
	csTemp = resultStr.at(3).c_str();
	m_edit->SetWindowTextW(csTemp);

	//���֤����4
	m_edit = (CEdit*)GetDlgItem(IDC_RESULT_IDNUMBER);
	csTemp = resultStr.at(4).c_str();
	m_edit->SetWindowTextW(csTemp);

	finish = clock();
	totaltime = (double)(finish - start);
	LPTSTR lpsz = new TCHAR[100];
	_itot(totaltime, lpsz, 10);
	//MessageBox(lpsz);
}


void COCRDemoDlg::OnStnClickedTitleDay()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
}

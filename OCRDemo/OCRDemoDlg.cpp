
// OCRDemoDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "OCRDemo.h"
#include "OCRDemoDlg.h"
#include "afxdialogex.h"
#include <time.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// COCRDemoDlg 对话框



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


// COCRDemoDlg 消息处理程序

BOOL COCRDemoDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	namedWindow("view");
	HWND hWnd = (HWND)cvGetWindowHandle("view");
	HWND hParent = ::GetParent(hWnd);
	::SetParent(hWnd, GetDlgItem(IDC_PICTURE)->m_hWnd);
	::ShowWindow(hParent, SW_HIDE);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void COCRDemoDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR COCRDemoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void COCRDemoDlg::OnEnChangeEdit1()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}


void COCRDemoDlg::OnStnClickedTitleBirth2()
{
	// TODO: 在此添加控件通知处理程序代码
}


void COCRDemoDlg::OnBnClickedBtnOpen()
{
	// TODO: 在此添加控件通知处理程序代码
	CString picPath;   //定义图片路径变量  
	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_ALLOWMULTISELECT,
		NULL, this);   //选择文件对话框  
	
	if (dlg.DoModal() != IDOK)
	{
		return;
	}
	picPath = dlg.GetPathName();  //获取图片路径  

	//CString to string  使用这个方法记得字符集选用“使用多字节字符”，不然会报错
	USES_CONVERSION;
	string picpath(W2A(picPath));

	IDImage = imread(picpath);
	Mat imagedst;
	//以下操作获取图形控件尺寸并以此改变图片尺寸  
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

	// TODO: 在此添加控件通知处理程序代码
	vector<string> resultStr;
	OCRIDCard op;
	int ret = op.init();
	ret = op.recognizeIDCard(IDImage, resultStr);
	if (ret != 0)
		return;

	CEdit* m_edit;
	CString csTemp;

	//姓名0
	m_edit = (CEdit*)GetDlgItem(IDC_RESULT_NAME);
	csTemp = resultStr.at(0).c_str();
	m_edit->SetWindowTextW(csTemp);

	//性别1
	m_edit = (CEdit*)GetDlgItem(IDC_RESULT_GENDER);
	csTemp = resultStr.at(1).c_str();
	m_edit->SetWindowTextW(csTemp);

	//民族2
	m_edit = (CEdit*)GetDlgItem(IDC_RESULT_NATION);
	csTemp = resultStr.at(2).c_str();
	m_edit->SetWindowTextW(csTemp);

	//出生年月日 567
	m_edit = (CEdit*)GetDlgItem(IDC_RESULT_YEAR);
	csTemp = resultStr.at(5).c_str();
	m_edit->SetWindowTextW(csTemp);

	m_edit = (CEdit*)GetDlgItem(IDC_RESULT_MONTH);
	csTemp = resultStr.at(6).c_str();
	m_edit->SetWindowTextW(csTemp);

	m_edit = (CEdit*)GetDlgItem(IDC_RESULT_DAY);
	csTemp = resultStr.at(7).c_str();
	m_edit->SetWindowTextW(csTemp);

	//住址3
	m_edit = (CEdit*)GetDlgItem(IDC_RESULT_ADDRESS);
	csTemp = resultStr.at(3).c_str();
	m_edit->SetWindowTextW(csTemp);

	//身份证号码4
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
	// TODO: 在此添加控件通知处理程序代码
}

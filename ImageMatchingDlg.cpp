
// ImageMatchingDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "ImageMatching.h"
#include "ImageMatchingDlg.h"
#include "afxdialogex.h"
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CImageMatchingDlg 对话框



CImageMatchingDlg::CImageMatchingDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_IMAGEMATCHING_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_currentColor = RGB(0, 255, 0);
}

void CImageMatchingDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, myListBox);
	DDX_Control(pDX, IDC_LIST2, m_AlgsList);
	DDX_Control(pDX, IDC_LIST3, m_NMSList);
	DDX_Control(pDX, IDC_EDIT1, m_editCtrl);
	DDX_Control(pDX, IDC_BUTTONA, m_buttonA);
	DDX_Control(pDX, IDC_BUTTONB, m_buttonB);
	DDX_Control(pDX, IDC_BUTTON1, button1);
	DDX_Control(pDX, IDC_BUTTONA, m_buttonA);
	DDX_Control(pDX, IDC_BUTTONB, m_buttonB);
	DDX_Control(pDX, IDC_BUTTON1, button1);
	DDX_Control(pDX, IDC_BUTTON4, m_prevButton);
}

BEGIN_MESSAGE_MAP(CImageMatchingDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDCANCEL, &CImageMatchingDlg::OnBnClickedCancel)
	ON_LBN_SELCHANGE(IDC_LIST1, &CImageMatchingDlg::OnLbnSelchangeList1)
	ON_BN_CLICKED(IDC_BUTTON1, &CImageMatchingDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTONA, &CImageMatchingDlg::OnBnClickedButtona)
	ON_BN_CLICKED(IDC_BUTTONB, &CImageMatchingDlg::OnBnClickedButtonb)
	ON_WM_CTLCOLOR()
	ON_STN_CLICKED(IDC_STATIC_COLOR_BLOCK, &CImageMatchingDlg::OnStnClickedStaticColorBlock)
	ON_BN_CLICKED(IDC_BUTTON4, &CImageMatchingDlg::OnBnClickedButton4)
END_MESSAGE_MAP()


// CImageMatchingDlg 消息处理程序

inline double CImageMatchingDlg::ShowImageMatchControl(const cv::Mat& img, const CRect& OrigImg, const char* name, int controlID)
{
	if (img.empty()) return 1.0;

	// 获取控件句柄和初始最大尺寸
	CWnd* pWnd = GetDlgItem(controlID);
	if (!pWnd) return 1.0;

	CRect rectControl;
	pWnd->GetWindowRect(&rectControl);
	ScreenToClient(&rectControl); // 转换为当前对话框的客户区坐标

	// 允许的最大边界
	int maxW = OrigImg.Width();
	int maxH = OrigImg.Height();

	// 计算缩放比例，保持长宽比
	int imgW = img.cols;
	int imgH = img.rows;

	double scaleW = (double)maxW / imgW;
	double scaleH = (double)maxH / imgH;
	double scale = std::min(scaleW, scaleH); // 取较小的缩放比，确保两边都不超限

	// 计算消除黑边后的新控件尺寸
	int newW = static_cast<int>(imgW * scale);
	int newH = static_cast<int>(imgH * scale);

	// 居中对齐
	//int offsetX = (maxW - newW) / 2;
	//int offsetY = (maxH - newH) / 2;
	int offsetX = 0, offsetY = 0;

	// 更新控件位置和大小
	pWnd->MoveWindow(OrigImg.left + offsetX, OrigImg.top + offsetY, newW, newH, TRUE);

	// 使用 OpenCV 显示图片
	cv::resizeWindow(name, newW, newH);
	cv::imshow(name, img);

	m_wndOverlay.Invalidate(FALSE);
	return scale;
}

BOOL CImageMatchingDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_bgBrush.CreateSolidBrush(RGB(243, 243, 243));
	mFont.CreatePointFont(120, _T("Microsoft YaHei UI"));
	mSmallFont.CreatePointFont(100, _T("Microsoft YaHei UI"));

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	 // 设置每一行的高度
	// 参数1是索引（设为0表示全部），参数2是高度像素
	myListBox.SetItemHeight(0, 80);
	myListBox.SetFont(&mFont);
	// 添加左侧导航的选项内容
	myListBox.InsertString(-1, steps[0]);
	myListBox.SetCurSel(0);
	m_AlgsList.SetItemHeight(0, 40);
	m_AlgsList.SetFont(&mSmallFont);
	for (const auto& item : algs) {
		m_AlgsList.InsertString(-1, item);
	}
	m_AlgsList.SetCurSel(0);
	m_NMSList.SetItemHeight(0, 40);
	m_NMSList.SetFont(&mSmallFont);
	for (const auto& item : fort) {
		m_NMSList.InsertString(-1, item);
	}
	m_NMSList.SetCurSel(1);

	for (const auto i : {
		IDC_STATICA2,
		IDC_STATICB,
		IDC_EDITA,
		IDC_EDITB,
		IDC_STATICA,
		IDC_STATICA3,
		IDC_STATICA4
		}) {
		GetDlgItem(i)->SetFont(&mSmallFont);
	}

	cv::namedWindow("picViewA", cv::WINDOW_NORMAL);
	hWndA = (HWND)cvGetWindowHandle("picViewA");
	HWND hParentA = ::GetParent(hWndA);
	::SetParent(hWndA, GetDlgItem(IDC_IMGA)->m_hWnd);
	::ShowWindow(hParentA, SW_HIDE);

	cv::namedWindow("picViewB", cv::WINDOW_NORMAL);
	hWndB = (HWND)cvGetWindowHandle("picViewB");
	HWND hParentB = ::GetParent(hWndB);
	::SetParent(hWndB, GetDlgItem(IDC_IMGB)->m_hWnd);
	::ShowWindow(hParentB, SW_HIDE);

	GetDlgItem(IDC_IMGA)->GetWindowRect(&m_rcOrigImgA);
	ScreenToClient(&m_rcOrigImgA);
	GetDlgItem(IDC_IMGB)->GetWindowRect(&m_rcOrigImgB);
	ScreenToClient(&m_rcOrigImgB);

	scaleA = ShowImageMatchControl(pure, m_rcOrigImgA, "picViewA", IDC_IMGA);
	scaleB = ShowImageMatchControl(pure, m_rcOrigImgB, "picViewB", IDC_IMGB);

	 //创建遮罩窗口，注意传入 WS_EX_TRANSPARENT 样式
	CRect rectClient;
	GetClientRect(&rectClient);

	m_wndOverlay.CreateEx(
		WS_EX_TRANSPARENT,
		AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, ::LoadCursor(NULL, IDC_ARROW), (HBRUSH)GetStockObject(NULL_BRUSH)),
		_T(""),
		WS_CHILD | WS_VISIBLE,
		rectClient,
		this,
		12345 // 任意未占用的 ID
	);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CImageMatchingDlg::OnPaint()
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
		CRect rect;
		CPaintDC dc(this);
		GetClientRect(rect);
		dc.FillSolidRect(rect, RGB(243, 243, 243));
		
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CImageMatchingDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

// 转换函数：将某个控件内Mat图上的特征点，映射为对话框的客户区坐标
inline CPoint CImageMatchingDlg::MapMatPointToClient(CWnd* pCtrl, cv::Point matPt, double scale)
{
	// 点在控件内部的局部坐标
	int ctrlX = static_cast<int>(matPt.x * scale);
	int ctrlY = static_cast<int>(matPt.y * scale);

	// 获取控件相对于屏幕的矩形区域
	CRect ctrlRect;
	pCtrl->GetWindowRect(&ctrlRect);

	// 将屏幕坐标转换为对话框的客户区坐标
	// 此时 ctrlRect.Size() 变成了当前对话框下的相对坐标
	GetSafeHwnd(); // 确保安全
	this->ScreenToClient(&ctrlRect);

	// 最终对话框坐标 = 控件左上角客户区坐标 + 控件内偏移
	CPoint clientPt;
	clientPt.x = ctrlRect.left + ctrlX;
	clientPt.y = ctrlRect.top + ctrlY;

	return clientPt;
}

void CImageMatchingDlg::OnBnClickedCancel()
{
	CDialogEx::OnCancel();
	exit(0);
}

void CImageMatchingDlg::OnLbnSelchangeList1()
{
	int selIndex = myListBox.GetCurSel();
	if (selIndex == LB_ERR) return;

	// 根据步骤索引显示对应的图片
	if (selIndex == 0) {
		for (int i = IDC_BUTTONA; i <= IDC_BUTTONB; ++i) {
			GetDlgItem(i)->EnableWindow(TRUE);
		}
		GetDlgItem(IDC_LIST2)->EnableWindow(TRUE);
		GetDlgItem(IDC_LIST3)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC_COLOR_BLOCK)->EnableWindow(TRUE);
	}
	else
	{
		for (int i = IDC_BUTTONA; i <= IDC_BUTTONB; ++i) {
			GetDlgItem(i)->EnableWindow(FALSE);
		}
		GetDlgItem(IDC_LIST2)->EnableWindow(FALSE);
		GetDlgItem(IDC_LIST3)->EnableWindow(FALSE);
		GetDlgItem(IDC_STATIC_COLOR_BLOCK)->EnableWindow(FALSE);
	}
	// 显示左右两张图
	Mat left = (selIndex < (int)m_stepLeftImages.size() && !m_stepLeftImages[selIndex].empty())
		? m_stepLeftImages[selIndex]
		: Mat();
	Mat right = (selIndex < (int)m_stepRightImages.size() && !m_stepRightImages[selIndex].empty())
		? m_stepRightImages[selIndex]
		: Mat();

	if (!left.empty()) {
		scaleA = ShowImageMatchControl(left, m_rcOrigImgA, "picViewA", IDC_IMGA);
	}
	if (!right.empty()) {
		scaleB = ShowImageMatchControl(right, m_rcOrigImgB, "picViewB", IDC_IMGB);
	}
	switch (selIndex)
	{
	case 2:
		m_wndOverlay.m_vecLines = lines1;
		m_wndOverlay.SetShowLines(true);
		break;
	case 3:
		m_wndOverlay.m_vecLines = lines2;
		m_wndOverlay.SetShowLines(true);
		Invalidate();
		break;
	default:
		m_wndOverlay.SetShowLines(false);
		break;
	}
}
void CImageMatchingDlg::OnBnClickedButton1()
{
	int sel = myListBox.GetCurSel();
	if (sel == LB_ERR) return;

	// 特殊情况：在选择文件的界面点这个按钮，代表开始新的处理
	if (sel == 0) [[unlikely]] {
		CString sa, sb;
		GetDlgItem(IDC_EDITA)->GetWindowTextW(sa);
		GetDlgItem(IDC_EDITB)->GetWindowTextW(sb);
		if (sa.IsEmpty() || sb.IsEmpty()) {
			return;
		}
		currentStep = 0;
		myListBox.ResetContent();
		myListBox.AddString(steps[0]);
		controller.setImages(imread((String)CT2A(sa)),
			imread((String)CT2A(sb)));
		controller.setAlgorithm((String)CT2A(algs[m_AlgsList.GetCurSel()]), m_NMSList.GetCurSel());
		double b = (m_currentColor >> 16) & 0xFF;
		double g = (m_currentColor >> 8) & 0xFF;
		double r = m_currentColor & 0xFF;
		controller.getContext().color = { b, g, r };
		m_wndOverlay.color = m_currentColor;
		time.clear();
		m_editCtrl.SetWindowTextW(_T(""));
		m_stepLeftImages.clear();
		m_stepRightImages.clear();
		lines1.clear();
		lines2.clear();
		m_wndOverlay.m_vecLines.clear();
		m_wndOverlay.Invalidate();
		m_stepLeftImages.push_back(OrigMatA);
		m_stepRightImages.push_back(OrigMatB);
	}
	
	for (int i = IDC_BUTTONA; i <= IDC_BUTTONB; ++i) {
		GetDlgItem(i)->EnableWindow(FALSE);
	}
	GetDlgItem(IDC_LIST2)->EnableWindow(FALSE);
	GetDlgItem(IDC_LIST3)->EnableWindow(FALSE);
	GetDlgItem(IDC_STATIC_COLOR_BLOCK)->EnableWindow(FALSE);

	// 情况1：回看历史步骤 -> 仅切换到下一步（已有结果）
	if (sel < currentStep) {
		int next = sel + 1;
		if (next <= currentStep) {
			myListBox.SetCurSel(next);
			OnLbnSelchangeList1();   // 刷新图片显示
		}
		return;
	}

	// 情况2：当前选中的是最新步骤，尝试执行新的下一步
	if (sel == currentStep) {
		if (currentStep + 1 < (int)steps.size()) {
			++currentStep;
			myListBox.InsertString(-1, steps[currentStep]);
			myListBox.SetCurSel(currentStep);

			// 执行算法，获取新步骤的图像
			controller.executeNextStep();
			time.push_back(controller.getContext().executionTimeMs);
			int stepIdx = controller.getCurrentStepIndex();
			Mat leftImg = controller.getCurrentLeftImage();
			Mat rightImg = controller.getCurrentRightImage();

			// 存储左右图
			m_stepLeftImages.push_back(leftImg.clone());
			m_stepRightImages.push_back(rightImg.clone());

			// 根据步骤类型显示图片
			OnLbnSelchangeList1();

			// 这时候要获取连线
			if (sel == 1) {
				const StitchingContext& context = controller.getContext();
				for (size_t i = 0; i < context.goodMatches.size(); i++) {
					auto PointA = context.kpsTemplate[context.goodMatches[i].queryIdx].pt;
					auto PointB = context.kpsScene[context.goodMatches[i].trainIdx].pt;
					lines1.push_back({
						MapMatPointToClient(GetDlgItem(IDC_IMGA), PointA, scaleA),
						MapMatPointToClient(GetDlgItem(IDC_IMGB), PointB, scaleB),
						0
					});
				}
				m_wndOverlay.m_vecLines = lines1;
			}
			else if (sel == 2) {
				const StitchingContext& context = controller.getContext();
				for (size_t i = 0; i < context.inlierMatches.size(); i++) {
					auto PointA = context.kpsTemplate[context.inlierMatches[i].queryIdx].pt;
					auto PointB = context.kpsScene[context.inlierMatches[i].trainIdx].pt;
					lines2.push_back({
						MapMatPointToClient(GetDlgItem(IDC_IMGA), PointA, scaleA),
						MapMatPointToClient(GetDlgItem(IDC_IMGB), PointB, scaleB),
						0
					});
				}
				m_wndOverlay.m_vecLines = lines2;
			}

			// 显示时间
			CString str;
			str.Format(_T("步骤%d: %s\r\n耗时: %.1fms\r\n\r\n"), sel + 1, steps[sel + 1], time[sel]);
			int len = m_editCtrl.GetWindowTextLength();
			m_editCtrl.SetSel(len, len);
			m_editCtrl.ReplaceSel(str);

			// 最后一步计算完成之后，还要显示其他统计数据
			if (sel == (int)steps.size() - 2) {
				str.Format(_T("初始匹配总数: %d\r\n正确匹配点数: %d\r\n匹配准确率: %.1f%%"),
					controller.getContext().totalInitialMatches,
					controller.getContext().matchedPointsCount,
					controller.getContext().accuracyScore);
				int len = m_editCtrl.GetWindowTextLength();
				m_editCtrl.SetSel(len, len);
				m_editCtrl.ReplaceSel(str);
			}
		}
		else {
			AfxMessageBox(_T("已经是最后一步了"));
		}
	}
}

void CImageMatchingDlg::OnBnClickedButtona()
{
	CFileDialog fileDlg(TRUE);
	fileDlg.m_ofn.lpstrTitle = _T("浏览文件");
	fileDlg.m_ofn.lpstrFilter = _T("PNG (*.png)\0*.png\0JPEG (*.jpg;*.jpeg;*.jpe;*.jfif)\0*.jpg; *.jpeg; *.jpe; *.jfif\0所有文件(*.*)\0*.*\0\0");
	if (IDOK == fileDlg.DoModal())
	{
		GetDlgItem(IDC_EDITA)->SetWindowTextW(fileDlg.GetPathName());
		OrigMatA = imread((String)CT2A(fileDlg.GetPathName()));
		ShowImageMatchControl(OrigMatA, m_rcOrigImgA, "picViewA", IDC_IMGA);
	}
}

void CImageMatchingDlg::OnBnClickedButtonb()
{
	CFileDialog fileDlg(TRUE);
	fileDlg.m_ofn.lpstrTitle = _T("浏览文件");
	fileDlg.m_ofn.lpstrFilter = _T("PNG (*.png)\0*.png\0JPEG (*.jpg;*.jpeg;*.jpe;*.jfif)\0*.jpg; *.jpeg; *.jpe; *.jfif\0所有文件(*.*)\0*.*\0\0");
	if (IDOK == fileDlg.DoModal())
	{
		GetDlgItem(IDC_EDITB)->SetWindowTextW(fileDlg.GetPathName());
		OrigMatB = imread((String)CT2A(fileDlg.GetPathName()));
		ShowImageMatchControl(OrigMatB, m_rcOrigImgB, "picViewB", IDC_IMGB);
	}
}

HBRUSH CImageMatchingDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);

	// 检查是不是要修改的特定 ID 的控件
	switch (pWnd->GetDlgCtrlID()) {
	case IDC_EDIT1:
	case IDC_STATICA2:
	case IDC_STATICB:
	case IDC_EDITA:
	case IDC_EDITB:
	case IDC_STATICA:
	case IDC_STATICA3:
	case IDC_STATICA4:
		// 设置文本的背景颜色，使其与刷子颜色一致，防止文字周围有白边
		pDC->SetBkColor(RGB(243, 243, 243));
		pDC->SetTextColor(RGB(0, 0, 0)); // 设置文本颜色
		// 返回创建的刷子
		return (HBRUSH)m_bgBrush.GetSafeHandle();
	}

	// 判断当前正在绘制的控件是否是颜色展示块
	if (pWnd->GetDlgCtrlID() == IDC_STATIC_COLOR_BLOCK)
	{
		// 设置背景色为当前选中的颜色（默认为绿色）
		pDC->SetBkColor(m_currentColor);

		// 返回一个对应颜色的实心画刷，系统会用它来填充控件背景
		// 注意：为避免内存泄漏，建议将 m_brush 定义为类的成员变量，并在构造函数中初始化
		m_brush.DeleteObject();
		m_brush.CreateSolidBrush(m_currentColor);
		return m_brush;
	}

	return hbr;
}

void CImageMatchingDlg::OnStnClickedStaticColorBlock()
{
	CColorDialog colorDlg(m_currentColor);
	if (colorDlg.DoModal() == IDOK)
	{
		m_currentColor = colorDlg.GetColor();
		GetDlgItem(IDC_STATIC_COLOR_BLOCK)->Invalidate();
	}
}

void CImageMatchingDlg::OnBnClickedButton4()
{
	int sel = myListBox.GetCurSel();
	if (sel == LB_ERR) return;
	if (sel == 1) {
		for (int i = IDC_BUTTONA; i <= IDC_BUTTONB; ++i) {
			GetDlgItem(i)->EnableWindow(TRUE);
		}
		GetDlgItem(IDC_LIST2)->EnableWindow(TRUE);
		GetDlgItem(IDC_LIST3)->EnableWindow(TRUE);
		GetDlgItem(IDC_STATIC_COLOR_BLOCK)->EnableWindow(TRUE);
	}
	int next = sel - 1;
	if (next >= 0) {
		myListBox.SetCurSel(next);
		OnLbnSelchangeList1();   // 刷新图片显示
	}
}

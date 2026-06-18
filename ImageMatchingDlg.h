
// ImageMatchingDlg.h: 头文件
//

#pragma once

#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui_c.h>
#include "CNavListBox.h"
#include <vector>
#include "MatchingAlgs.h"
#include "COverlayWnd.h"

// CImageMatchingDlg 对话框
class CImageMatchingDlg : public CDialogEx
{
// 构造
public:
	CImageMatchingDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_IMAGEMATCHING_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;
	HWND hWndA, hWndB;
	bool f0 = false;
	std::vector<const wchar_t *> steps{
		_T("读取文件"),
		_T("检测特征点"),
		_T("计算描述子"),
		_T("特征匹配"),
		_T("计算单应性矩阵")
	};
	std::vector<const wchar_t*> algs{
		_T("ORB"),
		_T("SIFT")
	};
	std::vector<const wchar_t*> fort{ // F or T
		_T("不使用"),
		_T("使用")
	};
	int currentStep = 0; // 记录现在到哪一步了
	CRect m_rcOrigImgA, m_rcOrigImgB; // 记录两个图片控件的原始尺寸
	double scaleA = 1.0, scaleB = 1.0;
	StitchController controller;
    double ShowImageMatchControl(const cv::Mat& img, const CRect& OrigImg, const char* name, int controlID);

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

	// 转换函数：将某个控件内Mat图上的特征点，映射为对话框的客户区坐标
	CPoint MapMatPointToClient(CWnd* pCtrl, cv::Point matPt, double scale);

private:
	CFont mFont;
	cv::Mat OrigMatA, OrigMatB;
	cv::Mat pure = { 1, 1, CV_8UC3, cv::Scalar(243, 243, 243) };
	std::vector<cv::Mat> m_stepLeftImages;  // 存储每一步的左图
	std::vector<cv::Mat> m_stepRightImages; // 存储每一步的右图

public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnLbnSelchangeList1();
	CNavListBox myListBox;
	afx_msg void OnBnClickedButton1();

	COverlayWnd m_wndOverlay; // 声明遮罩窗口对象
	

	afx_msg void OnBnClickedButtona();
	afx_msg void OnBnClickedButtonb();
	CNavListBox m_AlgsList;
	CNavListBox m_NMSList;
};

#pragma once

// 避免与 ImageMatchingDlg.h 的循环包含，使用前向声明
class CImageMatchingDlg;
// CNavListBox

class CNavListBox : public CListBox
{
	DECLARE_DYNAMIC(CNavListBox)

public:
	CNavListBox();
	virtual ~CNavListBox();
	void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	int SetCurSel(int nSelect);

protected:
	//CImageMatchingDlg* pwnd;
	int m_nHoverIndex;  // 记录当前鼠标悬停在哪一项上
	BOOL m_bTracking;   // 记录系统是否正在追踪鼠标状态
	void OnMouseMove(UINT nFlags, CPoint point);
	void OnMouseLeave();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	BOOL OnEraseBkgnd(CDC* pDC);
	DECLARE_MESSAGE_MAP()
};




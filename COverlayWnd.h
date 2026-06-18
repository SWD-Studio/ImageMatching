#pragma once
#include <afxwin.h>
#include <vector>

struct MyLine {
    POINT ptStart;
    POINT ptEnd;
    COLORREF color; // 被忽略
};

class COverlayWnd :
    public CWnd
{
    DECLARE_DYNAMIC(COverlayWnd)

public:
    COverlayWnd();
    virtual ~COverlayWnd();

    void SetLinesData(const std::vector<MyLine>& lines);

    void SetShowLines(bool bShow); // 控制同步显隐

    std::vector<MyLine> m_vecLines;

protected:
    bool m_bShowLines = false;
    afx_msg LRESULT OnNcHitTest(CPoint point);
    afx_msg void OnPaint();


    DECLARE_MESSAGE_MAP()
};


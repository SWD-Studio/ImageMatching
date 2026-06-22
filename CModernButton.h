#pragma once
#include <afxwin.h>
class CModernButton :
    public CButton
{
public:
    CModernButton();
    virtual ~CModernButton();
protected:
    // 核心：重写绘制函数
    virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) override;

    // 鼠标消息处理
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg LRESULT OnMouseLeave(WPARAM wParam, LPARAM lParam);
    afx_msg void OnLButtonDown(UINT nFlags, CPoint point); // 处理按下
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);   // 处理抬起

    DECLARE_MESSAGE_MAP()

private:
    bool m_bIsHover;     // 鼠标悬停
    bool m_bIsPressed;   // 鼠标按下
    // 定义颜色常量
    const COLORREF COLOR_BORDER = RGB(0, 108, 190); // 默认边框蓝
    const COLORREF COLOR_NORMAL_BG = RGB(255, 255, 255); // 默认背景白
    const COLORREF COLOR_HOVER_BG = RGB(201, 222, 245); // 悬停浅蓝
    const COLORREF COLOR_PRESSED_BG = RGB(0, 108, 190); // 按下深蓝
    const COLORREF COLOR_TEXT_NORMAL = RGB(30, 30, 30);     // 默认黑字
    const COLORREF COLOR_TEXT_WHITE = RGB(255, 255, 255); // 按下时白字
    const COLORREF COLOR_DISABLED_BG = RGB(250, 250, 250); // 禁用时灰色
    const COLORREF COLOR_TEXT_DISABLED = RGB(160, 160, 160); // 禁用时文字
    CFont m_font;
};


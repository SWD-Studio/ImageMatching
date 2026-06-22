#include "pch.h"
#include "CModernButton.h"

CModernButton::CModernButton()
{
    m_bIsHover = false;
    m_bIsPressed = false;
    m_font.CreatePointFont(90, _T("Microsoft YaHei UI"));
}

CModernButton::~CModernButton() {}

BEGIN_MESSAGE_MAP(CModernButton, CButton)
    ON_WM_MOUSEMOVE()
    ON_MESSAGE(WM_MOUSELEAVE, OnMouseLeave)
    ON_WM_LBUTTONDOWN() // 绑定按下消息
    ON_WM_LBUTTONUP()   // 绑定抬起消息
END_MESSAGE_MAP()
// 鼠标移动：检测悬停
void CModernButton::OnMouseMove(UINT nFlags, CPoint point)
{
    if (!m_bIsHover)
    {
        m_bIsHover = true;
        Invalidate(); // 重绘按钮以显示悬停颜色

        // 注册鼠标离开消息
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hWnd;
        ::TrackMouseEvent(&tme);
    }
    CButton::OnMouseMove(nFlags, point);
}
LRESULT CModernButton::OnMouseLeave(WPARAM wParam, LPARAM lParam)
{
    m_bIsHover = false;
    m_bIsPressed = false; // 离开时如果还按着，也要复位
    Invalidate();
    return 0;
}
// 鼠标左键按下
void CModernButton::OnLButtonDown(UINT nFlags, CPoint point)
{
    m_bIsPressed = true;
    Invalidate(); // 立即重绘显示深蓝色
    CButton::OnLButtonDown(nFlags, point);
}

// 鼠标左键抬起
void CModernButton::OnLButtonUp(UINT nFlags, CPoint point)
{
    m_bIsPressed = false;
    Invalidate(); // 恢复为悬停色或默认色
    CButton::OnLButtonUp(nFlags, point);
}
void CModernButton::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    if (!lpDrawItemStruct) return;

    CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
    CRect rect = lpDrawItemStruct->rcItem;
    UINT state = lpDrawItemStruct->itemState;

    // 1. 确定当前背景色和文字颜色
    COLORREF bgColor, textColor, borderColor = COLOR_BORDER;

    // 优先级：禁用 > 按下 > 悬停 > 默认
    if (state & ODS_DISABLED)
    {
        bgColor = COLOR_DISABLED_BG;
        textColor = COLOR_TEXT_DISABLED;
        borderColor = COLOR_TEXT_DISABLED;
    }
    else if (m_bIsPressed || (state & ODS_SELECTED))
    {
        bgColor = COLOR_PRESSED_BG;
        textColor = COLOR_TEXT_WHITE; // 深色背景下用白字
    }
    else if (m_bIsHover)
    {
        bgColor = COLOR_HOVER_BG;
        textColor = COLOR_TEXT_NORMAL;
    }
    else
    {
        bgColor = COLOR_NORMAL_BG;
        textColor = COLOR_TEXT_NORMAL;
    }

    // 2. 绘制直角背景
    // 使用 FillSolidRect 绘制纯色矩形，无圆角
    pDC->FillSolidRect(&rect, bgColor);

    // 3. 绘制蓝色边框
    // 创建一个单像素宽的蓝色画笔
    CPen penBorder(PS_SOLID, 1, borderColor);
    CPen* pOldPen = pDC->SelectObject(&penBorder);

    // 创建一个空画刷，避免填充覆盖刚才的背景
    CBrush brushNull;
    brushNull.CreateStockObject(NULL_BRUSH);
    CBrush* pOldBrush = pDC->SelectObject(&brushNull);

    // 绘制矩形边框 (Rectangle会画出四条边)
    // 注意：为了防止边框被裁剪，通常向内缩进0.5像素，但在GDI整数坐标下直接画即可
    pDC->Rectangle(&rect);

    // 恢复旧画笔和画刷
    pDC->SelectObject(pOldPen);
    pDC->SelectObject(pOldBrush);

    // 4. 绘制文字
    pDC->SetBkMode(TRANSPARENT); // 文字背景透明
    pDC->SetTextColor(textColor);

    // 选入我们创建的微软雅黑字体
    CFont* pOldFont = pDC->SelectObject(&m_font);

    // 获取按钮文字
    CString strText;
    GetWindowText(strText);

    // 居中绘制文字
    pDC->DrawText(strText, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // 恢复旧字体
    pDC->SelectObject(pOldFont);


}
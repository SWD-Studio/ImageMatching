// CNavListBox.cpp: 实现文件
//

#include "pch.h"
#include "CNavListBox.h"
//#include "ImageMatchingDlg.h"


// CNavListBox

IMPLEMENT_DYNAMIC(CNavListBox, CListBox)

CNavListBox::CNavListBox()
{
    m_nHoverIndex = -1; // -1 表示鼠标没有停留在任何有效项上
    m_bTracking = FALSE;
    //pwnd = dynamic_cast<CImageMatchingDlg*>(this->GetParent());
}

CNavListBox::~CNavListBox()
{
}

inline void CNavListBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
    CRect rect = lpDrawItemStruct->rcItem;
    int nIndex = lpDrawItemStruct->itemID;

    // 如果列表为空，直接返回
    if (nIndex == -1) return;

    // 获取当前项的文字
    CString strText;
    GetText(nIndex, strText);

    // 判断是否被选中
    BOOL bIsSelected = (lpDrawItemStruct->itemState & ODS_SELECTED);
    BOOL bIsHover = (nIndex == m_nHoverIndex); // 判断这一行是不是正被鼠标悬停

    if (bIsSelected) {
        // 1. 画选中项的整体背景（极浅的蓝灰色）
        pDC->FillSolidRect(&rect, RGB(232, 235, 240));
        pDC->SetTextColor(RGB(0, 0, 0)); // 激活时文字用更深的黑色

        // 2. 画左侧的蓝色指示条 (Accent Bar)
        CRect barRect = rect;
        barRect.right = barRect.left + 4; // 竖线的宽度设为 4 个像素
        // 为了更美观，可以让竖线上下留一点空隙（可选）
        // barRect.top += 4; 
        // barRect.bottom -= 4;
        pDC->FillSolidRect(&barRect, RGB(0, 95, 184));
    }
    else if (bIsHover)
    {
        // 状态 2：未选中，但鼠标正【悬停】在上面
        // 颜色比普通背景稍深一点点，给出丝滑的交互反馈
        pDC->FillSolidRect(&rect, RGB(229, 229, 229));
        pDC->SetTextColor(RGB(30, 30, 30));
    }
    else {
        // 未选中状态：普通浅灰背景
        pDC->FillSolidRect(&rect, RGB(243, 243, 243));
        pDC->SetTextColor(RGB(30, 30, 30));
    }
    // 设置背景透明并绘制文字
    pDC->SetBkMode(TRANSPARENT);
    // 将文字在矩形中垂直居中、左对齐（留出一定边距）
    rect.left += 15;
    pDC->DrawText(strText, &rect, DT_SINGLELINE | DT_VCENTER | DT_LEFT);
}

int CNavListBox::SetCurSel(int nSelect)
{
    int ret = CListBox::SetCurSel(nSelect);
    //dynamic_cast<CImageMatchingDlg*>(this->GetParent())->m_wndOverlay.Invalidate(FALSE);
    return ret;
}

// 绑定消息映射
BEGIN_MESSAGE_MAP(CNavListBox, CListBox)
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSELEAVE() // 捕获鼠标离开
    ON_WM_LBUTTONDOWN()  // 映射左键按下消息
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()



// CNavListBox 消息处理程序


// 鼠标移动事件
void CNavListBox::OnMouseMove(UINT nFlags, CPoint point)
{
    // 1. 如果没有在追踪鼠标，则开启追踪（这样鼠标移出控件外时，我们才能收到 WM_MOUSELEAVE 消息）
    if (!m_bTracking)
    {
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = this->GetSafeHwnd();
        if (::_TrackMouseEvent(&tme))
        {
            m_bTracking = TRUE;
        }
    }

    // 2. 计算当前鼠标坐标落在哪个列表项上
    BOOL bOutside = FALSE;
    int nIndex = ItemFromPoint(point, bOutside);

    // 如果鼠标超出了实际列表项的范围（比如底部空白处），则视为没有悬停
    if (bOutside)
    {
        nIndex = -1;
    }

    // 3. 如果鼠标悬停的项发生了变化，才需要重绘
    if (nIndex != m_nHoverIndex)
    {
        m_nHoverIndex = nIndex;
        // Invalidate(FALSE) 表示不擦除整个背景地重绘，能有效避免闪烁
        Invalidate(FALSE);
        //dynamic_cast<CImageMatchingDlg*>(this->GetParent())->m_wndOverlay.Invalidate(FALSE);
    }

    CListBox::OnMouseMove(nFlags, point);
}

// 鼠标离开事件
void CNavListBox::OnMouseLeave()
{
    m_bTracking = FALSE; // 重置追踪标志

    // 如果之前有悬停的高亮，现在鼠标走了，应该清除它
    if (m_nHoverIndex != -1)
    {
        m_nHoverIndex = -1;
        Invalidate(FALSE);
        //dynamic_cast<CImageMatchingDlg*>(this->GetParent())->m_wndOverlay.Invalidate(FALSE);
    }
}

// 鼠标左键单击事件
void CNavListBox::OnLButtonDown(UINT nFlags, CPoint point)
{
    CListBox::OnLButtonDown(nFlags, point);
    //dynamic_cast<CImageMatchingDlg*>(this->GetParent())->m_wndOverlay.Invalidate(FALSE);
}

BOOL CNavListBox::OnEraseBkgnd(CDC* pDC)
{
    // 获取 ListBox 整个控件的矩形大小（包括没有选项的空白区域）
    CRect rectClient;
    GetClientRect(&rectClient);

    // 用我们方案中的“普通未选中状态背景色”填满它，保持视觉统一
    pDC->FillSolidRect(&rectClient, RGB(243, 243, 243));

    // 返回 TRUE 告诉 Windows：“我自己已经画好背景了，你不需要再用默认的白色去画了”
    return TRUE;
}
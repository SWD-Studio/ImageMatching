#include "pch.h"
#include "COverlayWnd.h"

IMPLEMENT_DYNAMIC(COverlayWnd, CWnd)

COverlayWnd::COverlayWnd() : m_bShowLines(true) {}
COverlayWnd::~COverlayWnd() {}

inline void COverlayWnd::SetLinesData(const std::vector<MyLine>& lines) {
    m_vecLines = lines;
    if (GetSafeHwnd()) Invalidate(FALSE);
}

BEGIN_MESSAGE_MAP(COverlayWnd, CWnd)
    ON_WM_NCHITTEST()
    ON_WM_PAINT()
END_MESSAGE_MAP()

// 1. 实现鼠标穿透
LRESULT COverlayWnd::OnNcHitTest(CPoint point)
{
    // 关键：返回 HTTRANSPARENT，鼠标事件就会穿透到下层控件
    return HTTRANSPARENT;
}

// 2. 核心绘制逻辑
void COverlayWnd::OnPaint()
{
    CPaintDC dc(this);

    if (!m_bShowLines || m_vecLines.empty()) return;

    CRect rect;
    GetClientRect(&rect);

    // -------------------------------------------------------------
    // 1. 初始化双缓冲 (内存 DC)
    // -------------------------------------------------------------
    CDC memDC;
    CBitmap memBitmap;
    memDC.CreateCompatibleDC(&dc);
    memBitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height());
    CBitmap* pOldBitmap = memDC.SelectObject(&memBitmap);

    // 关键：因为是透明透明遮罩，内存DC初始化时必须完全透明/全白
    // 这里使用底层的标准不透明填充，随后由系统的 WS_EX_TRANSPARENT 混合
    // 或者直接使用透明画刷清除背景
    memDC.FillSolidRect(&rect, RGB(255, 255, 255));

    // -------------------------------------------------------------
    // 2. 设置画笔
    // -------------------------------------------------------------
    CPen pen(PS_SOLID, 4, RGB(0, 255, 0));
    CPen* pOldPen = memDC.SelectObject(&pen);

    // -------------------------------------------------------------
    // 3. 高性能批量绘制 (Polyline)
    // -------------------------------------------------------------
    // 如果使用 MoveTo/LineTo，1万条线要调用2万次 GDI 命令
    // 使用 Polyline 可以把多条连续或不连续的线成批交给底层驱动

    size_t lineCount = m_vecLines.size();

    // Polyline 要求传入一个连续的点数组
    // 每两条点连成一条线，我们可以分批或者用更高效的 PolyPolyline
    // 这里采用最稳健的成对绘制，能显著减少用户态到内核态的切换
    for (size_t i = 0; i < lineCount; ++i)
    {
        CPoint pts[2] = { m_vecLines[i].ptStart, m_vecLines[i].ptEnd };
        memDC.Polyline(pts, 2);
    }

    // -------------------------------------------------------------
    // 4. 将内存中的画面一次性拷贝到屏幕
    // -------------------------------------------------------------
    // 使用 TransparentBlt 可以指定某种颜色为透明色（比如我们刚才填充的白色）
    // 这样就只有线条会被画到屏幕上，而不会遮挡底层控件
    dc.TransparentBlt(
        0, 0, rect.Width(), rect.Height(),
        &memDC,
        0, 0, rect.Width(), rect.Height(),
        RGB(255, 255, 255) // 把刚才填充的白色设为透明
    );

    // 5. 释放资源
    memDC.SelectObject(pOldPen);
    memDC.SelectObject(pOldBitmap);
}


void COverlayWnd::SetShowLines(bool bShow)
{
    m_bShowLines = bShow;
    if (m_bShowLines) {
        ShowWindow(SW_SHOW);
        Invalidate(); // 刷新重绘
    }
    else {
        ShowWindow(SW_HIDE);
    }
}
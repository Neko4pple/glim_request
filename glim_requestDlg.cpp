#include "pch.h"
#include "framework.h"
#include "glim_request.h"
#include "glim_requestDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CGlimRequestDlg, CDialogEx)
    ON_WM_PAINT()
    ON_WM_DESTROY()
    ON_WM_TIMER()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MOUSEMOVE()
    ON_BN_CLICKED(IDC_BTN_RESET, &CGlimRequestDlg::OnBnClickedButtonReset)
    ON_BN_CLICKED(IDC_BTN_RANDOM_MOVE, &CGlimRequestDlg::OnBnClickedButtonRandom)
    ON_MESSAGE(WM_APP_RANDOM_UPDATE, &CGlimRequestDlg::OnRandomUpdate)
END_MESSAGE_MAP()

CGlimRequestDlg::CGlimRequestDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_GLIM_REQUEST_DIALOG, pParent)
{
}

CGlimRequestDlg::~CGlimRequestDlg()
{
    StopRandomMoveThread();
}

void CGlimRequestDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BOOL CGlimRequestDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    SetDlgItemText(IDC_EDIT_POINT_RADIUS, _T("8"));
    SetDlgItemText(IDC_EDIT_BORDER_THICKNESS, _T("2"));

    CWnd* pCanvasWnd = GetDlgItem(IDC_STATIC_CANVAS);
    if (pCanvasWnd != nullptr)
    {
        pCanvasWnd->GetWindowRect(&m_drawRect);
        ScreenToClient(&m_drawRect);
        pCanvasWnd->ShowWindow(SW_HIDE);
    }
    else
    {
        CRect rcClient;
        GetClientRect(&rcClient);
        m_drawRect.SetRect(20, 70, rcClient.right - 20, rcClient.bottom - 20);
    }

    UpdatePointLabels();
    InitCanvas();
    RebuildScene();

    return TRUE;
}

void CGlimRequestDlg::OnDestroy()
{
    m_destroying = true;
    StopRandomMoveThread();

    if (!m_canvas.IsNull())
    {
        m_canvas.Destroy();
    }

    CDialogEx::OnDestroy();
}

void CGlimRequestDlg::OnTimer(UINT_PTR nIDEvent)
{
    if (nIDEvent == TIMER_ID_INITIAL_DRAW)
    {
        KillTimer(TIMER_ID_INITIAL_DRAW);
        Invalidate(FALSE);
    }

    CDialogEx::OnTimer(nIDEvent);
}

void CGlimRequestDlg::InitCanvas()
{
    if (!m_canvas.IsNull())
    {
        m_canvas.Destroy();
    }

    const int width = m_drawRect.Width();
    const int height = m_drawRect.Height();

    m_canvas.Create(width, height, 32);
    ClearCanvas();
}

void CGlimRequestDlg::ClearCanvas(COLORREF bgColor)
{
    if (m_canvas.IsNull())
        return;

    const int width = m_canvas.GetWidth();
    const int height = m_canvas.GetHeight();
    const int pitch = m_canvas.GetPitch();
    BYTE* pBits = static_cast<BYTE*>(m_canvas.GetBits());

    if (pBits == nullptr)
        return;

    const BYTE r = GetRValue(bgColor);
    const BYTE g = GetGValue(bgColor);
    const BYTE b = GetBValue(bgColor);

    for (int y = 0; y < height; ++y)
    {
        BYTE* pRow = pBits + y * pitch;
        for (int x = 0; x < width; ++x)
        {
            BYTE* px = pRow + x * 4;
            px[0] = b;
            px[1] = g;
            px[2] = r;
            px[3] = 0;
        }
    }
}

void CGlimRequestDlg::PutPixelSafe(int x, int y, COLORREF color)
{
    if (m_canvas.IsNull())
        return;

    const int width = m_canvas.GetWidth();
    const int height = m_canvas.GetHeight();

    if (x < 0 || x >= width || y < 0 || y >= height)
        return;

    const int pitch = m_canvas.GetPitch();
    BYTE* pBits = static_cast<BYTE*>(m_canvas.GetBits());
    if (pBits == nullptr)
        return;

    BYTE* pRow = pBits + y * pitch;
    BYTE* px = pRow + x * 4;

    px[0] = GetBValue(color);
    px[1] = GetGValue(color);
    px[2] = GetRValue(color);
    px[3] = 0;
}

void CGlimRequestDlg::DrawFilledCirclePixel(int cx, int cy, int radius, COLORREF color)
{
    if (radius <= 0)
        return;

    const int r2 = radius * radius;

    for (int dy = -radius; dy <= radius; ++dy)
    {
        for (int dx = -radius; dx <= radius; ++dx)
        {
            if (dx * dx + dy * dy <= r2)
            {
                PutPixelSafe(cx + dx, cy + dy, color);
            }
        }
    }
}

void CGlimRequestDlg::DrawCircleOutlinePixel(int cx, int cy, int radius, int thickness, COLORREF color)
{
    if (radius <= 0)
        return;

    thickness = max(1, thickness);

    const int outerR = radius;
    const int innerR = max(0, radius - thickness + 1);

    const int outer2 = outerR * outerR;
    const int inner2 = innerR * innerR;

    for (int y = cy - outerR; y <= cy + outerR; ++y)
    {
        for (int x = cx - outerR; x <= cx + outerR; ++x)
        {
            const int dx = x - cx;
            const int dy = y - cy;
            const int dist2 = dx * dx + dy * dy;

            if (dist2 <= outer2 && dist2 >= inner2)
            {
                PutPixelSafe(x, y, color);
            }
        }
    }
}

bool CGlimRequestDlg::ComputeCircumcircle(
    const PointF& a,
    const PointF& b,
    const PointF& c,
    PointF& center,
    double& radius)
{
    const double d =
        2.0 * (
            a.x * (b.y - c.y) +
            b.x * (c.y - a.y) +
            c.x * (a.y - b.y)
            );

    if (fabs(d) < 1e-9)
        return false;

    const double a2 = a.x * a.x + a.y * a.y;
    const double b2 = b.x * b.x + b.y * b.y;
    const double c2 = c.x * c.x + c.y * c.y;

    center.x = (a2 * (b.y - c.y) + b2 * (c.y - a.y) + c2 * (a.y - b.y)) / d;
    center.y = (a2 * (c.x - b.x) + b2 * (a.x - c.x) + c2 * (b.x - a.x)) / d;

    const double dx = a.x - center.x;
    const double dy = a.y - center.y;
    radius = sqrt(dx * dx + dy * dy);

    return true;
}

void CGlimRequestDlg::UpdateCircumcircle()
{
    m_hasCircumcircle = false;
    m_circleCenter = {};
    m_circleRadius = 0.0;

    if (m_points.size() != 3)
        return;

    PointF center{};
    double radius = 0.0;

    if (ComputeCircumcircle(m_points[0], m_points[1], m_points[2], center, radius))
    {
        m_hasCircumcircle = true;
        m_circleCenter = center;
        m_circleRadius = radius;
    }
}

void CGlimRequestDlg::RebuildScene()
{
    std::lock_guard<std::mutex> lock(m_dataMutex);

    if (m_canvas.IsNull())
        return;

    ClearCanvas();

    const int pointRadius = GetPointRadius();
    const int circleThickness = GetCircleThickness();

    UpdateCircumcircle();

    if (m_hasCircumcircle)
    {
        const int cx = static_cast<int>(std::lround(m_circleCenter.x));
        const int cy = static_cast<int>(std::lround(m_circleCenter.y));
        const int rr = static_cast<int>(std::lround(m_circleRadius));

        DrawCircleOutlinePixel(cx, cy, rr, circleThickness, RGB(0, 0, 0));
    }

    for (const auto& pt : m_points)
    {
        const int x = static_cast<int>(std::lround(pt.x));
        const int y = static_cast<int>(std::lround(pt.y));
        DrawFilledCirclePixel(x, y, pointRadius, RGB(0, 0, 0));
    }

    UpdatePointLabels();
    Invalidate(FALSE);
    UpdateWindow();
}

int CGlimRequestDlg::GetPointRadius() const
{
    CString s;
    GetDlgItemText(IDC_EDIT_POINT_RADIUS, s);

    int v = _ttoi(s);
    if (v <= 0)
        v = 8;
    return v;
}

int CGlimRequestDlg::GetCircleThickness() const
{
    CString s;
    GetDlgItemText(IDC_EDIT_BORDER_THICKNESS, s);

    int v = _ttoi(s);
    if (v <= 0)
        v = 2;
    return v;
}

int CGlimRequestDlg::HitTestPoint(CPoint pt) const
{
    if (m_points.empty())
        return -1;

    const int radius = GetPointRadius();
    const int hitR = max(radius + 4, 10);
    const int hitR2 = hitR * hitR;

    const int localX = pt.x - m_drawRect.left;
    const int localY = pt.y - m_drawRect.top;

    for (int i = 0; i < static_cast<int>(m_points.size()); ++i)
    {
        const int px = static_cast<int>(std::lround(m_points[i].x));
        const int py = static_cast<int>(std::lround(m_points[i].y));

        const int dx = localX - px;
        const int dy = localY - py;

        if (dx * dx + dy * dy <= hitR2)
            return i;
    }

    return -1;
}

PointF CGlimRequestDlg::ClientToCanvasPoint(CPoint pt) const
{
    PointF out{};
    out.x = static_cast<double>(pt.x - m_drawRect.left);
    out.y = static_cast<double>(pt.y - m_drawRect.top);
    return out;
}

bool CGlimRequestDlg::IsInsideDrawRect(CPoint pt) const
{
    return m_drawRect.PtInRect(pt) ? true : false;
}

void CGlimRequestDlg::UpdatePointLabels()
{
    CString s1, s2, s3, sc;

    if (m_points.size() >= 1)
        s1.Format(_T("Point 1 : (%.0f, %.0f)"), m_points[0].x, m_points[0].y);
    else
        s1 = _T("Point 1 : (-, -)");

    if (m_points.size() >= 2)
        s2.Format(_T("Point 2 : (%.0f, %.0f)"), m_points[1].x, m_points[1].y);
    else
        s2 = _T("Point 2 : (-, -)");

    if (m_points.size() >= 3)
        s3.Format(_T("Point 3 : (%.0f, %.0f)"), m_points[2].x, m_points[2].y);
    else
        s3 = _T("Point 3 : (-, -)");

    if (m_hasCircumcircle)
        sc.Format(_T("Center : (%.0f, %.0f)"), m_circleCenter.x, m_circleCenter.y);
    else
        sc = _T("Center : (-, -)");

    SetDlgItemText(IDC_STATIC_P1, s1);
    SetDlgItemText(IDC_STATIC_P2, s2);
    SetDlgItemText(IDC_STATIC_P3, s3);
    SetDlgItemText(IDC_STATIC_CENTER, sc);
}

void CGlimRequestDlg::ResetAll()
{
    StopRandomMoveThread();

    {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        m_points.clear();
        m_hasCircumcircle = false;
        m_circleCenter = {};
        m_circleRadius = 0.0;
        m_isDragging = false;
        m_dragIndex = -1;
    }

    UpdatePointLabels();
    RebuildScene();
}

void CGlimRequestDlg::StartRandomMoveThread()
{
    StopRandomMoveThread();

    {
        std::lock_guard<std::mutex> lock(m_dataMutex);
        if (m_points.size() != 3)
        {
            AfxMessageBox(_T("점 3개를 먼저 찍으세요."));
            return;
        }
    }

    m_randomRunning = true;

    m_randomThread = std::thread([this]()
        {
            std::random_device rd;
            std::mt19937 gen(rd());

            const int pointRadius = max(1, GetPointRadius());
            const int minX = pointRadius;
            const int minY = pointRadius;
            const int maxX = max(pointRadius, m_drawRect.Width() - pointRadius - 1);
            const int maxY = max(pointRadius, m_drawRect.Height() - pointRadius - 1);

            std::uniform_int_distribution<int> distX(minX, maxX);
            std::uniform_int_distribution<int> distY(minY, maxY);

            for (int iter = 0; iter < 10; ++iter)
            {
                if (!m_randomRunning || m_destroying)
                    break;

                {
                    std::lock_guard<std::mutex> lock(m_dataMutex);

                    if (m_points.size() == 3)
                    {
                        for (auto& pt : m_points)
                        {
                            pt.x = static_cast<double>(distX(gen));
                            pt.y = static_cast<double>(distY(gen));
                        }
                    }
                }

                if (!m_destroying)
                {
                    PostMessage(WM_APP_RANDOM_UPDATE, 0, 0);
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }

            m_randomRunning = false;
        });
}

void CGlimRequestDlg::StopRandomMoveThread()
{
    m_randomRunning = false;

    if (m_randomThread.joinable())
    {
        m_randomThread.join();
    }
}

void CGlimRequestDlg::OnPaint()
{
    CPaintDC dc(this);

    if (m_canvas.IsNull())
        return;

    m_canvas.Draw(dc.m_hDC, m_drawRect);
}

void CGlimRequestDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
    if (!IsInsideDrawRect(point))
    {
        CDialogEx::OnLButtonDown(nFlags, point);
        return;
    }

    if (m_randomRunning)
    {
        CDialogEx::OnLButtonDown(nFlags, point);
        return;
    }

    int hitIndex = HitTestPoint(point);
    if (hitIndex >= 0)
    {
        m_isDragging = true;
        m_dragIndex = hitIndex;
        SetCapture();
        return;
    }

    if (m_points.size() < 3)
    {
        PointF pt = ClientToCanvasPoint(point);

        const int radius = GetPointRadius();
        pt.x = static_cast<double>(
            ClampValue<int>(
                static_cast<int>(std::lround(pt.x)),
                radius,
                m_drawRect.Width() - radius - 1));

        pt.y = static_cast<double>(
            ClampValue<int>(
                static_cast<int>(std::lround(pt.y)),
                radius,
                m_drawRect.Height() - radius - 1));

        {
            std::lock_guard<std::mutex> lock(m_dataMutex);
            m_points.push_back(pt);
        }

        RebuildScene();
    }

    CDialogEx::OnLButtonDown(nFlags, point);
}

void CGlimRequestDlg::OnMouseMove(UINT nFlags, CPoint point)
{
    if (m_isDragging && m_dragIndex >= 0)
    {
        PointF pt = ClientToCanvasPoint(point);

        const int radius = GetPointRadius();
        pt.x = static_cast<double>(
            ClampValue<int>(
                static_cast<int>(std::lround(pt.x)),
                radius,
                m_drawRect.Width() - radius - 1));

        pt.y = static_cast<double>(
            ClampValue<int>(
                static_cast<int>(std::lround(pt.y)),
                radius,
                m_drawRect.Height() - radius - 1));

        {
            std::lock_guard<std::mutex> lock(m_dataMutex);
            if (m_dragIndex >= 0 && m_dragIndex < static_cast<int>(m_points.size()))
            {
                m_points[m_dragIndex] = pt;
            }
        }

        RebuildScene();
    }

    CDialogEx::OnMouseMove(nFlags, point);
}

void CGlimRequestDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
    UNREFERENCED_PARAMETER(point);

    if (m_isDragging)
    {
        m_isDragging = false;
        m_dragIndex = -1;
        ReleaseCapture();
    }

    CDialogEx::OnLButtonUp(nFlags, point);
}

void CGlimRequestDlg::OnBnClickedButtonReset()
{
    ResetAll();
}

void CGlimRequestDlg::OnBnClickedButtonRandom()
{
    StartRandomMoveThread();
}

LRESULT CGlimRequestDlg::OnRandomUpdate(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    if (!m_destroying)
    {
        RebuildScene();
    }

    return 0;
}
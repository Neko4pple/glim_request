#pragma once

#include <afxdialogex.h>
#include <atlimage.h>

#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <cmath>
#include <random>

constexpr UINT WM_APP_RANDOM_UPDATE = WM_APP + 100;
constexpr UINT_PTR TIMER_ID_INITIAL_DRAW = 1;

struct PointF
{
    double x = 0.0;
    double y = 0.0;
};

class CGlimRequestDlg : public CDialogEx
{
public:
    CGlimRequestDlg(CWnd* pParent = nullptr);
    virtual ~CGlimRequestDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_GLIM_REQUEST_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;

    DECLARE_MESSAGE_MAP()

private:
    // ------------------------------
    // 렌더링/도형 상태
    // ------------------------------
    CImage m_canvas;
    CRect  m_drawRect;

    std::vector<PointF> m_points;   // 최대 3개
    bool   m_hasCircumcircle = false;
    PointF m_circleCenter{};
    double m_circleRadius = 0.0;

    // 드래그 상태
    bool m_isDragging = false;
    int  m_dragIndex = -1;

    // 스레드 상태
    std::thread m_randomThread;
    std::atomic<bool> m_randomRunning{ false };
    std::atomic<bool> m_destroying{ false };
    std::mutex m_dataMutex;

private:
    // ------------------------------
    // 유틸
    // ------------------------------
    template<typename T>
    T ClampValue(T v, T lo, T hi)
    {
        if (v < lo) return lo;
        if (v > hi) return hi;
        return v;
    }

    int GetPointRadius() const;
    int GetCircleThickness() const;

    void InitCanvas();
    void ClearCanvas(COLORREF bgColor = RGB(245, 245, 245));
    void RebuildScene();

    void PutPixelSafe(int x, int y, COLORREF color);
    void DrawFilledCirclePixel(int cx, int cy, int radius, COLORREF color);
    void DrawCircleOutlinePixel(int cx, int cy, int radius, int thickness, COLORREF color);

    bool ComputeCircumcircle(const PointF& a, const PointF& b, const PointF& c, PointF& center, double& radius);
    void UpdateCircumcircle();
    int  HitTestPoint(CPoint pt) const;
    PointF ClientToCanvasPoint(CPoint pt) const;
    bool IsInsideDrawRect(CPoint pt) const;

    void UpdatePointLabels();
    void ResetAll();

    void StartRandomMoveThread();
    void StopRandomMoveThread();

private:
    // ------------------------------
    // 메시지 핸들러
    // ------------------------------
public:
    afx_msg void OnPaint();
    afx_msg void OnDestroy();
    afx_msg void OnTimer(UINT_PTR nIDEvent);

    afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
    afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
    afx_msg void OnMouseMove(UINT nFlags, CPoint point);

    afx_msg void OnBnClickedButtonReset();
    afx_msg void OnBnClickedButtonRandom();

    afx_msg LRESULT OnRandomUpdate(WPARAM wParam, LPARAM lParam);
};
#include "pch.h"
#include "framework.h"
#include "glim_request.h"
#include "glim_requestDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CGlimRequestApp, CWinApp)
END_MESSAGE_MAP()

CGlimRequestApp theApp;

CGlimRequestApp::CGlimRequestApp() noexcept
{
}

BOOL CGlimRequestApp::InitInstance()
{
    CWinApp::InitInstance();

    AfxEnableControlContainer();

    CGlimRequestDlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();

    return FALSE;
}
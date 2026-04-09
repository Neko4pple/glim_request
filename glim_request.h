#pragma once

#include "resource.h"
#include <afxwin.h>

class CGlimRequestApp : public CWinApp
{
public:
    CGlimRequestApp() noexcept;
    virtual BOOL InitInstance() override;

    DECLARE_MESSAGE_MAP()
};

extern CGlimRequestApp theApp;
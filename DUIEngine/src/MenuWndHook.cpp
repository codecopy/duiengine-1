/*########################################################################
	Filename: 	MenuWndHook.cpp
	----------------------------------------------------
	Remarks:	...
	----------------------------------------------------
	Author:		成真
	Email:		anyou@sina.com
				anyou@msn.com
	Created:	7/4/2003 17:38
				6/20/2012 黄建雄，修改为使用DUI皮肤绘制非客户区边框，支持边框大小配置
  ########################################################################*/

#include "duistd.h"
#include "MenuWndHook.h"
#include "duiskinpool.h"
#include "duiskin.h"

#ifdef _WIN64
#define GWL_WNDPROC	GWLP_WNDPROC
#endif//_WIN64

namespace DuiEngine
{

const TCHAR CoolMenu_oldProc[] = _T("CoolMenu_oldProc");


#define SM_CXMENUBORDER	3		//默认菜单边框大小


/*########################################################################
			  ------------------------------------------------
								class CMenuWndHook
			  ------------------------------------------------
  ########################################################################*/
CDuiMap <HWND,CMenuWndHook*> CMenuWndHook::m_WndMenuMap;

HHOOK CMenuWndHook::m_hMenuHook = NULL;
CDuiStringA CMenuWndHook::m_strSkinName = "";

CMenuWndHook::CMenuWndHook (HWND hWnd)
    : m_hWnd(hWnd)
{

}

CMenuWndHook::~CMenuWndHook ()
{
    WNDPROC oldWndProc = (WNDPROC)::GetProp(m_hWnd, CoolMenu_oldProc);
    if (oldWndProc != NULL)
    {
        ::SetWindowLong(m_hWnd, GWL_WNDPROC, (DWORD)(ULONG)oldWndProc);
        ::RemoveProp(m_hWnd, CoolMenu_oldProc);
    }
    m_WndMenuMap.RemoveKey(m_hWnd);
}

void CMenuWndHook::InstallHook(HINSTANCE hInst,LPCSTR pszSkinName)
{
    if (m_hMenuHook == NULL )
    {
        m_hMenuHook = ::SetWindowsHookEx(WH_CALLWNDPROC, WindowHook,
                                         hInst, ::GetCurrentThreadId());
        m_strSkinName=pszSkinName;
    }
}

void CMenuWndHook::UnInstallHook()
{
    POSITION pos= m_WndMenuMap.GetStartPosition();
    while(pos)
    {
        CDuiMap<HWND,CMenuWndHook*>::CPair *p=m_WndMenuMap.GetNext(pos);
        delete p->m_value;
    }
    m_WndMenuMap.RemoveAll();

    if (m_hMenuHook != NULL)
    {
        ::UnhookWindowsHookEx(m_hMenuHook);
    }
}

CMenuWndHook* CMenuWndHook::GetWndHook(HWND hwnd)
{

    CDuiMap<HWND,CMenuWndHook*>::CPair *p=m_WndMenuMap.Lookup(hwnd);
    if(!p) return NULL;
    return p->m_value;
}

CMenuWndHook* CMenuWndHook::AddWndHook(HWND hwnd)
{
    CMenuWndHook* pWnd = GetWndHook(hwnd);
    if(pWnd) return pWnd;

    pWnd = new CMenuWndHook(hwnd);
    if (pWnd != NULL)
    {
        m_WndMenuMap[hwnd]=pWnd;
    }
    return pWnd;
}

/*########################################################################
			  ------------------------------------------------
								  消息过程
			  ------------------------------------------------
  ########################################################################*/
LRESULT CALLBACK CMenuWndHook::WindowHook(int code, WPARAM wParam, LPARAM lParam)
{
    CWPSTRUCT* pStruct = (CWPSTRUCT*)lParam;

    while (code == HC_ACTION)
    {
        HWND hWnd = pStruct->hwnd;

        if (pStruct->message != WM_CREATE && pStruct->message != 0x01E2)
        {
            break;
        }

        // 是否为菜单类 ----------------------------------------
        TCHAR strClassName[10];
        int Count = ::GetClassName (hWnd, strClassName, sizeof(strClassName) / sizeof(strClassName[0]));
        if (Count != 6 || _tcscmp(strClassName, _T("#32768")) != 0 )
        {
            break;
        }

        // 是否已经被子类化 ------------------------------------
        if (::GetProp(hWnd, CoolMenu_oldProc) != NULL )
        {
            break;
        }

        AddWndHook(pStruct->hwnd);

        // 取得原来的窗口过程 ----------------------------------
        WNDPROC oldWndProc = (WNDPROC)(long)::GetWindowLong(hWnd, GWL_WNDPROC);
        if (oldWndProc == NULL)
        {
            break;
        }

        DUIASSERT(oldWndProc != CoolMenuProc);
        // 保存到窗口的属性中 ----------------------------------
        if (!SetProp(hWnd, CoolMenu_oldProc, oldWndProc) )
        {
            break;
        }

        // 子类化 ----------------------------------------------
        if (!SetWindowLong(hWnd, GWL_WNDPROC,(DWORD)(ULONG)CoolMenuProc) )
        {
            ::RemoveProp(hWnd, CoolMenu_oldProc);
            break;
        }

        break;
    }
    return CallNextHookEx (m_hMenuHook, code, wParam, lParam);
}

LRESULT CALLBACK CMenuWndHook::CoolMenuProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    WNDPROC oldWndProc = (WNDPROC)::GetProp(hWnd, CoolMenu_oldProc);
    CMenuWndHook* pWnd = NULL;

    switch (uMsg)
    {
    case WM_CREATE:
    {
        LRESULT lResult = CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
        if ((pWnd = GetWndHook(hWnd)) != NULL)
        {
            lResult = (LRESULT)pWnd->OnCreate((LPCREATESTRUCT)lParam);
        }
        return lResult;
    }
    break;
    case WM_NCCALCSIZE:
    {
        LRESULT lResult = CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
        if ((pWnd = GetWndHook(hWnd)) != NULL)
        {
            pWnd->OnNcCalcsize((BOOL)wParam,(NCCALCSIZE_PARAMS*)lParam);
        }
        return lResult;
    }
    break;
    case WM_WINDOWPOSCHANGING:
    {
        if ((pWnd = GetWndHook(hWnd)) != NULL)
        {
            pWnd->OnWindowPosChanging((LPWINDOWPOS)lParam);
        }
    }
    break;
// 		case WM_WINDOWPOSCHANGED:
// 			{
// 				LRESULT lResult = CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
// 				if ((pWnd = GetWndHook(hWnd)) != NULL)
// 				{
//  					pWnd->SetLayeredAttr();
// 				}
// 				return lResult;
// 			}
// 			break;
    case WM_PRINT:
    {
        LRESULT lResult = CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
        if ((pWnd = GetWndHook(hWnd)) != NULL)
        {
            pWnd->OnPrint((HDC)wParam);
        }
        return lResult;
    }
    break;
    case WM_NCPAINT:
    {
        if ((pWnd = GetWndHook(hWnd)) != NULL)
        {
            pWnd->OnNcPaint();
            return 0;
        }
    }
    break;
    case WM_NCDESTROY:
    {
        if ((pWnd = GetWndHook(hWnd)) != NULL)
        {
            pWnd->OnNcDestroy();
        }
    }
    break;
    }
    return CallWindowProc(oldWndProc, hWnd, uMsg, wParam, lParam);
}

/*########################################################################
			  ------------------------------------------------
								消息处理函数
			  ------------------------------------------------
  ########################################################################*/
int CMenuWndHook::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    SetClassLong(m_hWnd, GCL_STYLE, GetClassLong(m_hWnd, GCL_STYLE) & ~CS_DROPSHADOW);
    return 0;
}

void CMenuWndHook::OnWindowPosChanging(WINDOWPOS *pWindowPos)
{
    if(m_strSkinName.IsEmpty()) return;
    CDuiSkinBase *pSkin=DuiSkinPool::getSingleton().GetSkin(m_strSkinName);
    if(!pSkin) return;
    CDuiSkinMenuBorder *pBorderSkin=static_cast<CDuiSkinMenuBorder*>(pSkin);
    if(!pBorderSkin) return;
    pWindowPos->cx += pBorderSkin->GetMarginRect().left+pBorderSkin->GetMarginRect().right-SM_CXMENUBORDER*2;
    pWindowPos->cy -= pBorderSkin->GetMarginRect().top+pBorderSkin->GetMarginRect().bottom-SM_CXMENUBORDER*2;
}

void CMenuWndHook::OnNcCalcsize(BOOL bValidCalc,NCCALCSIZE_PARAMS* lpncsp)
{
    if(m_strSkinName.IsEmpty()) return;
    CDuiSkinBase *pSkin=DuiSkinPool::getSingleton().GetSkin(m_strSkinName);
    if(!pSkin) return;
    CDuiSkinMenuBorder *pBorderSkin=static_cast<CDuiSkinMenuBorder*>(pSkin);
    if(!pBorderSkin) return;

    lpncsp->rgrc[0].left=lpncsp->lppos->x+pBorderSkin->GetMarginRect().left;
    lpncsp->rgrc[0].top=lpncsp->lppos->y+pBorderSkin->GetMarginRect().top;
    lpncsp->rgrc[0].right=lpncsp->lppos->x+lpncsp->lppos->cx-pBorderSkin->GetMarginRect().right;
    lpncsp->rgrc[0].bottom=lpncsp->lppos->y+lpncsp->lppos->cy-pBorderSkin->GetMarginRect().bottom;
}

void CMenuWndHook::OnNcPaint()
{
    CDCHandle dc=GetWindowDC(m_hWnd);
    OnPrint(dc);
    ReleaseDC(m_hWnd,dc);
}

void CMenuWndHook::OnPrint(CDCHandle dc)
{
    if(m_strSkinName.IsEmpty()) return;
    CDuiSkinBase *pSkin=DuiSkinPool::getSingleton().GetSkin(m_strSkinName);
    if(!pSkin) return;
    CDuiSkinMenuBorder *pBorderSkin=static_cast<CDuiSkinMenuBorder*>(pSkin);
    if(!pBorderSkin) return;

    CRect rcClient;
    GetClientRect(m_hWnd,&rcClient);
    ClientToScreen(m_hWnd,(LPPOINT)&rcClient);
    ClientToScreen(m_hWnd,((LPPOINT)&rcClient)+1);
    CRect rcWnd;
    GetWindowRect(m_hWnd, &rcWnd);
    rcClient.OffsetRect(-rcWnd.TopLeft());
    dc.ExcludeClipRect(rcClient);
    rcWnd.MoveToXY(0,0);

    pBorderSkin->Draw(dc,rcWnd,0,0xFF);
}

void CMenuWndHook::OnNcDestroy()
{
    delete this;
}

//不能设计窗口半透明，设置区域后，非客户区位置发生改变，不明白原因。
void CMenuWndHook::SetLayeredAttr()
{
    CRect rcWnd;
    GetWindowRect(m_hWnd,&rcWnd);
    rcWnd.MoveToXY(0,0);
    CRgn rgn;
    rgn.CreateEllipticRgnIndirect(&rcWnd);
    //rgn.CreateRoundRectRgn(rcWnd.left,rcWnd.top,rcWnd.right,rcWnd.bottom,5,5);
    SetWindowRgn(m_hWnd,rgn,TRUE);
}

}//namespace DuiEngine
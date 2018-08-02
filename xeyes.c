#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "resource.h"

#define ID_TIMER 1

#define LEFT_EYE 0
#define RIGHT_EYE 1
#define MAX_EYES 2

struct Globals {
    BOOL ResetClippingRegion;
    POINT EyeCenter[MAX_EYES];
    POINT EyeSize;
    POINT EyeBallSize;
    RECT  PreviousLocation[MAX_EYES];
} g =
{
    TRUE,
{ { 0, 0 }, { 0, 0 } },
{ 0, 0 },
{ 0, 0 },
{ { 0, 0, 0, 0 }, { 0, 0, 0, 0 } } };

//HINSTANCE g_hinst = NULL;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK AboutDlgProc(HWND, UINT, WPARAM, LPARAM);

BOOL LoadWindowPosition(LPPOINT pt);
void SaveWindowPosition(HWND hwnd);

void UpdateEyes(HWND hwnd, BOOL force);
void PaintEyes(HWND hwnd);
void UpdateClippingRegion(HWND hwnd);

int APIENTRY wWinMain(HINSTANCE hinst, HINSTANCE hprevinst, LPWSTR lpcmdline, int cmdshow)
{
    WNDCLASSEX wc = { 0 };
    POINT pt = { CW_USEDEFAULT, CW_USEDEFAULT };
    RECT rect = { 0 };
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
    HWND hwnd = NULL;
    MSG msg = { 0 };

    UNREFERENCED_PARAMETER(hprevinst);
    UNREFERENCED_PARAMETER(lpcmdline);

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hinst;
    wc.hIcon = LoadIcon(hinst, MAKEINTRESOURCE(IDI_XEYES));
    wc.hIconSm = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = L"Xeyes";

    RegisterClassEx(&wc);

    LoadWindowPosition(&pt);

    rect.left = pt.x;
    rect.top = pt.y;
    rect.right = pt.x + 150;
    rect.bottom = pt.y + 100;
    AdjustWindowRectEx(&rect, style, FALSE, WS_EX_TOOLWINDOW);
    hwnd = CreateWindowEx(WS_EX_TOOLWINDOW, L"Xeyes", L"Xeyes", style, pt.x, pt.y, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, hinst, NULL);
    if (hwnd == NULL)
    {
        return 0;
    }

    if (!SetTimer(hwnd, ID_TIMER, 50, NULL))
    {
        return 0;
    }

    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

    ShowWindow(hwnd, cmdshow);
    UpdateWindow(hwnd);

    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, NULL, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    static HINSTANCE hinst = NULL;
    HMENU hmenu = NULL;
    static POINT DragPosition = { 0, 0 };
    static BOOL WindowIsMoving = FALSE;
    static POINT MousePosition = { 0, 0 };
    POINT NewMousePosition = { 0, 0 };
    RECT rect = { 0, 0, 0, 0 };

    switch (message)
    {
    case WM_CREATE:
        hinst = GetModuleHandle(NULL);
        hmenu = GetSystemMenu(hwnd, FALSE);

        DeleteMenu(hmenu, SC_RESTORE, MF_BYCOMMAND);
        DeleteMenu(hmenu, SC_MINIMIZE, MF_BYCOMMAND);
        DeleteMenu(hmenu, SC_MAXIMIZE, MF_BYCOMMAND);

        //InsertMenu(hmenu, 2, MF_STRING | MF_BYPOSITION, ID_DEFAULT_SIZE, "&Default Size");
        //InsertMenu(hmenu, 3, MF_STRING | MF_BYPOSITION, ID_ALWAYS_ON_TOP, "&Always on Top");
        AppendMenu(hmenu, MF_SEPARATOR, (INT_PTR)NULL, NULL);
        AppendMenu(hmenu, MF_STRING, IDM_ABOUT, L"A&bout Xeyes...");

        break;

    case WM_LBUTTONDOWN:
        WindowIsMoving = TRUE;
        SetCapture(hwnd);
        DragPosition.x = LOWORD(lparam);
        DragPosition.y = HIWORD(lparam);
        break;

    case WM_LBUTTONUP:
        if (TRUE == WindowIsMoving)
        {
            WindowIsMoving = FALSE;
            ReleaseCapture();
        }

        break;

    case WM_MOUSEMOVE:
        if (TRUE == WindowIsMoving)
        {
            GetWindowRect(hwnd, &rect);

            NewMousePosition.x = rect.left + LOWORD(lparam);
            NewMousePosition.y = rect.top + HIWORD(lparam);
            if ((NewMousePosition.x != MousePosition.x) || (NewMousePosition.y != MousePosition.y))
            {
                MousePosition.x = NewMousePosition.x;
                MousePosition.y = NewMousePosition.y;
                MoveWindow(hwnd, rect.left + LOWORD(lparam) - DragPosition.x, rect.top + HIWORD(lparam) - DragPosition.x, rect.right - rect.left, rect.bottom - rect.top, TRUE);
            }
        }
        else
        {
            return DefWindowProc(hwnd, message, wparam, lparam);
        }

        break;


    case WM_SYSCOMMAND:
        switch (wparam)
        {
        case IDM_ABOUT:
            DialogBox(hinst, MAKEINTRESOURCE(IDD_ABOUT), hwnd, AboutDlgProc);
            break;

        default:
            return DefWindowProc(hwnd, message, wparam, lparam);
        }

        break;

    case WM_COMMAND:
    {
        switch (LOWORD(wparam))
        {
        case IDM_ABOUT:
            DialogBox(hinst, MAKEINTRESOURCE(IDD_ABOUT), hwnd, AboutDlgProc);
            break;

        default:
            return DefWindowProc(hwnd, message, wparam, lparam);
        }

        break;
    }

    case WM_MOVE:
    case WM_PAINT:
        PaintEyes(hwnd);
        break;

    case WM_SIZE:
        g.ResetClippingRegion = TRUE;
        RedrawWindow(hwnd, NULL, NULL, RDW_ERASE | RDW_FRAME | RDW_INVALIDATE);
        break;

    case WM_TIMER:
        UpdateEyes(hwnd, FALSE);
        break;

    case WM_DESTROY:
        KillTimer(hwnd, ID_TIMER);
        SaveWindowPosition(hwnd);
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, message, wparam, lparam);
    }

    return 0;
}

INT_PTR CALLBACK AboutDlgProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    UNREFERENCED_PARAMETER(lparam);

    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wparam) == IDOK || LOWORD(wparam) == IDCANCEL)
        {
            EndDialog(hwnd, LOWORD(wparam));
            return (INT_PTR)TRUE;
        }

        break;
    }

    return (INT_PTR)FALSE;
}

void UpdateEyes(HWND hwnd, BOOL force)
{
    RECT  forEllipse1, forEllipse2;
    POINT newmouseloc, current1, current2, relmouse[2];
    POINT WindowOrigin;
    HDC   hdc;
    double len;
    double eyecos[2], eyesin[2];
    static POINT mouseloc;

    GetCursorPos((LPPOINT)&newmouseloc);
    if ((mouseloc.x == newmouseloc.x) && (mouseloc.y == newmouseloc.y) && (!force))
        return;

    mouseloc.x = newmouseloc.x, mouseloc.y = newmouseloc.y;

    hdc = GetDC(hwnd);
    GetDCOrgEx(hdc, &WindowOrigin);


    relmouse[LEFT_EYE].x = mouseloc.x - WindowOrigin.x - g.EyeCenter[LEFT_EYE].x;
    relmouse[LEFT_EYE].y = mouseloc.y - WindowOrigin.y - g.EyeCenter[LEFT_EYE].y;
    relmouse[RIGHT_EYE].x = mouseloc.x - WindowOrigin.x - g.EyeCenter[RIGHT_EYE].x;
    relmouse[RIGHT_EYE].y = mouseloc.y - WindowOrigin.y - g.EyeCenter[RIGHT_EYE].y;

    if ((relmouse[LEFT_EYE].x != 0) || (relmouse[LEFT_EYE].y != 0)) {
        len = sqrt((double)(relmouse[LEFT_EYE].x*relmouse[LEFT_EYE].x + relmouse[LEFT_EYE].y * relmouse[LEFT_EYE].y));
        eyecos[LEFT_EYE] = relmouse[LEFT_EYE].x / len;
        eyesin[LEFT_EYE] = relmouse[LEFT_EYE].y / len;
    }
    else {
        eyecos[LEFT_EYE] = 0; eyesin[LEFT_EYE] = 0;
    }
    if ((relmouse[RIGHT_EYE].x != 0) || (relmouse[RIGHT_EYE].y != 0)) {
        len = sqrt((double)(relmouse[RIGHT_EYE].x*relmouse[RIGHT_EYE].x + relmouse[RIGHT_EYE].y * relmouse[RIGHT_EYE].y));
        eyecos[RIGHT_EYE] = relmouse[RIGHT_EYE].x / len;
        eyesin[RIGHT_EYE] = relmouse[RIGHT_EYE].y / len;
    }
    else {
        eyecos[RIGHT_EYE] = 0; eyesin[RIGHT_EYE] = 0;
    }

    current1.x = (int)(eyecos[LEFT_EYE] * g.EyeSize.x); current1.y = (int)(eyesin[LEFT_EYE] * g.EyeSize.y);
    current2.x = (int)(eyecos[RIGHT_EYE] * g.EyeSize.x); current2.y = (int)(eyesin[RIGHT_EYE] * g.EyeSize.y);
    if (current1.x * current1.x + current1.y * current1.y >
        relmouse[LEFT_EYE].x * relmouse[LEFT_EYE].x + relmouse[LEFT_EYE].y * relmouse[LEFT_EYE].y) {
        current1.x = relmouse[LEFT_EYE].x;
        current1.y = relmouse[LEFT_EYE].y;
    }
    if (current2.x * current2.x + current2.y * current2.y >
        relmouse[RIGHT_EYE].x * relmouse[RIGHT_EYE].x + relmouse[RIGHT_EYE].y * relmouse[RIGHT_EYE].y) {
        current2.x = relmouse[RIGHT_EYE].x;
        current2.y = relmouse[RIGHT_EYE].y;
    }
    current1.x += g.EyeCenter[LEFT_EYE].x;
    current1.y += g.EyeCenter[LEFT_EYE].y;
    current2.x += g.EyeCenter[RIGHT_EYE].x;
    current2.y += g.EyeCenter[RIGHT_EYE].y;

    forEllipse1.left = current1.x - g.EyeBallSize.x, forEllipse1.top = current1.y - g.EyeBallSize.y;
    forEllipse1.right = current1.x + g.EyeBallSize.x, forEllipse1.bottom = current1.y + g.EyeBallSize.y;

    forEllipse2.left = current2.x - g.EyeBallSize.x, forEllipse2.top = current2.y - g.EyeBallSize.y;
    forEllipse2.right = current2.x + g.EyeBallSize.x, forEllipse2.bottom = current2.y + g.EyeBallSize.y;

    if (g.PreviousLocation[LEFT_EYE].left < g.PreviousLocation[LEFT_EYE].right)
    {
        SelectObject(hdc, GetStockObject(WHITE_PEN));
        SelectObject(hdc, GetStockObject(WHITE_BRUSH));
        Ellipse(hdc, g.PreviousLocation[LEFT_EYE].left, g.PreviousLocation[LEFT_EYE].top, g.PreviousLocation[LEFT_EYE].right, g.PreviousLocation[LEFT_EYE].bottom);
        Ellipse(hdc, g.PreviousLocation[RIGHT_EYE].left, g.PreviousLocation[RIGHT_EYE].top, g.PreviousLocation[RIGHT_EYE].right, g.PreviousLocation[RIGHT_EYE].bottom);
    }

    SelectObject(hdc, GetStockObject(BLACK_BRUSH));
    SelectObject(hdc, GetStockObject(BLACK_PEN));
    Ellipse(hdc, forEllipse1.left, forEllipse1.top, forEllipse1.right, forEllipse1.bottom);
    Ellipse(hdc, forEllipse2.left, forEllipse2.top, forEllipse2.right, forEllipse2.bottom);

    g.PreviousLocation[LEFT_EYE].left = forEllipse1.left, g.PreviousLocation[LEFT_EYE].top = forEllipse1.top;
    g.PreviousLocation[LEFT_EYE].right = forEllipse1.right, g.PreviousLocation[LEFT_EYE].bottom = forEllipse1.bottom;
    g.PreviousLocation[RIGHT_EYE].left = forEllipse2.left, g.PreviousLocation[RIGHT_EYE].top = forEllipse2.top;
    g.PreviousLocation[RIGHT_EYE].right = forEllipse2.right, g.PreviousLocation[RIGHT_EYE].bottom = forEllipse2.bottom;

    ReleaseDC(hwnd, hdc);
}

void PaintEyes(HWND hwnd)
{
    PAINTSTRUCT ps;
    POINT size;
    RECT  forEllipse1, forEllipse2, rect;

    if (g.ResetClippingRegion) {
        UpdateClippingRegion(hwnd);
        g.ResetClippingRegion = 0;
    }
    GetClientRect(hwnd, &rect);
    size.x = rect.right - rect.left;
    size.y = rect.bottom - rect.top;
    forEllipse1.left = rect.left + 1;
    forEllipse1.right = (long)(size.x / 2 - size.x*0.025);
    forEllipse1.top = rect.top + 1;
    forEllipse1.bottom = size.y;
    forEllipse2.right = rect.right - 1;
    forEllipse2.left = (long)(forEllipse1.right + size.x*0.05);
    forEllipse2.top = rect.top + 1;
    forEllipse2.bottom = size.y;

    BeginPaint(hwnd, (LPPAINTSTRUCT)&ps);


    SelectObject(ps.hdc, GetStockObject(BLACK_BRUSH));
    SelectObject(ps.hdc, GetStockObject(BLACK_PEN));
    Ellipse(ps.hdc, forEllipse1.left, forEllipse1.top, forEllipse1.right, forEllipse1.bottom);
    Ellipse(ps.hdc, forEllipse2.left, forEllipse2.top, forEllipse2.right, forEllipse2.bottom);

    forEllipse1.left += size.x / 2 / 10; 	forEllipse1.right -= size.x / 2 / 10;
    forEllipse1.top += size.y / 10;    forEllipse1.bottom -= size.y / 10;
    forEllipse2.left += size.x / 2 / 10; 	forEllipse2.right -= size.x / 2 / 10;
    forEllipse2.top += size.y / 10;    forEllipse2.bottom -= size.y / 10;

    SelectObject(ps.hdc, GetStockObject(WHITE_BRUSH));
    SelectObject(ps.hdc, GetStockObject(WHITE_PEN));
    Ellipse(ps.hdc, forEllipse1.left, forEllipse1.top, forEllipse1.right, forEllipse1.bottom);
    Ellipse(ps.hdc, forEllipse2.left, forEllipse2.top, forEllipse2.right, forEllipse2.bottom);

    g.EyeCenter[LEFT_EYE].x = (forEllipse1.right + forEllipse1.left) / 2 + 1;
    g.EyeCenter[LEFT_EYE].y = (forEllipse1.top + forEllipse1.bottom) / 2 + 1;

    g.EyeCenter[RIGHT_EYE].x = (forEllipse2.right + forEllipse2.left) / 2 + 1;
    g.EyeCenter[RIGHT_EYE].y = g.EyeCenter[LEFT_EYE].y;

    g.EyeSize.x = (long)((forEllipse1.right - forEllipse1.left) / 2 / 1.7);
    g.EyeSize.y = (long)((forEllipse1.bottom - forEllipse1.top) / 2 / 1.7);
    g.EyeBallSize.x = (long)(g.EyeSize.x / 2.5);
    g.EyeBallSize.y = (long)(g.EyeSize.y / 2.5);
    UpdateEyes(hwnd, TRUE);
    EndPaint(hwnd, (LPPAINTSTRUCT)&ps);
}

void UpdateClippingRegion(HWND hwnd)
{
    POINT size;
    RECT forEllipse1, forEllipse2, winrect, rect;
    HRGN leye, reye;
    int loff, toff;
    POINT client_origin;

    // Get window and client rects
    GetWindowRect(hwnd, &winrect);
    GetClientRect(hwnd, &rect);

    // Get the client origin in window coordinates
    client_origin.x = 0; client_origin.y = 0;
    ClientToScreen(hwnd, &client_origin);
    client_origin.x -= winrect.left;
    client_origin.y -= winrect.top;

    size.x = rect.right - rect.left;
    size.y = rect.bottom - rect.top;
    forEllipse1.left = rect.left + 1;
    forEllipse1.right = (long)(size.x / 2 - size.x*0.025 + 1);
    forEllipse1.top = rect.top + 1;
    forEllipse1.bottom = size.y;
    forEllipse2.right = rect.right - 1;
    forEllipse2.left = (long)(forEllipse1.right + size.x*0.05) - 1;
    forEllipse2.top = rect.top + 1;
    forEllipse2.bottom = size.y;
    loff = client_origin.x;
    toff = client_origin.y;
    leye = CreateEllipticRgn(forEllipse1.left + loff, forEllipse1.top + toff,
        forEllipse1.right + loff, forEllipse1.bottom + toff);
    reye = CreateEllipticRgn(forEllipse2.left + loff, forEllipse2.top + toff,
        forEllipse2.right + loff, forEllipse2.bottom + toff);
    CombineRgn(leye, leye, reye, RGN_OR);
    SetWindowRgn(hwnd, leye, 1);
}

BOOL LoadWindowPosition(LPPOINT pt)
{
    BOOL result = FALSE;
    HKEY hkey = NULL;
    LSTATUS rc = ERROR_SUCCESS;
    DWORD ValueType = REG_NONE;
    DWORD ValueSize = sizeof(LONG);

    if (pt != NULL)
    {
        rc = RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\Xeyes", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hkey, NULL);
        if (ERROR_SUCCESS == rc)
        {
            rc = RegQueryValueEx(hkey, L"x", NULL, &ValueType, (LPBYTE)&pt->x, &ValueSize);
            if ((ERROR_SUCCESS == rc) && (REG_DWORD == ValueType) && (REG_DWORD == ValueSize))
            {
                ValueType = REG_NONE;
                ValueSize = sizeof(LONG);
                rc = RegQueryValueEx(hkey, L"y", NULL, &ValueType, (LPBYTE)&pt->y, &ValueSize);
                if ((ERROR_SUCCESS == rc) && (REG_DWORD == ValueType) && (REG_DWORD == ValueSize))
                {
                    result = TRUE;
                }
                else
                {
                    pt->x = CW_USEDEFAULT;
                    pt->y = CW_USEDEFAULT;
                    result = FALSE;
                }
            }

            RegCloseKey(hkey);
        }
    }

    return result;
}

void SaveWindowPosition(HWND hwnd)
{
    RECT rect = { 0, 0 };
    HKEY hkey = NULL;
    LSTATUS rc = ERROR_SUCCESS;

    if (GetWindowRect(hwnd, &rect) != FALSE)
    {
        rc = RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\Xeyes", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, NULL);
        if (ERROR_SUCCESS == rc)
        {
            rc = RegSetValueEx(hkey, L"x", 0, REG_DWORD, (CONST BYTE*)&rect.left, sizeof(LONG));
            if (ERROR_SUCCESS == rc)
            {
                rc = RegSetValueEx(hkey, L"y", 0, REG_DWORD, (CONST BYTE*)&rect.top, sizeof(LONG));
                if (ERROR_SUCCESS != rc)
                {
                    RegDeleteValue(hkey, L"x");
                }
            }

            RegCloseKey(hkey);
        }
    }
}

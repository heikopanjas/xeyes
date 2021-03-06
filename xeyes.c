#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "resource.h"

#define ID_TIMER 1

#define LEFT_EYE 0
#define RIGHT_EYE 1
#define MAX_EYES 2

#define APP_NAME L"Xeyes"

struct _GVARS {
    BOOL resetClippingRegion;
    POINT eyeCenter[MAX_EYES];
    POINT eyeSize;
    POINT eyeballSize;
    RECT  previousLocation[MAX_EYES];
}
g = { TRUE, { { 0 }, { 0 } }, { 0 }, { 0 }, { { 0 }, { 0 } } };

void InitApplication(HINSTANCE hinst);
BOOL InitInstance(HINSTANCE hinst, int showCmd);

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK AboutDlgProc(HWND, UINT, WPARAM, LPARAM);

BOOL LoadWindowPosition(LPPOINT pt);
void SaveWindowPosition(HWND hwnd);

void UpdateEyes(HWND hwnd, BOOL force);
void PaintEyes(HWND hwnd);
void UpdateClippingRegion(HWND hwnd);

int APIENTRY wWinMain(HINSTANCE hinst, HINSTANCE hprevinst, LPWSTR cmdLine, int showCmd)
{
    MSG msg = { 0 };

    UNREFERENCED_PARAMETER(hprevinst);
    UNREFERENCED_PARAMETER(cmdLine);

    InitApplication(hinst);
    if (!InitInstance(hinst, showCmd))
    {
        return 0;
    }

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

void InitApplication(HINSTANCE hinst)
{
    WNDCLASSEX wc = { 0 };

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
    wc.lpszClassName = APP_NAME;

    RegisterClassEx(&wc);
}

BOOL InitInstance(HINSTANCE hinst, int showCmd)
{
    BOOL result = FALSE;
    POINT windowPosition = { CW_USEDEFAULT, CW_USEDEFAULT };
    RECT windowDimensions = { 0 };
    DWORD windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME;
    HWND hwnd = NULL;

    LoadWindowPosition(&windowPosition);

    windowDimensions.left = windowPosition.x;
    windowDimensions.top = windowPosition.y;
    windowDimensions.right = windowPosition.x + 150;
    windowDimensions.bottom = windowPosition.y + 100;
    AdjustWindowRectEx(&windowDimensions, windowStyle, FALSE, WS_EX_TOOLWINDOW);
    hwnd = CreateWindowEx(WS_EX_TOOLWINDOW, APP_NAME, APP_NAME, windowStyle, windowPosition.x, windowPosition.y,
        windowDimensions.right - windowDimensions.left, windowDimensions.bottom - windowDimensions.top, NULL, NULL, hinst, NULL);
    if (hwnd)
    {
        if (SetTimer(hwnd, ID_TIMER, 50, NULL))
        {
            SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            ShowWindow(hwnd, showCmd);
            UpdateWindow(hwnd);

            result = TRUE;
        }
    }

    return result;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    static HINSTANCE hinst = NULL;
    HMENU hmenu = NULL;
    static POINT dragPosition = { 0 };
    static BOOL windowIsMoving = FALSE;
    static POINT mousePosition = { 0 };
    POINT newMousePosition = { 0 };
    RECT rect = { 0 };

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
        AppendMenu(hmenu, MF_STRING, IDM_ABOUT, L"A&bout " APP_NAME L"...");

        break;

    case WM_LBUTTONDOWN:
        windowIsMoving = TRUE;
        SetCapture(hwnd);
        dragPosition.x = LOWORD(lparam);
        dragPosition.y = HIWORD(lparam);
        break;

    case WM_LBUTTONUP:
        if (windowIsMoving)
        {
            windowIsMoving = FALSE;
            ReleaseCapture();
        }

        break;

    case WM_MOUSEMOVE:
        if (windowIsMoving)
        {
            GetWindowRect(hwnd, &rect);

            newMousePosition.x = rect.left + LOWORD(lparam);
            newMousePosition.y = rect.top + HIWORD(lparam);
            if ((newMousePosition.x != mousePosition.x) || (newMousePosition.y != mousePosition.y))
            {
                mousePosition.x = newMousePosition.x;
                mousePosition.y = newMousePosition.y;
                MoveWindow(hwnd, rect.left + LOWORD(lparam) - dragPosition.x,
                    rect.top + HIWORD(lparam) - dragPosition.x,
                    rect.right - rect.left, rect.bottom - rect.top, TRUE);
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
        g.resetClippingRegion = TRUE;
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
    RECT leftRectangle = { 0 };
    RECT rightRectangle = { 0 };
    POINT newPointerPosition = { 0 };
    POINT currentPosition[MAX_EYES] = { { 0 }, { 0 } };
    POINT relativePointerPosition[MAX_EYES] = { { 0 }, { 0 } };
    POINT windowOrigin = { 0 };
    HDC   hdc;
    double length = 0;
    double eyeCosine[MAX_EYES] = { 0 };
    double eyeSine[MAX_EYES] = { 0 };
    static POINT mousePosition = { 0 };


    GetCursorPos((LPPOINT)&newPointerPosition);
    if ((mousePosition.x == newPointerPosition.x) && (mousePosition.y == newPointerPosition.y) && (!force))
    {
        return;
    }

    mousePosition.x = newPointerPosition.x, mousePosition.y = newPointerPosition.y;

    hdc = GetDC(hwnd);
    GetDCOrgEx(hdc, &windowOrigin);


    relativePointerPosition[LEFT_EYE].x = mousePosition.x - windowOrigin.x - g.eyeCenter[LEFT_EYE].x;
    relativePointerPosition[LEFT_EYE].y = mousePosition.y - windowOrigin.y - g.eyeCenter[LEFT_EYE].y;
    relativePointerPosition[RIGHT_EYE].x = mousePosition.x - windowOrigin.x - g.eyeCenter[RIGHT_EYE].x;
    relativePointerPosition[RIGHT_EYE].y = mousePosition.y - windowOrigin.y - g.eyeCenter[RIGHT_EYE].y;

    if ((relativePointerPosition[LEFT_EYE].x != 0) || (relativePointerPosition[LEFT_EYE].y != 0))
    {
        length = sqrt((double)(relativePointerPosition[LEFT_EYE].x * relativePointerPosition[LEFT_EYE].x + relativePointerPosition[LEFT_EYE].y * relativePointerPosition[LEFT_EYE].y));
        eyeCosine[LEFT_EYE] = relativePointerPosition[LEFT_EYE].x / length;
        eyeSine[LEFT_EYE] = relativePointerPosition[LEFT_EYE].y / length;
    }
    else
    {
        eyeCosine[LEFT_EYE] = 0;
        eyeSine[LEFT_EYE] = 0;
    }

    if ((relativePointerPosition[RIGHT_EYE].x != 0) || (relativePointerPosition[RIGHT_EYE].y != 0))
    {
        length = sqrt((double)(relativePointerPosition[RIGHT_EYE].x*relativePointerPosition[RIGHT_EYE].x + relativePointerPosition[RIGHT_EYE].y * relativePointerPosition[RIGHT_EYE].y));
        eyeCosine[RIGHT_EYE] = relativePointerPosition[RIGHT_EYE].x / length;
        eyeSine[RIGHT_EYE] = relativePointerPosition[RIGHT_EYE].y / length;
    }
    else
    {
        eyeCosine[RIGHT_EYE] = 0;
        eyeSine[RIGHT_EYE] = 0;
    }

    currentPosition[LEFT_EYE].x = (int)(eyeCosine[LEFT_EYE] * g.eyeSize.x);
    currentPosition[LEFT_EYE].y = (int)(eyeSine[LEFT_EYE] * g.eyeSize.y);
    currentPosition[RIGHT_EYE].x = (int)(eyeCosine[RIGHT_EYE] * g.eyeSize.x);
    currentPosition[RIGHT_EYE].y = (int)(eyeSine[RIGHT_EYE] * g.eyeSize.y);

    if (currentPosition[LEFT_EYE].x * currentPosition[LEFT_EYE].x + currentPosition[LEFT_EYE].y * currentPosition[LEFT_EYE].y >
        relativePointerPosition[LEFT_EYE].x * relativePointerPosition[LEFT_EYE].x + relativePointerPosition[LEFT_EYE].y * relativePointerPosition[LEFT_EYE].y)
    {
        currentPosition[LEFT_EYE].x = relativePointerPosition[LEFT_EYE].x;
        currentPosition[LEFT_EYE].y = relativePointerPosition[LEFT_EYE].y;
    }

    if (currentPosition[RIGHT_EYE].x * currentPosition[RIGHT_EYE].x + currentPosition[RIGHT_EYE].y * currentPosition[RIGHT_EYE].y >
        relativePointerPosition[RIGHT_EYE].x * relativePointerPosition[RIGHT_EYE].x + relativePointerPosition[RIGHT_EYE].y * relativePointerPosition[RIGHT_EYE].y)
    {
        currentPosition[RIGHT_EYE].x = relativePointerPosition[RIGHT_EYE].x;
        currentPosition[RIGHT_EYE].y = relativePointerPosition[RIGHT_EYE].y;
    }

    currentPosition[LEFT_EYE].x += g.eyeCenter[LEFT_EYE].x;
    currentPosition[LEFT_EYE].y += g.eyeCenter[LEFT_EYE].y;
    currentPosition[RIGHT_EYE].x += g.eyeCenter[RIGHT_EYE].x;
    currentPosition[RIGHT_EYE].y += g.eyeCenter[RIGHT_EYE].y;

    leftRectangle.left = currentPosition[LEFT_EYE].x - g.eyeballSize.x;
    leftRectangle.top = currentPosition[LEFT_EYE].y - g.eyeballSize.y;
    leftRectangle.right = currentPosition[LEFT_EYE].x + g.eyeballSize.x;
    leftRectangle.bottom = currentPosition[LEFT_EYE].y + g.eyeballSize.y;

    rightRectangle.left = currentPosition[RIGHT_EYE].x - g.eyeballSize.x;
    rightRectangle.top = currentPosition[RIGHT_EYE].y - g.eyeballSize.y;
    rightRectangle.right = currentPosition[RIGHT_EYE].x + g.eyeballSize.x;
    rightRectangle.bottom = currentPosition[RIGHT_EYE].y + g.eyeballSize.y;

    if (g.previousLocation[LEFT_EYE].left < g.previousLocation[LEFT_EYE].right)
    {
        SelectObject(hdc, GetStockObject(WHITE_PEN));
        SelectObject(hdc, GetStockObject(WHITE_BRUSH));
        Ellipse(hdc, g.previousLocation[LEFT_EYE].left, g.previousLocation[LEFT_EYE].top,
            g.previousLocation[LEFT_EYE].right, g.previousLocation[LEFT_EYE].bottom);
        Ellipse(hdc, g.previousLocation[RIGHT_EYE].left, g.previousLocation[RIGHT_EYE].top,
            g.previousLocation[RIGHT_EYE].right, g.previousLocation[RIGHT_EYE].bottom);
    }

    SelectObject(hdc, GetStockObject(BLACK_BRUSH));
    SelectObject(hdc, GetStockObject(BLACK_PEN));
    Ellipse(hdc, leftRectangle.left, leftRectangle.top, leftRectangle.right, leftRectangle.bottom);
    Ellipse(hdc, rightRectangle.left, rightRectangle.top, rightRectangle.right, rightRectangle.bottom);

    g.previousLocation[LEFT_EYE].left = leftRectangle.left;
    g.previousLocation[LEFT_EYE].top = leftRectangle.top;
    g.previousLocation[LEFT_EYE].right = leftRectangle.right;
    g.previousLocation[LEFT_EYE].bottom = leftRectangle.bottom;

    g.previousLocation[RIGHT_EYE].left = rightRectangle.left;
    g.previousLocation[RIGHT_EYE].top = rightRectangle.top;
    g.previousLocation[RIGHT_EYE].right = rightRectangle.right;
    g.previousLocation[RIGHT_EYE].bottom = rightRectangle.bottom;

    ReleaseDC(hwnd, hdc);
}

void PaintEyes(HWND hwnd)
{
    PAINTSTRUCT ps = { 0 };
    RECT clientRectangle = { 0 };
    POINT windowSize = { 0 };
    RECT  leftRectangle = { 0 };
    RECT rightRectangle = { 0 };

    if (g.resetClippingRegion)
    {
        UpdateClippingRegion(hwnd);
        g.resetClippingRegion = 0;
    }

    GetClientRect(hwnd, &clientRectangle);
    windowSize.x = clientRectangle.right - clientRectangle.left;
    windowSize.y = clientRectangle.bottom - clientRectangle.top;

    leftRectangle.left = clientRectangle.left + 1;
    leftRectangle.right = (long)(windowSize.x / 2 - windowSize.x * 0.025);
    leftRectangle.top = clientRectangle.top + 1;
    leftRectangle.bottom = windowSize.y;

    rightRectangle.right = clientRectangle.right - 1;
    rightRectangle.left = (long)(leftRectangle.right + windowSize.x * 0.05);
    rightRectangle.top = clientRectangle.top + 1;
    rightRectangle.bottom = windowSize.y;

    BeginPaint(hwnd, (LPPAINTSTRUCT)&ps);

    SelectObject(ps.hdc, GetStockObject(BLACK_BRUSH));
    SelectObject(ps.hdc, GetStockObject(BLACK_PEN));
    Ellipse(ps.hdc, leftRectangle.left, leftRectangle.top, leftRectangle.right, leftRectangle.bottom);
    Ellipse(ps.hdc, rightRectangle.left, rightRectangle.top, rightRectangle.right, rightRectangle.bottom);

    leftRectangle.left += windowSize.x / 2 / 10; 	leftRectangle.right -= windowSize.x / 2 / 10;
    leftRectangle.top += windowSize.y / 10;    leftRectangle.bottom -= windowSize.y / 10;
    rightRectangle.left += windowSize.x / 2 / 10; 	rightRectangle.right -= windowSize.x / 2 / 10;
    rightRectangle.top += windowSize.y / 10;    rightRectangle.bottom -= windowSize.y / 10;

    SelectObject(ps.hdc, GetStockObject(WHITE_BRUSH));
    SelectObject(ps.hdc, GetStockObject(WHITE_PEN));
    Ellipse(ps.hdc, leftRectangle.left, leftRectangle.top, leftRectangle.right, leftRectangle.bottom);
    Ellipse(ps.hdc, rightRectangle.left, rightRectangle.top, rightRectangle.right, rightRectangle.bottom);

    g.eyeCenter[LEFT_EYE].x = (leftRectangle.right + leftRectangle.left) / 2 + 1;
    g.eyeCenter[LEFT_EYE].y = (leftRectangle.top + leftRectangle.bottom) / 2 + 1;

    g.eyeCenter[RIGHT_EYE].x = (rightRectangle.right + rightRectangle.left) / 2 + 1;
    g.eyeCenter[RIGHT_EYE].y = g.eyeCenter[LEFT_EYE].y;

    g.eyeSize.x = (long)((leftRectangle.right - leftRectangle.left) / 2 / 1.7);
    g.eyeSize.y = (long)((leftRectangle.bottom - leftRectangle.top) / 2 / 1.7);
    g.eyeballSize.x = (long)(g.eyeSize.x / 2.5);
    g.eyeballSize.y = (long)(g.eyeSize.y / 2.5);

    UpdateEyes(hwnd, TRUE);

    EndPaint(hwnd, &ps);
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

    if (pt)
    {
        rc = RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\" APP_NAME, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_READ, NULL, &hkey, NULL);
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

    if (GetWindowRect(hwnd, &rect))
    {
        rc = RegCreateKeyEx(HKEY_CURRENT_USER, L"Software\\" APP_NAME, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, NULL);
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

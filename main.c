#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <shellapi.h>

#include "keygen.h"

#include "assets/resource.h"

static HINSTANCE instance;
static HBITMAP banner_bitmap;

static const char prompt_empty[] = "Enter your six digit Registration Code...";
static const char prompt_short[] = "Enter all six Registration Code digits...";

/* the original exe keeps these GetDlgItemText buffers in writable DATA.
   bytes following the new terminating nul are not erased between edits. */
static char dialog_serial[17];
static char dialog_authorization[8];
static char dialog_registration[7];

static void update_activation(HWND dialog)
{
    char activation[16];
    UINT length;

    GetDlgItemTextA(dialog, IDC_SERIAL, dialog_serial, sizeof(dialog_serial));
    GetDlgItemTextA(dialog, IDC_AUTH, dialog_authorization, sizeof(dialog_authorization));
    length = GetDlgItemTextA(dialog, IDC_REG, dialog_registration, sizeof(dialog_registration));

    if (length == 0)
    {
        SetDlgItemTextA(dialog, IDC_ACTIVATION, prompt_empty);
    }
    else if (length != 6)
    {
        SetDlgItemTextA(dialog, IDC_ACTIVATION, prompt_short);
    }
    else if (kg_make_activation_raw(dialog_serial, dialog_authorization, dialog_registration, activation))
    {
        SetDlgItemTextA(dialog, IDC_ACTIVATION, activation);
    }
}

static void generate(HWND dialog)
{
    char serial[17];
    char authorization[8];
    kg_u32 random = kg_next_random((kg_u32)GetTickCount(), 0x000fffeeUL);

    kg_make_serial(random, serial, authorization);
    SetDlgItemTextA(dialog, IDC_SERIAL, serial);
    SetDlgItemTextA(dialog, IDC_AUTH, authorization);
    update_activation(dialog);
}

static void center_like_original(HWND dialog)
{
    RECT window;
    RECT desktop;
    HWND parent = GetDesktopWindow();
    int width, height, screen_width, screen_height;

    GetWindowRect(dialog, &window);
    GetWindowRect(parent, &desktop);
    width = window.right - window.left;
    height = window.bottom - window.top;
    screen_width = desktop.right - desktop.left;
    screen_height = desktop.bottom - desktop.top;

    MoveWindow(dialog, (screen_width - width) / 2, (screen_height - height) / 2, width, height, FALSE);
}

/* SS_BITMAP sizes itself to the unscaled bitmap pixels, even in a dpi-aware
   dialog. SS_OWNERDRAW keeps the resource-defined rectangle instead. */
static void draw_banner(const DRAWITEMSTRUCT *item)
{
    BITMAP bitmap;
    HDC source;
    HGDIOBJ old_bitmap;
    int width = item->rcItem.right - item->rcItem.left;
    int height = item->rcItem.bottom - item->rcItem.top;

    if (!banner_bitmap || !width || !height || !GetObjectA(banner_bitmap, sizeof(bitmap), &bitmap))
    {
        FillRect(item->hDC, &item->rcItem, (HBRUSH)(COLOR_3DFACE + 1));
        return;
    }

    source = CreateCompatibleDC(item->hDC);
    if (!source)
        return;

    old_bitmap = SelectObject(source, banner_bitmap);
    SetStretchBltMode(item->hDC, HALFTONE);
    SetBrushOrgEx(item->hDC, 0, 0, NULL);
    StretchBlt(item->hDC, item->rcItem.left, item->rcItem.top, width, height, source, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);
    SelectObject(source, old_bitmap);
    DeleteDC(source);
}

static INT_PTR CALLBACK dialog_proc(HWND dialog, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message)
    {
    case WM_INITDIALOG:
        banner_bitmap = LoadBitmapA(instance, MAKEINTRESOURCEA(IDB_BANNER));
        SendMessageA(dialog, WM_SETICON, ICON_SMALL, (LPARAM)LoadIconA(instance, MAKEINTRESOURCEA(IDI_MAIN)));
        center_like_original(dialog);
        SetDlgItemTextA(dialog, IDC_ACTIVATION, prompt_empty);
        return TRUE;

    case WM_DRAWITEM:
        if (wparam == IDC_BANNER)
        {
            draw_banner((const DRAWITEMSTRUCT *)lparam);
            return TRUE;
        }
        return FALSE;

    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case IDCANCEL:
            EndDialog(dialog, 0);
            break;
        case IDC_GENERATE:
            generate(dialog);
            break;
        case IDC_REG:
            /* the original handles every WM_COMMAND from registration. */
            update_activation(dialog);
            break;
        case IDC_BANNER:
            ShellExecuteA(NULL, NULL, "http://www.borland.com/bcppbuilder/", NULL, NULL, SW_HIDE);
            break;
        }
        return TRUE;

    case WM_CLOSE:
        EndDialog(dialog, 0);
        return TRUE;
    }
    return FALSE;
}

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE previous, LPSTR command_line, int show_command)
{
    INT_PTR result;
    instance = hinstance;
    result = DialogBoxParamA(instance, MAKEINTRESOURCEA(IDD_KEYGEN), NULL, dialog_proc, 0);
    if (banner_bitmap)
    {
        DeleteObject(banner_bitmap);
    }
    if (result == -1)
    {
        MessageBoxA(NULL, "Could not create the application dialog.", "Error", MB_ICONERROR);
        return 1;
    }
    return 0;
}

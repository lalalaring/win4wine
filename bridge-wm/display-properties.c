/* Display Properties for win4wine's shells.
 *
 * Wine's own desk.cpl is an empty stub, so the desktop's Properties (and the
 * Control Panel's Display icon) did nothing at all. This program is the window
 * the applet opens; it applies the settings the same way prefix/display.py
 * does: the registry values XP's Display Properties write, and the theme and
 * Start menu helpers this repository builds.
 *
 * Win32 APIs only, no resources: the window and its controls are created in
 * code so bridge-wm/build.sh can build it with nothing but a cross compiler. */
#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>
#include <commctrl.h>
#include <stdlib.h>

#define ID_THEME_CLASSIC    101
#define ID_THEME_LUNA       102
#define ID_MENU_CLASSIC     103
#define ID_MENU_XP          104
#define ID_DPI              105
#define ID_SMOOTH_STANDARD  106
#define ID_SMOOTH_CLEARTYPE 107
#define ID_OK               1
#define ID_CANCEL           2
#define ID_APPLY            108

static const WCHAR class_name[] = L"Win4WineDisplayProperties";
static HINSTANCE module;
static HWND theme_classic, theme_luna, menu_classic, menu_xp, dpi_box, smooth_standard, smooth_cleartype;

static void checked(HWND button, BOOL state)
{
    SendMessageW(button, BM_SETCHECK, state ? BST_CHECKED : BST_UNCHECKED, 0);
}

static BOOL is_checked(HWND button)
{
    return SendMessageW(button, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

static DWORD reg_dword(HKEY root, const WCHAR *path, const WCHAR *name, DWORD fallback)
{
    DWORD value, size = sizeof(value), type;
    HKEY key;
    if (RegOpenKeyExW(root, path, 0, KEY_QUERY_VALUE, &key)) return fallback;
    if (RegQueryValueExW(key, name, NULL, &type, (BYTE *)&value, &size) || type != REG_DWORD) value = fallback;
    RegCloseKey(key);
    return value;
}

static void set_dword(const WCHAR *path, const WCHAR *name, DWORD value)
{
    HKEY key;
    if (!RegCreateKeyExW(HKEY_CURRENT_USER, path, 0, NULL, 0, KEY_SET_VALUE, NULL, &key, NULL))
    {
        RegSetValueExW(key, name, 0, REG_DWORD, (const BYTE *)&value, sizeof(value));
        RegCloseKey(key);
    }
}

static void set_string(const WCHAR *path, const WCHAR *name, const WCHAR *value)
{
    HKEY key;
    if (!RegCreateKeyExW(HKEY_CURRENT_USER, path, 0, NULL, 0, KEY_SET_VALUE, NULL, &key, NULL))
    {
        RegSetValueExW(key, name, 0, REG_SZ, (const BYTE *)value, (DWORD)((lstrlenW(value) + 1) * sizeof(WCHAR)));
        RegCloseKey(key);
    }
}

/* The helpers of this repository live next to the checkout. The session can say
 * where with WIN4WINE_BRIDGE; otherwise they are where setup.sh leaves them. */
static BOOL helper_path(const WCHAR *name, WCHAR *path)
{
    WCHAR user[64], directory[MAX_PATH];
    if (GetEnvironmentVariableW(L"WIN4WINE_BRIDGE", directory, MAX_PATH))
    {
        wsprintfW(path, L"%s\\%s", directory, name);
        return TRUE;
    }
    if (!GetEnvironmentVariableW(L"USERNAME", user, 64) || !user[0]) return FALSE;
    wsprintfW(directory, L"Z:\\home\\%s\\win4wine\\.local\\bridge", user);
    wsprintfW(path, L"%s\\%s", directory, name);
    return TRUE;
}

static void run_helper(const WCHAR *name, const WCHAR *argument)
{
    WCHAR path[MAX_PATH], command[MAX_PATH + 32];
    STARTUPINFOW si = { 0 };

    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    WCHAR environment[] = L"WIN4WINE_EXTERNAL_SHELL=1";
    if (!helper_path(name, path)) return;
    wsprintfW(command, L"\"%s\" %s", path, argument);
    if (CreateProcessW(path, command, NULL, NULL, FALSE, 0, environment, NULL, &si, &pi))
    {
        CloseHandle(pi.hThread);
        WaitForSingleObject(pi.hProcess, 30000);
        CloseHandle(pi.hProcess);
    }
}

static void apply(void)
{
    WCHAR user[64], wallpaper[MAX_PATH], text[16], *end;
    DWORD dpi;

    GetWindowTextW(dpi_box, text, 16);
    dpi = wcstoul(text, &end, 10);
    if (dpi < 19 || dpi > 480) dpi = 96;
    set_dword(L"Control Panel\\Desktop", L"LogPixels", dpi);

    /* What XP's Display Properties write (SPI_SETFONTSMOOTHING and
     * SPI_SETFONTSMOOTHINGTYPE); the session reads them when it starts. */
    set_dword(L"Control Panel\\Desktop", L"FontSmoothing", 2);
    set_dword(L"Control Panel\\Desktop", L"FontSmoothingType", is_checked(smooth_cleartype) ? 2 : 1);
    set_dword(L"Control Panel\\Desktop", L"FontSmoothingOrientation", 1);

    run_helper(L"theme-settings.exe", is_checked(theme_luna) ? L"luna" : L"classic");
    run_helper(L"menu-settings.exe", is_checked(menu_xp) ? L"xp" : L"classic");
    set_dword(L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer", L"NoSimpleStartMenu",
              is_checked(menu_xp) ? 0 : 1);

    /* The wallpaper of XP's themes, as prefix/display.py sets it. */
    if (!GetEnvironmentVariableW(L"USERNAME", user, 64)) user[0] = 0;
    wsprintfW(wallpaper, is_checked(theme_luna)
              ? L"C:\\users\\%s\\AppData\\Local\\Microsoft\\Wallpaper1.bmp" : L"", user);
    set_string(L"Control Panel\\Desktop", L"Wallpaper", wallpaper);
    set_string(L"Control Panel\\Desktop", L"WallpaperStyle", L"2");
    set_string(L"Control Panel\\Desktop", L"TileWallpaper", L"0");
    MessageBeep(MB_ICONASTERISK);
}

static void load_current(void)
{
    HKEY key;
    WCHAR dll[MAX_PATH] = { 0 }, number[16];
    DWORD size = sizeof(dll), type;
    DWORD active = reg_dword(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager",
                             L"ThemeActive", 0);
    if (!RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\ThemeManager",
                       0, KEY_QUERY_VALUE, &key))
    {
        RegQueryValueExW(key, L"DllName", NULL, &type, (BYTE *)dll, &size);
        RegCloseKey(key);
    }
    checked(active && wcsstr(dll, L"luna") ? theme_luna : theme_classic, TRUE);
    checked(reg_dword(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer",
                      L"NoSimpleStartMenu", 1) ? menu_classic : menu_xp, TRUE);
    checked(reg_dword(HKEY_CURRENT_USER, L"Control Panel\\Desktop", L"FontSmoothingType", 1) == 2
            ? smooth_cleartype : smooth_standard, TRUE);
    wsprintfW(number, L"%lu", (unsigned long)reg_dword(HKEY_CURRENT_USER, L"Control Panel\\Desktop",
                                                       L"LogPixels", 96));
    SetWindowTextW(dpi_box, number);
}

static HWND group(HWND window, const WCHAR *title, int x, int y, int w, int h)
{
    return CreateWindowExW(0, L"BUTTON", title, WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                           x, y, w, h, window, NULL, module, NULL);
}

static HWND radio(HWND window, const WCHAR *title, int x, int y, int w, int h, int id)
{
    return CreateWindowExW(0, L"BUTTON", title, WS_CHILD | WS_VISIBLE | WS_GROUP | BS_AUTORADIOBUTTON,
                           x, y, w, h, window, (HMENU)(INT_PTR)id, module, NULL);
}

static void create_controls(HWND window)
{
    group(window, L"主题", 12, 8, 380, 56);
    theme_classic = radio(window, L"Windows 经典", 28, 30, 130, 22, ID_THEME_CLASSIC);
    theme_luna = radio(window, L"Windows XP (Luna)", 170, 30, 170, 22, ID_THEME_LUNA);

    group(window, L"开始菜单", 12, 72, 380, 56);
    menu_classic = radio(window, L"经典", 28, 94, 110, 22, ID_MENU_CLASSIC);
    menu_xp = radio(window, L"XP", 170, 94, 110, 22, ID_MENU_XP);

    group(window, L"字体大小 (DPI)", 12, 136, 380, 62);
    dpi_box = CreateWindowExW(WS_EX_CLIENTEDGE, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWN,
                              28, 158, 90, 200, window, (HMENU)(INT_PTR)ID_DPI, module, NULL);
    SendMessageW(dpi_box, CB_ADDSTRING, 0, (LPARAM)L"96");
    SendMessageW(dpi_box, CB_ADDSTRING, 0, (LPARAM)L"120");
    SendMessageW(dpi_box, CB_ADDSTRING, 0, (LPARAM)L"144");
    SendMessageW(dpi_box, CB_ADDSTRING, 0, (LPARAM)L"192");
    CreateWindowExW(0, L"STATIC", L"96 标准，120 大尺寸；也可以填 19–480",
                    WS_CHILD | WS_VISIBLE, 132, 160, 250, 34, window, NULL, module, NULL);

    group(window, L"字体平滑", 12, 206, 380, 56);
    smooth_standard = radio(window, L"标准", 28, 228, 110, 22, ID_SMOOTH_STANDARD);
    smooth_cleartype = radio(window, L"ClearType", 170, 228, 150, 22, ID_SMOOTH_CLEARTYPE);

    CreateWindowExW(0, L"STATIC", L"改动会在下次登录后生效。", WS_CHILD | WS_VISIBLE,
                    16, 272, 376, 20, window, NULL, module, NULL);
    CreateWindowExW(0, L"BUTTON", L"确定", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
                    196, 300, 84, 28, window, (HMENU)(INT_PTR)ID_OK, module, NULL);
    CreateWindowExW(0, L"BUTTON", L"应用", WS_CHILD | WS_VISIBLE,
                    288, 300, 84, 28, window, (HMENU)(INT_PTR)ID_APPLY, module, NULL);
    CreateWindowExW(0, L"BUTTON", L"取消", WS_CHILD | WS_VISIBLE,
                    380, 300, 84, 28, window, (HMENU)(INT_PTR)ID_CANCEL, module, NULL);
}

static LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message)
    {
    case WM_CREATE:
        create_controls(window);
        load_current();
        return 0;
    case WM_COMMAND:
        switch (LOWORD(wparam))
        {
        case ID_OK:
            apply();
            DestroyWindow(window);
            return 0;
        case ID_APPLY:
            apply();
            return 0;
        case ID_CANCEL:
            DestroyWindow(window);
            return 0;
        }
        return 0;
    case WM_CLOSE:
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, LPWSTR command_line, int show)
{
    WNDCLASSEXW wc = { 0 };
    MSG message;
    HWND window;
    (void)previous;
    (void)command_line;
    (void)show;
    module = instance;
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = window_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = class_name;
    if (!RegisterClassExW(&wc)) return 1;
    window = CreateWindowExW(WS_EX_DLGMODALFRAME, class_name, L"显示 属性",
                             WS_CAPTION | WS_SYSMENU | WS_VISIBLE | WS_CLIPCHILDREN,
                             CW_USEDEFAULT, CW_USEDEFAULT, 512, 376, NULL, NULL, instance, NULL);
    if (!window) return 1;
    while (GetMessageW(&message, NULL, 0, 0) > 0)
    {
        if (!IsDialogMessageW(window, &message))
        {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    return 0;
}

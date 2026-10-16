/* desk.cpl - the applet the shell's desktop Properties starts.
 *
 * The shell runs "rundll32 … desk.cpl"; Wine's own desk.cpl is an empty stub,
 * so nothing ever appeared. This module only starts the window program this
 * repository builds (display-properties.exe, beside it in the prefix), which
 * keeps creating windows out of the loader's way.
 *
 * Win32 APIs only. */
#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>
#include <cpl.h>

static DWORD start_properties(void)
{
    WCHAR path[MAX_PATH], directory[MAX_PATH], command[MAX_PATH + 64];
    STARTUPINFOW si = { 0 };

    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    DWORD length;
    if (!GetModuleFileNameW(NULL, path, MAX_PATH)) return 1;
    length = lstrlenW(path);
    while (length && path[length] != L'\\' && path[length] != L'/') length--;
    lstrcpynW(directory, path, (int)length + 1);
    wsprintfW(path, L"%s\\display-properties.exe", directory);
    wsprintfW(command, L"\"%s\"", path);
    if (CreateProcessW(path, command, NULL, NULL, FALSE, 0, NULL, directory, &si, &pi))
    {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        return 0;
    }
    /* Without the program beside it, fall back to saying so. */
    MessageBoxW(NULL, L"display-properties.exe is missing next to desk.cpl.",
                L"显示 属性", MB_ICONERROR);
    return 2;
}

/* rundll32 loads this module and unloads it again at once, so the window
 * program is started here, before that can happen; CPlApplet is kept for the
 * Control Panel, which asks the applet to open itself. */
BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(instance);
        start_properties();
    }
    return TRUE;
}

LONG WINAPI __declspec(dllexport) CPlApplet(HWND window, UINT message, LPARAM lparam1, LPARAM lparam2)
{
    CPLINFO *info = (CPLINFO *)lparam2;
    (void)window;
    (void)lparam1;
    switch (message)
    {
    case CPL_INIT:
        return TRUE;
    case CPL_GETCOUNT:
        return 1;
    case CPL_INQUIRE:
        if (info)
        {
            info->idIcon = 0;
            info->idName = 0;
            info->idInfo = 0;
            info->lData = 0;
        }
        return 0;
    case CPL_DBLCLK:
    case CPL_STARTWPARMSA:
    case CPL_STARTWPARMSW:
        start_properties();
        return 0;
    }
    return 0;
}

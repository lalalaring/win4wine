/* Start a Windows program for a Start menu shortcut.
 *
 * Wine's builtin programs (taskmgr, notepad, cmd, regedit, control, …) are not
 * files inside the prefix, and IShellLink refuses a shortcut whose target does
 * not exist, so a shortcut cannot point at them directly. A shortcut points at
 * this launcher instead, with the program and its arguments as its arguments:
 *
 *   win-run.exe taskmgr
 *   win-run.exe cmd /k ver
 *
 * Win32 APIs only; bridge-wm/build.sh builds it as a GUI program (-municode
 * -mwindows), hence wWinMain and no console window. */
#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, LPWSTR command_line, int show)
{
    WCHAR *command = GetCommandLineW(), *program, *rest;
    STARTUPINFOW si = { 0 };
    PROCESS_INFORMATION pi;
    WCHAR buffer[32768];

    (void)instance;
    (void)previous;
    (void)command_line;
    (void)show;
    si.cb = sizeof(si);

    /* Skip our own program name, then the first word is the program to start. */
    program = command;
    if (*program == L'"')
    {
        program++;
        while (*program && *program != L'"') program++;
        if (*program) program++;
    }
    else
    {
        while (*program && *program != L' ' && *program != L'\t') program++;
    }
    while (*program == L' ' || *program == L'\t') program++;
    if (!*program) return 2;

    /* The arguments: everything after that first word, kept as it is. */
    rest = program;
    while (*rest && *rest != L' ' && *rest != L'\t') rest++;

    lstrcpynW(buffer, program, (int)(rest - program) + 1);
    if (*rest) lstrcatW(buffer, rest);

    if (!CreateProcessW(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) return 1;
    CloseHandle(pi.hThread);
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    return 0;
}

/*
 * Windows launcher for the James Pond 3 builder.
 *
 * Double-click it (or drag the ROM onto it), type the ROM path, and the
 * bundled CMake/Zig toolchain compiles the game into "James Pond 3 Recomp"
 * next to this executable.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#define CMD_MAX 32768

static int is_tty(FILE *stream)
{
    return _isatty(_fileno(stream));
}

/* Trim the whitespace and quotes that drag-and-drop or paste leaves behind. */
static void trim(wchar_t *s)
{
    size_t n = wcslen(s);
    while (n > 0 && (s[n - 1] == L'\n' || s[n - 1] == L'\r' ||
                     s[n - 1] == L' '  || s[n - 1] == L'\t' ||
                     s[n - 1] == L'"'  || s[n - 1] == L'\''))
        s[--n] = L'\0';
    wchar_t *p = s;
    while (*p == L' ' || *p == L'\t' || *p == L'"' || *p == L'\'')
        p++;
    if (p != s)
        memmove(s, p, (wcslen(p) + 1) * sizeof(wchar_t));
}

static void pause_console(void)
{
    if (!is_tty(stdin))
        return;
    wprintf(L"\nPress Enter to close...");
    fflush(stdout);
    wchar_t line[16];
    fgetws(line, 16, stdin);
}

static void usage(void)
{
    wprintf(L"James Pond 3 builder\n"
            L"\n"
            L"Usage: Build James Pond 3.exe [path-to-rom]\n"
            L"\n"
            L"Without an argument the builder asks for the path to the ROM\n"
            L"\"James Pond 3 (USA, Europe).md\" (2,097,152 bytes).\n"
            L"It compiles the game into \"James Pond 3 Recomp\" next to this\n"
            L"executable and leaves your ROM untouched.\n");
}

int wmain(int argc, wchar_t **argv)
{
    wchar_t exe[MAX_PATH];
    DWORD len = GetModuleFileNameW(NULL, exe, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) {
        fwprintf(stderr, L"Cannot locate the builder executable.\n");
        return 1;
    }
    wchar_t *slash = wcsrchr(exe, L'\\');
    if (slash)
        *slash = L'\0';

    if (argc > 1 && (!wcscmp(argv[1], L"--help") ||
                     !wcscmp(argv[1], L"-h") ||
                     !wcscmp(argv[1], L"/?"))) {
        usage();
        pause_console();
        return 0;
    }

    static wchar_t rom[CMD_MAX];
    if (argc > 1) {
        wcsncpy(rom, argv[1], CMD_MAX - 1);
    } else {
        wprintf(L"James Pond 3 builder\n\n"
                L"Enter the path to your ROM\n"
                L"(James Pond 3 (USA, Europe).md): ");
        fflush(stdout);
        if (!fgetws(rom, CMD_MAX, stdin)) {
            fwprintf(stderr, L"\nNo ROM path given.\n");
            pause_console();
            return 1;
        }
    }
    trim(rom);
    if (!rom[0]) {
        fwprintf(stderr, L"No ROM path given.\n");
        pause_console();
        return 1;
    }

    wprintf(L"\nBuilding into: %ls\\James Pond 3 Recomp\n"
            L"This takes a few minutes; progress follows.\n\n", exe);
    fflush(stdout);

    /* cmd.exe strips the first and last quote of a /c command line, so guard
     * the quoted executable path with one extra pair. */
    static wchar_t cmd[CMD_MAX];
    int n = _snwprintf(cmd, CMD_MAX,
        L"\"\"%ls\\toolchain\\cmake\\bin\\cmake.exe\" "
        L"\"-DROM=%ls\" \"-DOUT_DIR=%ls\\James Pond 3 Recomp\" "
        L"\"-DPAYLOAD=%ls\" -P \"%ls\\builder.cmake\"",
        exe, rom, exe, exe, exe);
    if (n < 0 || n >= CMD_MAX) {
        fwprintf(stderr, L"Path is too long.\n");
        pause_console();
        return 1;
    }

    int rc = _wsystem(cmd);
    if (rc == 0) {
        wprintf(L"\nDone. The game is in:\n  %ls\\James Pond 3 Recomp\n"
                L"Run JamesPond3Recomp.exe there.\n", exe);
    } else {
        fwprintf(stderr,
                 L"\nBuild failed (exit code %d). See the messages above.\n",
                 rc);
    }
    pause_console();
    return rc;
}

#include <windows.h>
#include <string>
#include <random>
#include <ctime>
#include <sstream>

// ---- Control IDs ----
#define IDC_LENGTH_EDIT   101
#define IDC_UPPER_CHECK   102
#define IDC_LOWER_CHECK   103
#define IDC_NUMBER_CHECK  104
#define IDC_SYMBOL_CHECK  105
#define IDC_GENERATE_BTN  106
#define IDC_OUTPUT_EDIT   107
#define IDC_COPY_BTN      108
#define IDC_STRENGTH_BAR  109

// ---- Helper: Generate Password ----
std::string GeneratePassword(int length, bool useUpper, bool useLower, bool useNumbers, bool useSymbols)
{
    std::string upper = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string lower = "abcdefghijklmnopqrstuvwxyz";
    std::string numbers = "0123456789";
    std::string symbols = "!@#$%^&*()-_=+[]{};:,.<>?/";

    std::string all;
    if (useUpper) all += upper;
    if (useLower) all += lower;
    if (useNumbers) all += numbers;
    if (useSymbols) all += symbols;

    if (all.empty()) return "Select at least one option";

    std::mt19937 rng(static_cast<unsigned>(std::time(nullptr)));
    std::uniform_int_distribution<> dist(0, static_cast<int>(all.size()) - 1);

    std::string result;
    for (int i = 0; i < length; ++i)
        result += all[dist(rng)];

    return result;
}

// ---- Helper: Estimate Strength ----
int EstimateStrength(const std::string& pwd)
{
    int score = 0;
    if (pwd.length() >= 8) score += 25;
    if (pwd.length() >= 12) score += 25;
    bool hasUpper = false, hasLower = false, hasNum = false, hasSym = false;
    for (char c : pwd)
    {
        if (isupper((unsigned char)c)) hasUpper = true;
        else if (islower((unsigned char)c)) hasLower = true;
        else if (isdigit((unsigned char)c)) hasNum = true;
        else hasSym = true;
    }
    if (hasUpper && hasLower) score += 25;
    if (hasNum || hasSym) score += 25;
    return min(score, 100);
}

// ---- Window Procedure ----
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        CreateWindow("static", "Length:", WS_VISIBLE | WS_CHILD, 20, 20, 60, 20, hwnd, NULL, NULL, NULL);
        CreateWindow("edit", "12", WS_VISIBLE | WS_CHILD | WS_BORDER, 80, 20, 50, 20, hwnd, (HMENU)IDC_LENGTH_EDIT, NULL, NULL);

        CreateWindow("button", "Uppercase", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 20, 50, 100, 20, hwnd, (HMENU)IDC_UPPER_CHECK, NULL, NULL);
        CreateWindow("button", "Lowercase", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 20, 75, 100, 20, hwnd, (HMENU)IDC_LOWER_CHECK, NULL, NULL);
        CreateWindow("button", "Numbers", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 20, 100, 100, 20, hwnd, (HMENU)IDC_NUMBER_CHECK, NULL, NULL);
        CreateWindow("button", "Symbols", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX, 20, 125, 100, 20, hwnd, (HMENU)IDC_SYMBOL_CHECK, NULL, NULL);

        CreateWindow("button", "Generate", WS_VISIBLE | WS_CHILD, 20, 160, 100, 25, hwnd, (HMENU)IDC_GENERATE_BTN, NULL, NULL);
        CreateWindow("edit", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, 20, 190, 260, 25, hwnd, (HMENU)IDC_OUTPUT_EDIT, NULL, NULL);
        CreateWindow("button", "Copy", WS_VISIBLE | WS_CHILD, 200, 160, 80, 25, hwnd, (HMENU)IDC_COPY_BTN, NULL, NULL);

        CreateWindow(PROGRESS_CLASS, NULL, WS_VISIBLE | WS_CHILD, 20, 220, 260, 20, hwnd, (HMENU)IDC_STRENGTH_BAR, NULL, NULL);
        SendMessage(GetDlgItem(hwnd, IDC_STRENGTH_BAR), PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_GENERATE_BTN)
        {
            char lenBuffer[10];
            GetWindowText(GetDlgItem(hwnd, IDC_LENGTH_EDIT), lenBuffer, 10);
            int length = atoi(lenBuffer);
            if (length <= 0) length = 12;

            bool upper = SendMessage(GetDlgItem(hwnd, IDC_UPPER_CHECK), BM_GETCHECK, 0, 0);
            bool lower = SendMessage(GetDlgItem(hwnd, IDC_LOWER_CHECK), BM_GETCHECK, 0, 0);
            bool number = SendMessage(GetDlgItem(hwnd, IDC_NUMBER_CHECK), BM_GETCHECK, 0, 0);
            bool symbol = SendMessage(GetDlgItem(hwnd, IDC_SYMBOL_CHECK), BM_GETCHECK, 0, 0);

            std::string password = GeneratePassword(length, upper, lower, number, symbol);
            SetWindowText(GetDlgItem(hwnd, IDC_OUTPUT_EDIT), password.c_str());

            int strength = EstimateStrength(password);
            SendMessage(GetDlgItem(hwnd, IDC_STRENGTH_BAR), PBM_SETPOS, strength, 0);
        }
        else if (LOWORD(wParam) == IDC_COPY_BTN)
        {
            char buffer[256];
            GetWindowText(GetDlgItem(hwnd, IDC_OUTPUT_EDIT), buffer, 256);

            if (OpenClipboard(hwnd))
            {
                EmptyClipboard();
                HGLOBAL hClipboardData = GlobalAlloc(GMEM_DDESHARE, strlen(buffer) + 1);
                char* pchData = (char*)GlobalLock(hClipboardData);
                strcpy(pchData, buffer);
                GlobalUnlock(hClipboardData);
                SetClipboardData(CF_TEXT, hClipboardData);
                CloseClipboard();
            }
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ---- Entry Point ----
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow)
{
    // Register progress bar control
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icex);

    const char CLASS_NAME[] = "PasswordGenWin32";

    WNDCLASS wc = { };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    HWND hwnd = CreateWindow(CLASS_NAME, "C++ Password Generator",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 320, 300,
        NULL, NULL, hInst, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

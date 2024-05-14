#include <string>
#include <iostream>
#include "framework.h"
#include "novusTerm.h"
#include <Windows.h>
#include <richedit.h>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND hConsoleWnd = NULL;
HANDLE hStdIn = NULL;
HANDLE hStdOut = NULL;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
DWORD WINAPI        ConsoleThreadProc(LPVOID lpParameter);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_NOVUSTERM, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_NOVUSTERM));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_NOVUSTERM));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = CreateSolidBrush(RGB(0, 0, 0)); // Set background color to black
    wcex.lpszMenuName = NULL; // Remove the menu
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance; // Store instance handle in our global variable

    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
    {
        return FALSE;
    }

    // Create a console window
    AllocConsole();

    // Redirect stdin, stdout, and stderr to the console window
    FILE* fp;
    freopen_s(&fp, "CONIN$", "r", stdin);
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);

    // Get the console window handle
    hConsoleWnd = GetConsoleWindow();

    // Set the console window as a child of the main window
    SetParent(hConsoleWnd, hWnd);

    // Modify the console window style to remove the close button and resize borders
    LONG_PTR style = GetWindowLongPtr(hConsoleWnd, GWL_STYLE);
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
    SetWindowLongPtr(hConsoleWnd, GWL_STYLE, style);

    // Set the console window background color to black
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_GREEN | FOREGROUND_INTENSITY);

    // Set the console window font to a fixed-width font
    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof(cfi);
    cfi.nFont = 0;
    cfi.dwFontSize.X = 0;
    cfi.dwFontSize.Y = 16;
    cfi.FontFamily = FF_DONTCARE;
    cfi.FontWeight = FW_NORMAL;
    wcscpy_s(cfi.FaceName, L"Consolas");
    SetCurrentConsoleFontEx(hConsole, FALSE, &cfi);

    // Create a thread to handle console input/output
    HANDLE hThread = CreateThread(NULL, 0, ConsoleThreadProc, NULL, 0, NULL);

    // Show the main window
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE: Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
    {
        // Resize the console window to fit the client area
        RECT rect;
        GetClientRect(hWnd, &rect);
        MoveWindow(hConsoleWnd, 0, 0, rect.right, rect.bottom, TRUE);
        break;
    }
    case WM_DESTROY:
        // Cleanup and post a quit message
        FreeConsole();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

//
//  FUNCTION: ConsoleThreadProc(LPVOID)
//
//  PURPOSE: Thread function to handle console input/output
//
//
//  FUNCTION: ConsoleThreadProc(LPVOID)
//
//  PURPOSE: Thread function to handle console input/output
//
// ... (previous code remains the same)

//
//  FUNCTION: ConsoleThreadProc(LPVOID)
//
//  PURPOSE: Thread function to handle console input/output
//
DWORD WINAPI ConsoleThreadProc(LPVOID lpParameter)
{
    CHAR chBuf[1024];
    BOOL bSuccess = FALSE;

    // Continuously read input from the console and execute commands
    while (true)
    {
        // Get the Current Directory 
        WCHAR currentDirectory[MAX_PATH];
        GetCurrentDirectory(MAX_PATH, currentDirectory);

        // Convert the current directory to a narrow-character string
        std::wstring wideCurrentDirectory(currentDirectory);
        std::string narrowCurrentDirectory(wideCurrentDirectory.begin(), wideCurrentDirectory.end());

        // Print the current directory as the command prompt
        std::cout << "\n" << narrowCurrentDirectory << ">";

        std::string command;
        std::getline(std::cin, command);


        // Check if the command is "cd"
        if (command.substr(0, 2) == "cd")
        {
            // Extract the directory path from the command
            std::string directory = command.substr(3);

            // Convert the directory path to a wide-character string
            std::wstring wideDirectory(directory.begin(), directory.end());

            // Change the current directory
            if (SetCurrentDirectory(wideDirectory.c_str()))
            {
                // Directory changed successfully
                continue;
            }
            else
            {
                std::cout << "Failed to change directory." << std::endl;
            }
        }
        // check if the command is clear
        else if (command == "clear" || command == "cls" || command == "c") {
			// Clear the console window
			system("cls");
        }
       
        // Check if the command is "vim"
        //path "C:\Program Files\Vim\vim91\vim.exe"
        else if (command == "vim")
        {
            STARTUPINFO si;
            PROCESS_INFORMATION pi;

            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));

            si.dwFlags = STARTF_USESTDHANDLES;
            si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
            si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
            si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

            // Specify the full path to the Vim executable
            WCHAR cmd[] = L"C:\\Program Files\\Vim\\vim91\\vim.exe";

            // Start the child process. 
            if (!CreateProcess(NULL,   // No module name (use command line)
                cmd,        // Command line
                NULL,           // Process handle not inheritable
                NULL,           // Thread handle not inheritable
                TRUE,          // Set handle inheritance to TRUE
                0,              // No creation flags
                NULL,           // Use parent's environment block
                NULL,           // Use parent's starting directory 
                &si,            // Pointer to STARTUPINFO structure
                &pi)           // Pointer to PROCESS_INFORMATION structure
                )
            {
                printf("CreateProcess failed (%d).\n", GetLastError());
                return 1;
            }

            // Wait until child process exits.
            WaitForSingleObject(pi.hProcess, INFINITE);

            // Close process and thread handles. 
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
        else
        {
	  // Wrap the command in quotes
	  std::string quotedCommand = "\"" + command + "\"";

          // Execute the command using _popen
            FILE* pipe = _popen(command.c_str(), "r");
            if (pipe != NULL)
            {
                while (fgets(chBuf, sizeof(chBuf), pipe) != NULL)
                {
                    printf("%s", chBuf);
                }
                _pclose(pipe);
            }
        }
    }

    return 0;
}

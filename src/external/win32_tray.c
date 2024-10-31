#include "win32_tray.h"

static HWND TRAY_WINDOW_HANDLE;
static NOTIFYICONDATAA ICON_DATA;
static WNDCLASSA TRAY_WINDOW_CLASS;
static HMENU TRAY_POPUP_MENU;


void TrayTest(char* test_string)
{
    puts("Tray test success");
    if (test_string)
    {
        puts(test_string);
    }
    return;
}

// Initialisation of the icon
bool InitTrayIcon(char* icon_path, char* tooltip_text, DWORD info_flags)
{
    // Create a dummy window class and window for just the tray icon, makes the window callback a bit simpler
    if (!RegisterTrayWindowClass())
    {
        puts("Problem registering tray window class");
    }

    InitNotificationStruct(tooltip_text, icon_path, info_flags);
    puts("Finished notifydata struct");
    PlaceTrayIcon();
    
    return true;
}

void InitNotificationStruct(char* tooltip_text, char* icon_path, DWORD info_flags)
{
    // The fixed items first
    ICON_DATA.cbSize = sizeof(NOTIFYICONDATAA);
    ICON_DATA.hWnd = TRAY_WINDOW_HANDLE;
    // Not interested in overcomplicating things by responding differently to keyboard selection or different locations on the icon
    ICON_DATA.uVersion = NOTIFYICON_VERSION;
    // {8E53BAC3-0CCE-4EA4-9260-4DD96814F1AA}
    static const GUID shell_icon_guid = { 0x8e53bac3, 0xcce, 0x4ea4, { 0x92, 0x60, 0x4d, 0xd9, 0x68, 0x14, 0xf1, 0xaa } };
    ICON_DATA.guidItem = shell_icon_guid;
    // Don't want to start hidden but make it an option to hide
    ICON_DATA.dwState = NIS_HIDDEN;
    ICON_DATA.dwStateMask = 0;

    ICON_DATA.uCallbackMessage = WM_TRAYICON;

    // Leave the balloon text alone for now
    *ICON_DATA.szInfo = '\0';
    *ICON_DATA.szInfoTitle = '\0';



    ICON_DATA.uFlags = NIF_ICON | NIF_TIP | NIF_STATE | NIF_GUID | NIF_SHOWTIP | NIF_MESSAGE;
    
    
    ICON_DATA.hIcon = LoadImageA(NULL, "C:/Users/Wilkie/Documents/RayGUI_dev/raylib/logo/raylib_64x64.png", IMAGE_ICON, 64, 64, LR_LOADFROMFILE | LR_LOADTRANSPARENT);
    /*
    if (icon_path == NULL)
    {
        ICON_DATA.hIcon = NULL;
    }
    else
    {
        ICON_DATA.hIcon = LoadImageA(NULL, "C:/Users/Wilkie/Documents/RayGUI_dev/raylib/logo/raylib_64x64.png", IMAGE_ICON, 64, 64, LR_LOADFROMFILE | LR_LOADTRANSPARENT);
    }
    */
    // Balloon uses the tray icon
    ICON_DATA.hBalloonIcon = ICON_DATA.hIcon;


    if (tooltip_text == NULL)
    {
        *ICON_DATA.szTip = '\0';
    }
    else
    {
        strcpy_s(ICON_DATA.szTip, tooltip_text, 128);
        ICON_DATA.szTip[127] = '\0';
    }
    return;
}

bool RegisterTrayWindowClass(void)
{
    memset(&TRAY_WINDOW_CLASS, 0, sizeof(TRAY_WINDOW_CLASS));

    char* tray_window_class_name = "TrayIconClass";

    TRAY_WINDOW_CLASS.lpfnWndProc = TrayWindowProcess;          // Pointer to the window procedure function
    TRAY_WINDOW_CLASS.hInstance = GetModuleHandleA(NULL);           // Handle to the application instance
    TRAY_WINDOW_CLASS.lpszClassName = tray_window_class_name;          // Name of the window class

    RegisterClassA(&TRAY_WINDOW_CLASS);

    TRAY_WINDOW_HANDLE = CreateWindowA(tray_window_class_name, "Tray Icon", WS_OVERLAPPEDWINDOW, 10, 10, 10, 10, NULL, NULL, NULL, NULL);

    if (!TRAY_WINDOW_HANDLE)
    {
        puts("Error creating tray window");
        return false;
    }
	return true;
}

void PlaceTrayIcon(void)
{
    if (!Shell_NotifyIconA(NIM_ADD, &ICON_DATA))
    {
        // It doesn't get cleaned up properly if the program is quit or crashes and so this fixes it in most cases
        Shell_NotifyIconA(NIM_DELETE, &ICON_DATA);
        if (!Shell_NotifyIconA(NIM_ADD, &ICON_DATA))
        {
            puts("Tray icon creation error");
            Shell_NotifyIconA(NIM_DELETE, &ICON_DATA);
            return;
        }
    }
    return;
}
// Notification balloon

// Interactions with the tray icon
LRESULT CALLBACK TrayWindowProcess(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_TRAYICON:
        switch (lParam)
        {
        case WM_LBUTTONDOWN:
        case WM_MOUSEMOVE:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONDBLCLK:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case NIN_BALLOONSHOW:
            // Don't care about any of these
            break;
        case WM_RBUTTONUP:
            // Not implemented yet
            TrayPopupMenu();
            break;
        // These get passed back to the user using a queue
        case WM_MBUTTONDBLCLK:
            PushTrayAction(tray_icon_double_clicked);
        case NIN_BALLOONHIDE:
            PushTrayAction(balloon_hidden);
        case NIN_BALLOONTIMEOUT:
            PushTrayAction(balloon_timed_out);
        case WM_LBUTTONUP:
            PushTrayAction(tray_icon_clicked);
        case NIN_BALLOONUSERCLICK:
            PushTrayAction(balloon_clicked);
            break;
        default:
            printf("Unrecognised tray Icon Event: %llu\n", lParam);
            break;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0); // Post a quit message when the window is closed
        break;
    case WM_COMMAND:
        PopupMenuItemSelected(LOWORD(wParam));
        break;
    default:
        return DefWindowProc(hwnd, message, wParam, lParam);
    }
    return (LRESULT)NULL;
}

void PushTrayAction(trayEvent event)
{
    TrayActionFIFO(event);
    return;
}

trayEvent GetTrayAction(void)
{
    return TrayActionFIFO(no_event);
}

#define TRAY_BUFFER_LENGTH 50
trayEvent TrayActionFIFO(trayEvent event)
{
    static trayEvent events_queue[TRAY_BUFFER_LENGTH] = { no_event };
    static int read = 0;
    static int write = 0;

    if (event == no_event) // Read
    {
        // Empty check
        if (read == write)
        {
            return no_event;
        }
        else
        {
            trayEvent read_event = events_queue[read];

            read++;
            if (read >= TRAY_BUFFER_LENGTH)
            {
                read = 0;
            }

            return read_event;
        }
    }
    else // Write
    {
        // Store event at the current write position
        events_queue[write] = event;

        // Advance the write position
        write++;
        if (write >= TRAY_BUFFER_LENGTH)
        {
            write = 0;
        }

        // Overflow, shove read forwards
        if (write == read)
        {
            read++;
            if (read >= TRAY_BUFFER_LENGTH)
            {
                read = 0;
            }
        }

        return no_event;
    }
}


// Context menu handling
void TrayPopupMenu(void)
{
    return;
}

void PopupMenuItemSelected(const UINT16 item)
{
    return;
}


// Cleaning up and quitting
void RemoveTrayWindowClass(void)
{
    DestroyWindow(TRAY_WINDOW_HANDLE);
    UnregisterClassA("TrayIconClass", TRAY_WINDOW_CLASS.hInstance);
    return;
}
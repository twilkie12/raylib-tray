#include <stdbool.h>
#include <stdio.h>

#define NOGDI
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <shellapi.h>


#define WM_TRAYICON (9009)

typedef enum
{
    NO_TRAY_EVENT = 0,
    TRAY_ICON_CLICKED,
    TRAY_ICON_DOUBLE_CLICKED,
    NOTIFICATION_BALLOON_TIMEOUT,
    NOTIFICATION_BALLOON_HIDDEN,
    NOTIFICATION_BALLOON_CLICKED,

}trayEvent;

#define TRAY_BUFFER_LENGTH 50
static HWND TRAY_WINDOW_HANDLE;
static NOTIFYICONDATAA ICON_DATA;
static WNDCLASSA TRAY_WINDOW_CLASS;
static HMENU TRAY_POPUP_MENU;


static void InitNotificationData(const char* icon_path, const char* tooltip_text, DWORD info_flags);
static bool RegisterTrayWindowClass(void);
static void PlaceTrayIcon(void);
static LRESULT CALLBACK TrayWindowProcess(HWND, UINT, WPARAM, LPARAM);
static trayEvent TrayEventFIFO(trayEvent event);
static void PushTrayEvent(trayEvent event);

void TrayPopupMenu(void);
static void PopupMenuItemSelected(const UINT16 item);


// Just checking that everything is working properly
void TrayTest(const char* test_string)
{
    puts("Tray test success");
    if (test_string)
    {
        puts(test_string);
    }
    return;
}

// Initialisation of the icon
bool InitTrayIcon(const char* icon_path, const char* tooltip_text)
{
    // Create a dummy window class and window for just the tray icon, makes the window callback a bit simpler
    if (!RegisterTrayWindowClass())
    {
        puts("Problem registering tray window class");
    }
    DWORD info_flags = 0;
    InitNotificationData(icon_path, tooltip_text, info_flags);
    puts("Finished NOTIFYDATAA struct");
    PlaceTrayIcon();
    
    return true;
}

void InitNotificationData(const char* icon_path, const char* tooltip_text, DWORD info_flags)
{
    // The fixed items first
    ICON_DATA.cbSize = sizeof(NOTIFYICONDATAA);
    ICON_DATA.hWnd = TRAY_WINDOW_HANDLE;
    // Not interested in overcomplicating things by responding differently to keyboard selection or different locations on the icon
    ICON_DATA.uVersion = NOTIFYICON_VERSION;
    // {98C27C25-829B-497D-BDA9-F4606B1399B6}
    static const GUID shell_icon_guid = { 0x98c27c25, 0x829b, 0x497d, { 0xbd, 0xa9, 0xf4, 0x60, 0x6b, 0x13, 0x99, 0xb6 } };

    ICON_DATA.guidItem = shell_icon_guid;
    // Don't want to start hidden but make it an option to hide
    ICON_DATA.dwState = 0;
    ICON_DATA.dwStateMask = NIS_HIDDEN;

    ICON_DATA.uCallbackMessage = WM_TRAYICON;

    // Leave the balloon text alone for now
    *ICON_DATA.szInfo = '\0';
    *ICON_DATA.szInfoTitle = '\0';

    ICON_DATA.uFlags = NIF_ICON | NIF_TIP | NIF_STATE | NIF_GUID | NIF_SHOWTIP | NIF_MESSAGE;
    ICON_DATA.dwInfoFlags = NIIF_NOSOUND | NIIF_LARGE_ICON | NIIF_USER;

    if (icon_path == NULL)
    {
        ICON_DATA.hIcon = NULL;
    }
    else
    {
        // Need to check that it is an .ico or .bmp file, other file types will not work
        // Also need to confirm what the x and y sizes should be for .ico files
        ICON_DATA.hIcon = LoadImageA(NULL, icon_path, IMAGE_ICON, 64, 64, LR_LOADFROMFILE | LR_LOADTRANSPARENT);
        if (ICON_DATA.hIcon == NULL)
        {
            printf("Error code %s loading icon file 0x%X\n", icon_path, GetLastError());
        }
    }
    
    // Balloon uses the tray icon
    ICON_DATA.hBalloonIcon = ICON_DATA.hIcon;

    if (tooltip_text == NULL)
    {
        *ICON_DATA.szTip = '\0';
    }
    else
    {
        strcpy_s(ICON_DATA.szTip, 128, tooltip_text);
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

void HideTrayIcon(void)
{
    ICON_DATA.dwState = NIS_HIDDEN;
    Shell_NotifyIconA(NIM_MODIFY, &ICON_DATA);
    return;
}

void ShowTrayIcon(void)
{
    ICON_DATA.dwState = 0;
    Shell_NotifyIconA(NIM_MODIFY, &ICON_DATA);
    return;
}

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
        case WM_MBUTTONDBLCLK:
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
        case WM_LBUTTONDBLCLK:
            PushTrayEvent(TRAY_ICON_DOUBLE_CLICKED);
            break;
        case NIN_BALLOONHIDE:
            PushTrayEvent(NOTIFICATION_BALLOON_HIDDEN);
            break;
        case NIN_BALLOONTIMEOUT:
            PushTrayEvent(NOTIFICATION_BALLOON_TIMEOUT);
            break;
        case WM_LBUTTONUP:
            PushTrayEvent(TRAY_ICON_CLICKED);
            break;
        case NIN_BALLOONUSERCLICK:
            PushTrayEvent(NOTIFICATION_BALLOON_CLICKED);
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

void PushTrayEvent(trayEvent event)
{
    TrayEventFIFO(event);
    return;
}

trayEvent GetTrayEvent(void)
{
    return TrayEventFIFO(NO_TRAY_EVENT);
}

trayEvent TrayEventFIFO(trayEvent event)
{
    static trayEvent events_queue[TRAY_BUFFER_LENGTH] = { NO_TRAY_EVENT };
    static int read = 0;
    static int write = 0;

    if (event == NO_TRAY_EVENT) // Read
    {
        // Empty check
        if (read == write)
        {
            return NO_TRAY_EVENT;
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

        return NO_TRAY_EVENT;
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
void RemoveTrayIcon(void)
{
    puts("Removing tray icon");
    Shell_NotifyIconA(NIM_DELETE, &ICON_DATA);
    DestroyWindow(TRAY_WINDOW_HANDLE);
    UnregisterClassA("TrayIconClass", TRAY_WINDOW_CLASS.hInstance);
    return;
}
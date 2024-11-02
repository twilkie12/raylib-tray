#include <stdbool.h>
#include <stdio.h>

#define NOGDI
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <shellapi.h>


#define WM_TRAYICON (9009)
#define TRAY_EVENT_BUFFER_LENGTH (5)
#define MAX_CONTEXT_MENU_ITEMS (20)
#define CONTEXT_MENU_END_ID -1
#define CONTEXT_MENU_SEPERATOR_ID -2
#define CONTEXT_MENU_NO_ITEM_SELECTED -1
#define NOTIFY_ICON_NONE "NONE"
#define NOTIFY_ICON_INFO "INFO"
#define NOTIFY_ICON_WARN "WARN"
#define NOTIFY_ICON_ERROR "ERROR"

typedef enum
{
    NO_TRAY_EVENT = 0,
    TRAY_ICON_CLICKED,
    TRAY_ICON_DOUBLE_CLICKED,
    NOTIFICATION_BALLOON_TIMEOUT,
    NOTIFICATION_BALLOON_HIDDEN,
    NOTIFICATION_BALLOON_CLICKED,

}trayEvent;

typedef struct
{
    const char* item_name;
    int item_id;
}contextMenuItem;

static HWND TRAY_WINDOW_HANDLE = NULL;
static NOTIFYICONDATAA ICON_DATA = { 0 };
static WNDCLASSA TRAY_WINDOW_CLASS = { 0 };
static HMENU TRAY_POPUP_MENU = NULL;
static int CONTEXT_MENU_ITEM_SELECTED = -1;

bool InitTrayIcon(const char* icon_path, const char* tooltip_text);
void ChangeTrayIcon(const char* icon_path);
void ChangeTrayTooltip(const char* tooltip_text);
void RemoveTrayIcon(void);
void HideTrayIcon(void);
void ShowTrayIcon(void);
bool GetTrayEvent(trayEvent* event);
bool SendNotification(const char* notification_title, const char* notification_text, const char* notify_icon_path, bool sound);
void InitContextMenu(const char* menu_item_names[20], const int menu_item_identifiers[20]);
bool ContextMenuItemSelected(int* menu_item_identifier);

// The static doesn't do anything except make it clear to developers what functions are to be internal
static void GetWindowMessages(void);
static void InitNotificationData(const char* icon_path, const char* tooltip_text, DWORD info_flags);
static bool RegisterTrayWindowClass(void);
static void PlaceTrayIcon(void);
static LRESULT CALLBACK TrayWindowProcess(HWND, UINT, WPARAM, LPARAM);
static trayEvent TrayEventFIFO(trayEvent event);
static void PushTrayEvent(trayEvent event);
static void NotificationFinished(void);
static void DrawContextMenu(void);

// This is important for making sure messages are received even when the raylib window is not initialised
void GetWindowMessages(void)
{
    MSG message;
    // First do the tray icon messages
    while (PeekMessageA(&message, TRAY_WINDOW_HANDLE, WM_TRAYICON, WM_TRAYICON, PM_REMOVE))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }
    // Then the context menu messages
    while (PeekMessageA(&message, TRAY_WINDOW_HANDLE, WM_COMMAND, WM_COMMAND, PM_REMOVE))
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
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
    PlaceTrayIcon();
    
    return true;
}

void InitNotificationData(const char* icon_path, const char* tooltip_text, DWORD info_flags)
{
    // The fixed menu_item_names first
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
        ICON_DATA.hIcon = LoadImageA(NULL, icon_path, IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_LOADTRANSPARENT | LR_DEFAULTSIZE);
        if (ICON_DATA.hIcon == NULL)
        {
            printf("Error code %s loading icon file 0x%X, check it is .ico\n", icon_path, GetLastError());
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

void ChangeTrayIcon(const char* icon_path)
{
    if (icon_path == NULL)
    {
        ICON_DATA.hIcon = NULL;
    }
    else
    {
        // Need to check that it is an .ico or .bmp file, other file types will not work
        // Also need to confirm what the x and y sizes should be for .ico files
        ICON_DATA.hIcon = LoadImageA(NULL, icon_path, IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_LOADTRANSPARENT | LR_DEFAULTSIZE);
        if (ICON_DATA.hIcon == NULL)
        {
            printf("Error code 0x%X loading icon file %s, check the path and that it is an .ico file\n", GetLastError(), icon_path);
        }
    }
    
    Shell_NotifyIconA(NIM_MODIFY, &ICON_DATA);
    return;
}

void ChangeTrayTooltip(const char* tooltip_text)
{
    if (tooltip_text == NULL)
    {
        *ICON_DATA.szTip = '\0';
    }
    else
    {
        strcpy_s(ICON_DATA.szTip, 128, tooltip_text);
        ICON_DATA.szTip[127] = '\0';
    }

    Shell_NotifyIconA(NIM_MODIFY, &ICON_DATA);
    return;
}

// Placing the Icon
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
            // Don't care about any of these for now
            break;
        case WM_RBUTTONUP:
            DrawContextMenu();
            break;
        // These all get passed back to the user using a queue to try and respect the immediate mode operation of Raylib
        case WM_LBUTTONDBLCLK:
            PushTrayEvent(TRAY_ICON_DOUBLE_CLICKED);
            break;
        case NIN_BALLOONHIDE:
            NotificationFinished();
            PushTrayEvent(NOTIFICATION_BALLOON_HIDDEN);
            break;
        case NIN_BALLOONTIMEOUT:
            NotificationFinished();
            PushTrayEvent(NOTIFICATION_BALLOON_TIMEOUT);
            break;
        case WM_LBUTTONUP:
            PushTrayEvent(TRAY_ICON_CLICKED);
            break;
        case NIN_BALLOONUSERCLICK:
            NotificationFinished();
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
        CONTEXT_MENU_ITEM_SELECTED = LOWORD(wParam);
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

bool GetTrayEvent(trayEvent* event)
{
    GetWindowMessages();
    *event = TrayEventFIFO(NO_TRAY_EVENT);
    if (*event == NO_TRAY_EVENT)
    {
        return false;
    }
    else
    {
        return true;
    }
}

trayEvent TrayEventFIFO(trayEvent event)
{
    static trayEvent events_queue[TRAY_EVENT_BUFFER_LENGTH] = { NO_TRAY_EVENT };
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
            if (read >= TRAY_EVENT_BUFFER_LENGTH)
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
        if (write >= TRAY_EVENT_BUFFER_LENGTH)
        {
            write = 0;
        }

        // Overflow, shove read forwards
        if (write == read)
        {
            read++;
            if (read >= TRAY_EVENT_BUFFER_LENGTH)
            {
                read = 0;
            }
        }

        return NO_TRAY_EVENT;
    }
}

// Notification balloon
bool SendNotification(const char* notification_title, const char* notification_text, const char* notify_icon_path, bool sound)
{
    ICON_DATA.dwInfoFlags = (sound ? 0 : NIIF_NOSOUND) | NIIF_LARGE_ICON | NIIF_RESPECT_QUIET_TIME;
 
    if (notify_icon_path == NULL)
    {
        ICON_DATA.dwInfoFlags |= NIIF_USER;
        ICON_DATA.hBalloonIcon = ICON_DATA.hIcon;
    }
    else if (memcmp(notify_icon_path, NOTIFY_ICON_NONE, 4) == 0)
    {
        ICON_DATA.dwInfoFlags |= NIIF_NONE;
    }
    else if (memcmp(notify_icon_path, NOTIFY_ICON_INFO, 4) == 0)
    {
        ICON_DATA.dwInfoFlags |= NIIF_INFO;
    }
    else if (memcmp(notify_icon_path, NOTIFY_ICON_WARN, 4) == 0)
    {
        ICON_DATA.dwInfoFlags |= NIIF_WARNING;
    }
    else if (memcmp(notify_icon_path, NOTIFY_ICON_ERROR, 5) == 0)
    {
        ICON_DATA.dwInfoFlags |= NIIF_ERROR;
    }
    else
    {
        ICON_DATA.hBalloonIcon = LoadImageA(NULL, notify_icon_path, IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_LOADTRANSPARENT | LR_DEFAULTSIZE);
        if (ICON_DATA.hBalloonIcon == NULL)
        {
            printf("Error code 0x%X loading icon file %s, check the path and that it is an .ico file\n", GetLastError(), notify_icon_path);
            if (ICON_DATA.hIcon == NULL)
            {
                ICON_DATA.dwInfoFlags |= NIIF_INFO;
            }
            else
            {
                ICON_DATA.dwInfoFlags |= NIIF_USER;
                ICON_DATA.hBalloonIcon = ICON_DATA.hIcon;
            }
        }
        else
        {
            ICON_DATA.dwInfoFlags |= NIIF_USER;
        }
    }


    if (notification_text == NULL)
    {
        *ICON_DATA.szInfo = '\0';
    }
    else
    {
        strcpy_s(ICON_DATA.szInfo, 255, notification_text);
        ICON_DATA.szInfo[255] = '\0';
    }

    if (notification_title == NULL)
    {
        *ICON_DATA.szInfoTitle = '\0';
    }
    else
    {
        strcpy_s(ICON_DATA.szInfoTitle, 63, notification_title);
        ICON_DATA.szInfoTitle[63] = '\0';
    }

    ICON_DATA.uFlags = ICON_DATA.uFlags | NIF_INFO;

    return Shell_NotifyIconA(NIM_MODIFY, &ICON_DATA);
}

void NotificationFinished(void)
{
    ICON_DATA.uFlags = ICON_DATA.uFlags & ~NIF_INFO;
    return;
}

// Context menu 
void InitContextMenu(const char** menu_item_name, const int* menu_item_identifier)
{
    if (TRAY_POPUP_MENU != NULL)
    {
        DestroyMenu(TRAY_POPUP_MENU);
        TRAY_POPUP_MENU = NULL;
    }

    // I'm going to make the assumption that people are hard-coding their menus can checking them in development so not going to check for errors in menu creation
    TRAY_POPUP_MENU = CreatePopupMenu();

    for ( int i = 0; i < MAX_CONTEXT_MENU_ITEMS; i++)
    {
        if (menu_item_identifier[i] == CONTEXT_MENU_END_ID)
        {
            break;
        }
        else if (menu_item_identifier[i] == CONTEXT_MENU_SEPERATOR_ID)
        {
            AppendMenuA(TRAY_POPUP_MENU, MF_SEPARATOR, 0, NULL);
        }
        else
        {
            AppendMenuA(TRAY_POPUP_MENU, MF_ENABLED | MF_STRING | MF_UNCHECKED, menu_item_identifier[i], menu_item_name[i]);
        }
    }

    return;
}

void DrawContextMenu(void)
{
    POINT cursor_position;
    GetCursorPos(&cursor_position);
    UINT popup_menu_options = TPM_NONOTIFY | TPM_LEFTBUTTON | TPM_NOANIMATION;

    int x_pixels = GetSystemMetrics(SM_CXSCREEN);
    int y_pixels = GetSystemMetrics(SM_CYSCREEN);
    
    // Horizontal alignement
    if (cursor_position.x < (x_pixels / 2))
    {
        popup_menu_options |= TPM_LEFTALIGN;
    }
    else
    {
        popup_menu_options |= TPM_RIGHTALIGN;
    }

    // Vertical alignement
    if (cursor_position.y < (y_pixels / 2))
    {
        popup_menu_options |= TPM_TOPALIGN;
    }
    else
    {
        popup_menu_options |= TPM_BOTTOMALIGN;
    }

    SetForegroundWindow(TRAY_WINDOW_HANDLE);
    TrackPopupMenu(TRAY_POPUP_MENU, popup_menu_options, cursor_position.x, cursor_position.y, 0, TRAY_WINDOW_HANDLE, NULL);
    return;
}

bool ContextMenuItemSelected(int* menu_identifier)
{
    // Make sure there are no messages queued
    GetWindowMessages();
    if (CONTEXT_MENU_ITEM_SELECTED != CONTEXT_MENU_NO_ITEM_SELECTED)
    {
        *menu_identifier = CONTEXT_MENU_ITEM_SELECTED;
        CONTEXT_MENU_ITEM_SELECTED = CONTEXT_MENU_NO_ITEM_SELECTED;
        return true;
    }
    else
    {
        return false;
    }
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
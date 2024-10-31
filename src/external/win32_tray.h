#ifndef _WIN32_TRAY_H_
#define _WIN32_TRAY_H_

#include <stdbool.h>
#include <stdio.h>

#define NOGDI
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <shellapi.h>


#define WM_TRAYICON (0x9009)

typedef enum
{
	no_event,
	tray_icon_clicked,
	tray_icon_double_clicked,
	balloon_timed_out,
	balloon_hidden,
	balloon_clicked,
	unknown
}trayEvent;

void TrayTest(char* test_string);

bool InitTrayIcon(char* icon_path, char* tooltip_text, DWORD info_flags);
trayEvent GetTrayAction(void);


static void InitNotificationStruct(char* tooltip_text, char* icon_path, DWORD info_flags);
static bool RegisterTrayWindowClass(void);
static void RemoveTrayWindowClass(void);
static void PlaceTrayIcon(void);

static LRESULT CALLBACK TrayWindowProcess(HWND, UINT, WPARAM, LPARAM);
static trayEvent TrayActionFIFO(trayEvent event);
static void PushTrayAction(trayEvent event);

void TrayPopupMenu(void);
static void PopupMenuItemSelected(const UINT16 item);

#endif // !_WIN32_TRAY_H_

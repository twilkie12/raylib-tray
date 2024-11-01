#ifndef _WIN32_TRAY_H_
#define _WIN32_TRAY_H_

#include <stdbool.h>
#include <stdio.h>


#define WM_TRAYICON (9009)

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

void TrayTest(const char* test_string);

bool InitTrayIcon(const char* icon_path, const char* tooltip_text);
trayEvent GetTrayAction(void);
void RemoveTrayIcon(void);

#endif // !_WIN32_TRAY_H_
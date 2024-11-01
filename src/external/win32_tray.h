#ifndef _WIN32_TRAY_H_
#define _WIN32_TRAY_H_

void TrayTest(const char* test_string);

bool InitTrayIcon(const char* icon_path, const char* tooltip_text);
void RemoveTrayIcon(void);
void HideTrayIcon(void);
void ShowTrayIcon(void);
trayEvent GetTrayEvent(void);

#endif // !

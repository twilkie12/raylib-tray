/*******************************************************************************************
*
*   raylib [core] example - Basic window
*
*   Welcome to raylib!
*
*   To test examples, just press F6 and execute raylib_compile_execute script
*   Note that compiled executable is placed in the same folder as .c file
*
*   You can find all basic examples on C:\raylib\raylib\examples folder or
*   raylib official webpage: www.raylib.com
*
*   Enjoy using raylib. :)
*
*   Example originally created with raylib 1.0, last time updated with raylib 1.0
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2013-2024 Ramon Santamaria (@raysan5)
*
********************************************************************************************/

#include "raylib.h"
#include <stdio.h>

const char* raylib_logo = "../raylib.ico";
const char* raylib_orange_logo = "./resources/raylib_orange.ico";
const char* raylib_blue_logo = "./resources/raylib_blue.ico";

bool GLOBAL_QUIT = false;
bool MINIMISED_TO_TRAY = false;

void ProcessTrayEvents(trayEvent tray_event);
void ProcessContextMenuSelection(selection_id);

contextMenuItem menu_items[4] = { 0 };


//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    InitTrayIcon(raylib_logo, "Tooltip text");
    // Context menu
    const char* menu_items[] = { "Base", "Orange", "Blue", NULL, "Hide/Unhide", NULL, "Quit", NULL };
    const int menu_itentifiers[] = { 0, 1, 2, CONTEXT_MENU_SEPERATOR_ID, 3, CONTEXT_MENU_SEPERATOR_ID, 10, CONTEXT_MENU_END_ID };
    // This and the implementation of ProcessContextMenuSelection both feel a bit janky
    InitContextMenu(menu_items, menu_itentifiers);

    const int screenWidth = 850;
    const int screenHeight = 400;

    //--------------------------------------------------------------------------------------
    int context_menu_id = 0;
    trayEvent tray_event = NO_TRAY_EVENT;

    // Start the GUI
    while (!GLOBAL_QUIT)
    {
        if (!MINIMISED_TO_TRAY)
        {
            // Init window
            InitWindow(screenWidth, screenHeight, "raylib [core] example - tray icon");
            SetTargetFPS(60);               // Set our game to run at 60 frames-per-second

            // Main window loop
            while (!WindowShouldClose() && !MINIMISED_TO_TRAY && !GLOBAL_QUIT)
            {
                //----------------------------------------------------------------------------------
                // TODO: Update your variables here
                //----------------------------------------------------------------------------------
                // Go through and process all the tray events
                while (GetTrayEvent(&tray_event))
                {
                    ProcessTrayEvents(tray_event);
                }

                // Check if a context menu item has been selected
                if (ContextMenuItemSelected(&context_menu_id))
                {
                    ProcessContextMenuSelection(context_menu_id);
                }

                // Draw
                //----------------------------------------------------------------------------------
                BeginDrawing();

                ClearBackground(RAYWHITE);

                DrawText("Tray icon and notifications example", 100, 190, 32, GRAY);

                EndDrawing();
            }

            // De-Initialization
            //--------------------------------------------------------------------------------------
            MINIMISED_TO_TRAY = true;
            CloseWindow();        // Close window and OpenGL context
        }
        else
        {

            // Go through and process all the tray events
            while (GetTrayEvent(&tray_event))
            {
                ProcessTrayEvents(tray_event);
            }

            // Check if a context menu item has been selected
            if (ContextMenuItemSelected(&context_menu_id))
            {
                ProcessContextMenuSelection(context_menu_id);
            }
            Sleep(100);
        }
    }

    RemoveTrayIcon();
    //--------------------------------------------------------------------------------------

    return 0;
}

void ProcessTrayEvents(trayEvent tray_event)
{
    switch (tray_event)
    {
    case (TRAY_ICON_CLICKED):
        if (!MINIMISED_TO_TRAY)
        {
            puts("Tray icon clicked, bring raylib window to front");
            SetWindowFocused();
        }
        else
        {
            MINIMISED_TO_TRAY = false;
        }
        break;
    case (TRAY_ICON_DOUBLE_CLICKED):
        puts("Tray icon double clicked, sending notification");
        SendNotification("Notification title", "Text for the notification body", NOTIFY_USE_TRAY_ICON, false);
        break;
    case (NOTIFICATION_BALLOON_TIMEOUT):
        puts("Balloon timed out - Occurs when there is either no interaction with the notification or the user clicks to dismiss it");
        break;
    case (NOTIFICATION_BALLOON_HIDDEN):
        puts("Balloon hidden - Occurs when notification is sent and user is in \"Quiet Time\"");
        break;
    case (NOTIFICATION_BALLOON_CLICKED):
        puts("Balloon clicked");
        break;
    default:
        puts("Unexpected value from tray event");
        break;
    }

    return;
}

void ProcessContextMenuSelection(selection_id)
{
    switch (selection_id)
    {
    case 0:
        puts("Changing to normal logo");
        ChangeTrayIcon(raylib_logo);
        ChangeTrayTooltip("Base colour");
        break;
    case 1:
        puts("Changing to orange logo");
        ChangeTrayIcon(raylib_orange_logo);
        ChangeTrayTooltip("Orange");
        break;
    case 2:
        puts("Changing to blue logo");
        ChangeTrayIcon(raylib_blue_logo);
        ChangeTrayTooltip("Blue");
        break;
    case 3:
        if (MINIMISED_TO_TRAY)
        {
            puts("Un-hiding window");
            MINIMISED_TO_TRAY = false;
        }
        else
        {
            puts("Hiding window");
            MINIMISED_TO_TRAY = true;
        }
        break;
    case 10:
        puts("Quit selected from menu");
        GLOBAL_QUIT = true;
        break;
    default:
        puts("Unknown context menu item selected");
        break;
    }
    return;
}
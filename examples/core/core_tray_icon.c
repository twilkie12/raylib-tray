/*******************************************************************************************
*
*   raylib [core] example - Tray icon
*
*   Example originally created with raylib 4.5, last time updated with raylib 4.5
*
*   Example contributed by T Wilkie (@twilkie12) and reviewed by Ramon Santamaria (@raysan5)
*
*   Example licensed under an unmodified zlib/libpng license, which is an OSI-certified,
*   BSD-like license that allows static linking with closed source software
*
*   Copyright (c) 2023 T Wilkie (@twilkie12)
*
********************************************************************************************/

#include "raylib.h"
#include <stdio.h>

const char* raylib_logo = "../raylib.ico";
const char* raylib_orange_logo = "./resources/raylib_orange.ico";
const char* raylib_blue_logo = "./resources/raylib_blue.ico";

bool GLOBAL_QUIT = false;
bool MINIMISED_TO_TRAY = false;

void TrayAndContextMenuEvents(void);
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
    const char* menu_item_names[] = { "Base", "Orange", "Blue", NULL, "Hide/Unhide", NULL, "Quit", NULL };
    const int menu_item_identifiers[] = { 0, 1, 2, CONTEXT_MENU_SEPERATOR_ID, 3, CONTEXT_MENU_SEPERATOR_ID, 10, CONTEXT_MENU_END_ID };
    // This and the implementation of ProcessContextMenuSelection both feel a bit janky but not sure how to do it better, a struct doesn't really work nicely either
    InitContextMenu(menu_item_names, menu_item_identifiers);

    const int screenWidth = 850;
    const int screenHeight = 400;

    //--------------------------------------------------------------------------------------

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
                // Does all the processing for the tray icon and tray context menu, call every draw loop
                TrayAndContextMenuEvents();

                //----------------------------------------------------------------------------------
                // TODO: Update your variables here
                //----------------------------------------------------------------------------------

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
            // This isn't the best way to do it, I might look at adding HideWindow() to Raylib in future
            CloseWindow();        // Close window and OpenGL context
        }
        else
        {
            // Even when the main Raylib window is closed keep checking the tray window for events
            TrayAndContextMenuEvents();
            // Rate limiting, a 0.1s response time feel responsive for the tray
            Sleep(100);
        }
    }

    RemoveTrayIcon();
    //--------------------------------------------------------------------------------------

    return 0;
}

void TrayAndContextMenuEvents(void)
{
    int context_menu_id = 0;
    trayEvent tray_event = NO_TRAY_EVENT;

    // Go through and process all the tray events
    while (GetTrayEvent(&tray_event))
    {
        ProcessTrayEvents(tray_event);
    }

    // Check if a context menu item has been selected and act on it
    if (ContextMenuItemSelected(&context_menu_id))
    {
        ProcessContextMenuSelection(context_menu_id);
    }
    return;
}

// This is all the events from acting on the tray icon and notification ballon
// You should implement your own actions here for each trayEvent
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

// When an item from the context menu is selected it will return the item id you associated with that item
// This is the place to implement your own actions for each menu item
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
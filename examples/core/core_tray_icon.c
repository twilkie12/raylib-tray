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

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(void)
{
    InitTrayIcon("C:/Users/Wilkie/Documents/RayGUI_dev/raylib/logo/raylib.ico", "Tooltip text");
    /*
    Sleep(2000);
    puts("Hiding icon");
    HideTrayIcon();
    Sleep(2000);
    puts("Showing icon");
    ShowTrayIcon();
    */


    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 850;
    const int screenHeight = 400;

    InitWindow(screenWidth, screenHeight, "raylib [core] example - tray icon");

    SetTargetFPS(60);               // Set our game to run at 60 frames-per-second
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // TODO: Update your variables here
        //----------------------------------------------------------------------------------
        trayEvent tray_event;
        do
        {
            tray_event = GetTrayEvent();

            switch (tray_event)
            {
            case (NO_TRAY_EVENT):
                break;
            case (TRAY_ICON_CLICKED):
                puts("Tray icon clicked");
                break;
            case (TRAY_ICON_DOUBLE_CLICKED):
                puts("Tray icon double clicked");
                break;
            case (NOTIFICATION_BALLOON_TIMEOUT):
                puts("Balloon timed out");
                break;
            case (NOTIFICATION_BALLOON_HIDDEN):
                puts("Balloon hidden");
                break;
            case (NOTIFICATION_BALLOON_CLICKED):
                puts("Balloon clicked");
                break;
            default:
                puts("Error with tray event fetch");
                break;
            }

        } while (tray_event != NO_TRAY_EVENT);

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(RAYWHITE);

            DrawText("Tray icon test", 190, 200, 20, BLACK);

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    RemoveTrayIcon();
    //--------------------------------------------------------------------------------------

    return 0;
}
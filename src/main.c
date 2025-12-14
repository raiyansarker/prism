/*
 * Pac-Man Game
 * 
 * A simple console-based Pac-Man game.
 * Use WASD to move, R to restart, Q to quit.
 */

#include <stdio.h>
#include <stdlib.h>

#include "app.h"
#include "platform.h"

// How often things happen (in milliseconds)
#define GAME_TICK_MS    400   // Ghosts move every 400ms
#define INPUT_POLL_MS   20    // Check for keyboard every 20ms

int main() {
    // Setup the terminal for the game
    platform_init();
    platform_enter_fullscreen();

    // Create the game
    struct App app = app_create();

    long last_tick = platform_time_ms();
    long last_poll = platform_time_ms();

    // Main game loop
    while (app.running) {
        long now = platform_time_ms();

        // Check for keyboard input
        if (now - last_poll >= INPUT_POLL_MS) {
            while (platform_kbhit()) {
                int ch = platform_getch();
                app_handle_input(&app, ch);
                if (app.running == false) {
                    break;
                }
            }
            last_poll = now;
        }

        // Update game (move ghosts, etc)
        if (now - last_tick >= GAME_TICK_MS) {
            app_update(&app);
            last_tick = now;
        }

        // Draw the game
        app_render(&app);
    }

    // Clean up
    app_destroy(&app);
    platform_exit_fullscreen();

    return 0;
}

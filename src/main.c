/*
 * Console Pac-Man - Main Entry Point
 *
 * Cross-platform terminal game.
 *
 * Standard C libraries used:
 *   - stdio.h, stdlib.h, time.h
 *
 * Platform-specific handling in platform.c:
 *   - Windows: conio.h + Windows Console API
 *   - Unix/Linux/macOS: termios.h + POSIX calls
 *
 * See platform.h for detailed documentation.
 */

#include <stdio.h>
#include <stdlib.h>

#include "app.h"
#include "platform.h"

/* Game timing constants (milliseconds) */
#define GAME_TICK_MS    400   /* Ghost movement interval */
#define INPUT_POLL_MS   20    /* How often to check for input */

int main(void) {
    /* Initialize platform-specific terminal handling */
    platform_init();
    platform_enter_fullscreen();

    App app = app_create();

    long last_tick = platform_time_ms();
    long last_poll = platform_time_ms();

    /* Main game loop - never blocks */
    while (app.running) {
        long now = platform_time_ms();

        /* Poll for input at regular intervals */
        if (now - last_poll >= INPUT_POLL_MS) {
            while (platform_kbhit()) {
                int ch = platform_getch();
                app_handle_input(&app, ch);
                if (!app.running) break;
            }
            last_poll = now;
        }

        /* Update game state at fixed tick rate */
        if (now - last_tick >= GAME_TICK_MS) {
            app_update(&app);
            last_tick = now;
        }

        /* Render if needed (only redraws on changes) */
        app_render(&app);
    }

    /* Cleanup */
    app_destroy(&app);
    platform_exit_fullscreen();

    return EXIT_SUCCESS;
}

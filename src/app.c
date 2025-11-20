#include "app.h"

#include <stdio.h>
#include <string.h>

App app_create(void) {
    App app = {
        .tick_count = 0u,
        .running = true,
    };
    return app;
}

void app_destroy(App *app) {
    if (!app) {
        return;
    }
    app->running = false;
}

void app_render(const App *app) {
    if (!app) {
        return;
    }

    puts("================================");
    puts(" Prism TUI Playground ");
    puts("--------------------------------");
    printf(" Tick count : %u\n", app->tick_count);
    puts(" Commands   : [n] next tick | [q] quit");
    puts("================================");
    fputs("> ", stdout);
    fflush(stdout);
}

void app_handle_input(App *app, const char *line) {
    if (!app || !line) {
        return;
    }

    if (line[0] == 'q' || line[0] == 'Q') {
        puts("Goodbye!\n");
        app->running = false;
        return;
    }

    if (line[0] == 'n' || line[0] == 'N') {
        ++app->tick_count;
        printf("Advancing to tick %u...\n", app->tick_count);
        return;
    }

    puts("Unknown command. Try 'n' or 'q'.");
}

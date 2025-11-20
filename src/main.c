#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app.h"

static void trim_newline(char *line) {
    if (!line) {
        return;
    }
    size_t len = strlen(line);
    if (len == 0) {
        return;
    }
    if (line[len - 1] == '\n') {
        line[len - 1] = '\0';
    }
}

int main(void) {
    App app = app_create();

    puts("Welcome to the Prism TUI playground!");
    puts("You can safely experiment here before adding a real terminal UI library.\n");

    char input_buffer[64];
    while (app.running) {
        app_render(&app);

        if (!fgets(input_buffer, sizeof(input_buffer), stdin)) {
            perror("fgets");
            break;
        }

        trim_newline(input_buffer);
        app_handle_input(&app, input_buffer);
    }

    app_destroy(&app);
    return EXIT_SUCCESS;
}

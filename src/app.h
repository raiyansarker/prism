#ifndef APP_H
#define APP_H

#include <stdbool.h>

typedef struct App {
    unsigned int tick_count;
    bool running;
} App;

App app_create(void);
void app_destroy(App *app);
void app_render(const App *app);
void app_handle_input(App *app, const char *line);

#endif // APP_H

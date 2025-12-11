/*
 * Console Pac-Man Game
 *
 * Cross-platform terminal game using platform abstraction layer.
 * See platform.h for platform-specific documentation.
 *
 * Standard C libraries: stdio.h, stdlib.h, string.h, time.h
 */

#ifndef APP_H
#define APP_H

#include <stdbool.h>

typedef struct Position {
    int row;
    int col;
} Position;

#define MAP_HEIGHT 15
#define MAP_WIDTH 40
#define FRAME_BUFFER_SIZE 16384
#define NUM_GHOSTS 4

/* Ghost behavior types */
typedef enum GhostType {
    GHOST_CHASER,    /* Red - directly chases Pac-Man */
    GHOST_AMBUSHER,  /* Pink - tries to get ahead of Pac-Man */
    GHOST_FLANKER,   /* Cyan - tries to flank from the side */
    GHOST_RANDOM     /* Orange - moves randomly */
} GhostType;

typedef struct Ghost {
    Position pos;
    Position start;
    GhostType type;
    int last_dir;    /* Last direction moved (0-3), helps avoid oscillation */
} Ghost;

typedef struct App {
    unsigned int tick_count;
    unsigned int score;
    unsigned int high_score;  /* Persisted highest score */
    unsigned int lives;
    unsigned int max_lives;  /* Maximum lives for display */
    unsigned int dots_remaining;
    bool running;
    bool won;
    bool game_over;  /* Game over state (don't exit, show banner) */
    bool needs_redraw;
    Position pacman;
    Position pacman_start;
    int pacman_dir;  /* Last direction Pac-Man moved (for ambush ghost) */
    Ghost ghosts[NUM_GHOSTS];
    char map[MAP_HEIGHT][MAP_WIDTH + 1];
    char frame_buffer[FRAME_BUFFER_SIZE];
    int term_rows;
    int term_cols;
} App;

App app_create(void);
void app_destroy(App *app);
void app_render(App *app);
void app_handle_input(App *app, int cmd);
void app_update(App *app);

#endif /* APP_H */

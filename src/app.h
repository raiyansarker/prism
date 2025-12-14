#include <stdbool.h>

// Position on the map
struct Position {
    int row;
    int col;
};

// Game settings
#define MAP_HEIGHT 15
#define MAP_WIDTH 40
#define FRAME_BUFFER_SIZE 16384
#define NUM_GHOSTS 4

// Different ghost types
#define GHOST_CHASER 0    // Red ghost - chases Pac-Man directly
#define GHOST_AMBUSHER 1  // Pink ghost - tries to get ahead
#define GHOST_FLANKER 2   // Cyan ghost - tries to flank
#define GHOST_RANDOM 3    // Orange ghost - moves randomly

// Ghost structure
struct Ghost {
    struct Position pos;
    struct Position start;
    int type;
    int last_dir;  // Last direction moved (0-3)
};

// Main game structure
struct App {
    unsigned int score;
    unsigned int high_score;
    unsigned int lives;
    unsigned int max_lives;
    unsigned int dots_remaining;
    bool running;
    bool won;
    bool game_over;
    bool needs_redraw;
    struct Position pacman;
    struct Position pacman_start;
    int pacman_dir;
    struct Ghost ghosts[NUM_GHOSTS];
    char map[MAP_HEIGHT][MAP_WIDTH + 1];
    char frame_buffer[FRAME_BUFFER_SIZE];
    int term_rows;
    int term_cols;
};

// Function declarations
struct App app_create();
void app_destroy(struct App *app);
void app_render(struct App *app);
void app_handle_input(struct App *app, int cmd);
void app_update(struct App *app);

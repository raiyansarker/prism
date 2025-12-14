#include "app.h"
#include "platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Colors for the terminal
#define ESC "\033"
#define COLOR_RESET   ESC "[0m"
#define COLOR_BOLD    ESC "[1m"
#define COLOR_YELLOW  ESC "[33m"
#define COLOR_RED     ESC "[31m"
#define COLOR_BLUE    ESC "[34m"
#define COLOR_WHITE   ESC "[37m"
#define COLOR_CYAN    ESC "[36m"
#define COLOR_GREEN   ESC "[32m"
#define COLOR_MAGENTA ESC "[35m"

// The maze layout
const char *LEVEL_TEMPLATE[MAP_HEIGHT] = {
    "########################################",
    "#.........#..........#..........#......#",
    "#.###.###.#.###.####.#.####.###.#.###..#",
    "#.....#.....#...#....#....#...#.....#..#",
    "#####.#.#####.#.#.##.#.##.#.#.#####.#.##",
    "#.....#.......#.#.##.#.##.#.#.......#..#",
    "#.#########.###.#....#....#.###.#######.#",
    "#.............#.#.##.#.##.#.#...........#",
    "#.###.#######.#.#.##.#.##.#.#.#######.###",
    "#...#.#.......#.#....#....#.#.......#...#",
    "#.#.#.#.#######.######.######.#######.#.#",
    "#.#.#.#.........#....#........#.......#.#",
    "#.#.#.###########.##.#.########.#######.#",
    "#.#.....................................#",
    "########################################",
};

// Copy the level template to the game map
void copy_level(char map[MAP_HEIGHT][MAP_WIDTH + 1]) {
    int r;
    for (r = 0; r < MAP_HEIGHT; r++) {
        strncpy(map[r], LEVEL_TEMPLATE[r], MAP_WIDTH + 1);
        map[r][MAP_WIDTH] = '\0';
    }
}

// Count dots remaining on the map
unsigned int count_dots(const char map[MAP_HEIGHT][MAP_WIDTH + 1]) {
    unsigned int dots = 0;
    int r, c;
    for (r = 0; r < MAP_HEIGHT; r++) {
        for (c = 0; c < MAP_WIDTH; c++) {
            if (map[r][c] == '.') {
                dots = dots + 1;
            }
        }
    }
    return dots;
}

// Check if a position is walkable (not a wall)
bool is_walkable(const struct App *app, int row, int col) {
    if (row < 0 || row >= MAP_HEIGHT || col < 0 || col >= MAP_WIDTH) {
        return false;
    }
    char c = app->map[row][col];
    if (c == '#') {
        return false;
    }
    return true;
}

// Reset pac-man and ghosts to starting positions
void reset_positions(struct App *app) {
    int i;
    app->pacman.row = app->pacman_start.row;
    app->pacman.col = app->pacman_start.col;
    app->pacman_dir = 0;
    for (i = 0; i < NUM_GHOSTS; i++) {
        app->ghosts[i].pos.row = app->ghosts[i].start.row;
        app->ghosts[i].pos.col = app->ghosts[i].start.col;
        app->ghosts[i].last_dir = -1;
    }
}

// Move pac-man in a direction
void move_pacman(struct App *app, int dr, int dc) {
    int nr = app->pacman.row + dr;
    int nc = app->pacman.col + dc;
    
    if (is_walkable(app, nr, nc) == false) {
        return;
    }
    
    app->pacman.row = nr;
    app->pacman.col = nc;
    app->needs_redraw = true;

    // Remember which direction pac-man is moving
    if (dr == -1) {
        app->pacman_dir = 0;  // up
    } else if (dr == 1) {
        app->pacman_dir = 1;  // down
    } else if (dc == -1) {
        app->pacman_dir = 2;  // left
    } else if (dc == 1) {
        app->pacman_dir = 3;  // right
    }

    // Eat dot if there is one
    if (app->map[nr][nc] == '.') {
        app->map[nr][nc] = ' ';
        app->score = app->score + 1;
        app->dots_remaining = app->dots_remaining - 1;
        platform_play_sound(SOUND_EAT_DOT);
        
        // Update high score
        if (app->score > app->high_score) {
            app->high_score = app->score;
        }
    }
}

// Calculate distance between two points (Manhattan distance)
int get_distance(int r1, int c1, int r2, int c2) {
    int dr = r1 - r2;
    int dc = c1 - c2;
    
    // Get absolute values
    if (dr < 0) {
        dr = -dr;
    }
    if (dc < 0) {
        dc = -dc;
    }
    
    return dr + dc;
}

// Get opposite direction to avoid going back
int get_opposite_dir(int dir) {
    if (dir == 0) return 1;  // up -> down
    if (dir == 1) return 0;  // down -> up
    if (dir == 2) return 3;  // left -> right
    if (dir == 3) return 2;  // right -> left
    return -1;
}

// Move a single ghost
void move_single_ghost(struct App *app, struct Ghost *ghost) {
    // Direction offsets: up, down, left, right
    int dir_row[4] = {-1, 1, 0, 0};
    int dir_col[4] = {0, 0, -1, 1};
    
    int valid_dirs[4];
    int valid_count = 0;
    int i;
    
    // Find all valid directions (not walls)
    for (i = 0; i < 4; i++) {
        int nr = ghost->pos.row + dir_row[i];
        int nc = ghost->pos.col + dir_col[i];
        if (nr >= 0 && nr < MAP_HEIGHT && nc >= 0 && nc < MAP_WIDTH) {
            if (app->map[nr][nc] != '#') {
                valid_dirs[valid_count] = i;
                valid_count = valid_count + 1;
            }
        }
    }
    
    if (valid_count == 0) {
        return;
    }
    
    // Try not to go backwards
    int opposite = get_opposite_dir(ghost->last_dir);
    int filtered_dirs[4];
    int filtered_count = 0;
    
    for (i = 0; i < valid_count; i++) {
        if (valid_dirs[i] != opposite || valid_count == 1) {
            filtered_dirs[filtered_count] = valid_dirs[i];
            filtered_count = filtered_count + 1;
        }
    }
    
    int chosen_dir = -1;
    int target_row = app->pacman.row;
    int target_col = app->pacman.col;
    
    // Different behavior based on ghost type
    if (ghost->type == GHOST_CHASER) {
        // Red ghost: chase pac-man directly
        int best_dist = 9999;
        for (i = 0; i < filtered_count; i++) {
            int d = filtered_dirs[i];
            int nr = ghost->pos.row + dir_row[d];
            int nc = ghost->pos.col + dir_col[d];
            int dist = get_distance(nr, nc, target_row, target_col);
            if (dist < best_dist) {
                best_dist = dist;
                chosen_dir = d;
            }
        }
    }
    else if (ghost->type == GHOST_AMBUSHER) {
        // Pink ghost: try to get ahead of pac-man
        int ahead_row[4] = {-4, 4, 0, 0};
        int ahead_col[4] = {0, 0, -4, 4};
        
        int aim_r = target_row + ahead_row[app->pacman_dir];
        int aim_c = target_col + ahead_col[app->pacman_dir];
        
        // Keep in bounds
        if (aim_r < 0) aim_r = 0;
        if (aim_r >= MAP_HEIGHT) aim_r = MAP_HEIGHT - 1;
        if (aim_c < 0) aim_c = 0;
        if (aim_c >= MAP_WIDTH) aim_c = MAP_WIDTH - 1;
        
        int best_dist = 9999;
        for (i = 0; i < filtered_count; i++) {
            int d = filtered_dirs[i];
            int nr = ghost->pos.row + dir_row[d];
            int nc = ghost->pos.col + dir_col[d];
            int dist = get_distance(nr, nc, aim_r, aim_c);
            if (dist < best_dist) {
                best_dist = dist;
                chosen_dir = d;
            }
        }
    }
    else if (ghost->type == GHOST_FLANKER) {
        // Cyan ghost: try to come from the side
        int flank_r = target_row;
        int flank_c = target_col;
        
        if (app->pacman_dir == 0 || app->pacman_dir == 1) {
            // Pac-man moving up/down, flank from side
            if (ghost->pos.col > target_col) {
                flank_c = flank_c + 3;
            } else {
                flank_c = flank_c - 3;
            }
        } else {
            // Pac-man moving left/right, flank from above/below
            if (ghost->pos.row > target_row) {
                flank_r = flank_r + 3;
            } else {
                flank_r = flank_r - 3;
            }
        }
        
        // Keep in bounds
        if (flank_r < 0) flank_r = 0;
        if (flank_r >= MAP_HEIGHT) flank_r = MAP_HEIGHT - 1;
        if (flank_c < 0) flank_c = 0;
        if (flank_c >= MAP_WIDTH) flank_c = MAP_WIDTH - 1;
        
        int best_dist = 9999;
        for (i = 0; i < filtered_count; i++) {
            int d = filtered_dirs[i];
            int nr = ghost->pos.row + dir_row[d];
            int nc = ghost->pos.col + dir_col[d];
            int dist = get_distance(nr, nc, flank_r, flank_c);
            if (dist < best_dist) {
                best_dist = dist;
                chosen_dir = d;
            }
        }
    }
    else {
        // Orange ghost: mostly random with some chasing
        int random_chance = rand() % 100;
        
        if (random_chance < 30) {
            // 30% chance to chase
            int best_dist = 9999;
            for (i = 0; i < filtered_count; i++) {
                int d = filtered_dirs[i];
                int nr = ghost->pos.row + dir_row[d];
                int nc = ghost->pos.col + dir_col[d];
                int dist = get_distance(nr, nc, target_row, target_col);
                if (dist < best_dist) {
                    best_dist = dist;
                    chosen_dir = d;
                }
            }
        } else {
            // 70% random movement
            int random_index = rand() % filtered_count;
            chosen_dir = filtered_dirs[random_index];
        }
    }
    
    // Move the ghost
    if (chosen_dir >= 0) {
        ghost->pos.row = ghost->pos.row + dir_row[chosen_dir];
        ghost->pos.col = ghost->pos.col + dir_col[chosen_dir];
        ghost->last_dir = chosen_dir;
        app->needs_redraw = true;
    }
}

// Move all ghosts
void move_ghosts(struct App *app) {
    int i;
    for (i = 0; i < NUM_GHOSTS; i++) {
        move_single_ghost(app, &app->ghosts[i]);
    }
}

// Check if pac-man hit a ghost
void check_collision(struct App *app) {
    int i;
    for (i = 0; i < NUM_GHOSTS; i++) {
        if (app->pacman.row == app->ghosts[i].pos.row && 
            app->pacman.col == app->ghosts[i].pos.col) {
            
            app->lives = app->lives - 1;
            
            if (app->lives == 0) {
                app->game_over = true;
                platform_play_sound(SOUND_GAME_OVER);
                if (app->score > 0) {
                    platform_save_highscore(app->high_score);
                }
            } else {
                platform_play_sound(SOUND_LOSE_LIFE);
                reset_positions(app);
            }
            app->needs_redraw = true;
            return;
        }
    }
}

// Create and initialize the game
struct App app_create() {
    struct App app;
    
    srand((unsigned int)time(NULL));
    
    // Load saved high score
    unsigned int saved_high_score = platform_load_highscore();
    
    // Initialize all the variables
    app.score = 0;
    app.high_score = saved_high_score;
    app.lives = 3;
    app.max_lives = 3;
    app.dots_remaining = 0;
    app.running = true;
    app.won = false;
    app.game_over = false;
    app.needs_redraw = true;
    app.pacman_start.row = 13;
    app.pacman_start.col = 20;
    app.pacman_dir = 0;
    app.term_rows = 24;
    app.term_cols = 80;
    
    platform_play_sound(SOUND_START);
    
    // Setup red ghost (chaser) - top left
    app.ghosts[0].start.row = 1;
    app.ghosts[0].start.col = 1;
    app.ghosts[0].type = GHOST_CHASER;
    app.ghosts[0].last_dir = -1;
    
    // Setup pink ghost (ambusher) - top right
    app.ghosts[1].start.row = 1;
    app.ghosts[1].start.col = 38;
    app.ghosts[1].type = GHOST_AMBUSHER;
    app.ghosts[1].last_dir = -1;
    
    // Setup cyan ghost (flanker) - center
    app.ghosts[2].start.row = 7;
    app.ghosts[2].start.col = 20;
    app.ghosts[2].type = GHOST_FLANKER;
    app.ghosts[2].last_dir = -1;
    
    // Setup orange ghost (random) - bottom
    app.ghosts[3].start.row = 11;
    app.ghosts[3].start.col = 10;
    app.ghosts[3].type = GHOST_RANDOM;
    app.ghosts[3].last_dir = -1;
    
    copy_level(app.map);
    app.dots_remaining = count_dots(app.map);
    reset_positions(&app);
    platform_get_terminal_size(&app.term_rows, &app.term_cols);
    
    return app;
}

void app_destroy(struct App *app) {
    if (app != NULL) {
        app->running = false;
    }
}

// Draw the game on screen
void app_render(struct App *app) {
    int r, c, i, g;
    
    if (app->needs_redraw == false) {
        return;
    }
    app->needs_redraw = false;

    platform_get_terminal_size(&app->term_rows, &app->term_cols);

    char *buf = app->frame_buffer;
    char *p = buf;

    // Clear screen
    p = p + sprintf(p, "\033[2J\033[H");

    // Calculate padding to center the game
    int content_h = MAP_HEIGHT + 8;
    int content_w = MAP_WIDTH + 4;
    int pad_top = (app->term_rows - content_h) / 2;
    int pad_left = (app->term_cols - content_w) / 2;
    if (pad_top < 0) pad_top = 0;
    if (pad_left < 0) pad_left = 0;

    // Add top padding
    for (i = 0; i < pad_top; i++) {
        *p = '\n';
        p = p + 1;
    }

    // Title
    for (i = 0; i < pad_left; i++) {
        *p = ' ';
        p = p + 1;
    }
    p = p + sprintf(p, COLOR_BOLD COLOR_MAGENTA "================= PAC-MAN =================" COLOR_RESET "\n");

    // Score and lives
    for (i = 0; i < pad_left; i++) {
        *p = ' ';
        p = p + 1;
    }
    p = p + sprintf(p, COLOR_CYAN " Score: " COLOR_BOLD COLOR_GREEN "%u" COLOR_RESET, app->score);
    p = p + sprintf(p, COLOR_CYAN "  Hi: " COLOR_BOLD COLOR_YELLOW "%u" COLOR_RESET, app->high_score);
    p = p + sprintf(p, COLOR_CYAN "  Lives: " COLOR_BOLD);
    if (app->lives > 0) {
        p = p + sprintf(p, COLOR_GREEN "%u" COLOR_RESET "/" COLOR_WHITE "%u", app->lives, app->max_lives);
    } else {
        p = p + sprintf(p, COLOR_RED "0" COLOR_RESET "/" COLOR_WHITE "%u", app->max_lives);
    }
    p = p + sprintf(p, COLOR_RESET COLOR_CYAN "  Dots: " COLOR_BOLD COLOR_WHITE "%u" COLOR_RESET "\n", app->dots_remaining);

    // Controls
    for (i = 0; i < pad_left; i++) {
        *p = ' ';
        p = p + 1;
    }
    p = p + sprintf(p, COLOR_WHITE " WASD" COLOR_CYAN "=Move " COLOR_WHITE "R/Space" COLOR_CYAN "=Restart " COLOR_WHITE "Q" COLOR_CYAN "=Quit" COLOR_RESET "\n");

    // Top line
    for (i = 0; i < pad_left; i++) {
        *p = ' ';
        p = p + 1;
    }
    p = p + sprintf(p, COLOR_BLUE "------------------------------------------" COLOR_RESET "\n");

    // Draw the map
    for (r = 0; r < MAP_HEIGHT; r++) {
        for (i = 0; i < pad_left; i++) {
            *p = ' ';
            p = p + 1;
        }
        *p = ' ';
        p = p + 1;

        for (c = 0; c < MAP_WIDTH; c++) {
            // Check if pac-man is here
            if (app->pacman.row == r && app->pacman.col == c) {
                p = p + sprintf(p, COLOR_BOLD COLOR_YELLOW "C" COLOR_RESET);
            } else {
                // Check if any ghost is here
                int ghost_here = -1;
                for (g = 0; g < NUM_GHOSTS; g++) {
                    if (app->ghosts[g].pos.row == r && app->ghosts[g].pos.col == c) {
                        ghost_here = g;
                        break;
                    }
                }
                
                if (ghost_here >= 0) {
                    // Draw ghost with different colors
                    if (app->ghosts[ghost_here].type == GHOST_CHASER) {
                        p = p + sprintf(p, COLOR_BOLD COLOR_RED "G" COLOR_RESET);
                    } else if (app->ghosts[ghost_here].type == GHOST_AMBUSHER) {
                        p = p + sprintf(p, COLOR_BOLD COLOR_MAGENTA "G" COLOR_RESET);
                    } else if (app->ghosts[ghost_here].type == GHOST_FLANKER) {
                        p = p + sprintf(p, COLOR_BOLD COLOR_CYAN "G" COLOR_RESET);
                    } else {
                        p = p + sprintf(p, COLOR_BOLD COLOR_YELLOW "G" COLOR_RESET);
                    }
                } else {
                    // Draw map tile
                    char tile = app->map[r][c];
                    if (tile == '#') {
                        p = p + sprintf(p, COLOR_BLUE "#" COLOR_RESET);
                    } else if (tile == '.') {
                        p = p + sprintf(p, COLOR_WHITE "." COLOR_RESET);
                    } else {
                        *p = ' ';
                        p = p + 1;
                    }
                }
            }
        }
        *p = '\n';
        p = p + 1;
    }

    // Bottom line
    for (i = 0; i < pad_left; i++) {
        *p = ' ';
        p = p + 1;
    }
    p = p + sprintf(p, COLOR_BLUE "------------------------------------------" COLOR_RESET "\n");

    // Status message at bottom
    for (i = 0; i < pad_left; i++) {
        *p = ' ';
        p = p + 1;
    }
    if (app->won) {
        p = p + sprintf(p, COLOR_BOLD COLOR_GREEN " *** MISSION COMPLETE! All dots cleared! ***" COLOR_RESET "\n");
    } else if (app->game_over) {
        p = p + sprintf(p, COLOR_BOLD COLOR_RED " ========== MISSION FAILED ========== " COLOR_RESET "\n");
        for (i = 0; i < pad_left; i++) {
            *p = ' ';
            p = p + 1;
        }
        p = p + sprintf(p, COLOR_BOLD COLOR_YELLOW " Press SPACE or R to try again!" COLOR_RESET "\n");
    } else {
        p = p + sprintf(p, COLOR_BOLD COLOR_YELLOW " C" COLOR_RESET "=Pac-Man ");
        p = p + sprintf(p, COLOR_BOLD COLOR_RED "G" COLOR_RESET "=Chaser ");
        p = p + sprintf(p, COLOR_BOLD COLOR_MAGENTA "G" COLOR_RESET "=Ambush ");
        p = p + sprintf(p, COLOR_BOLD COLOR_CYAN "G" COLOR_RESET "=Flank ");
        p = p + sprintf(p, COLOR_BOLD COLOR_YELLOW "G" COLOR_RESET "=Random\n");
    }

    // Write everything to screen at once
    platform_write(buf, (int)(p - buf));
}

// Handle keyboard input
void app_handle_input(struct App *app, int cmd) {
    if (cmd == -1) {
        return;
    }

    // Quit game
    if (cmd == 'q' || cmd == 'Q') {
        app->running = false;
        return;
    }

    // Restart game
    if (cmd == 'r' || cmd == 'R' || cmd == ' ') {
        copy_level(app->map);
        app->dots_remaining = count_dots(app->map);
        app->score = 0;
        app->lives = app->max_lives;
        app->won = false;
        app->game_over = false;
        app->running = true;
        reset_positions(app);
        app->needs_redraw = true;
        platform_play_sound(SOUND_START);
        return;
    }

    // Don't move if game is over
    if (app->game_over || app->won) {
        return;
    }

    // Movement controls
    if (cmd == 'w' || cmd == 'W') {
        move_pacman(app, -1, 0);
    } else if (cmd == 's' || cmd == 'S') {
        move_pacman(app, 1, 0);
    } else if (cmd == 'a' || cmd == 'A') {
        move_pacman(app, 0, -1);
    } else if (cmd == 'd' || cmd == 'D') {
        move_pacman(app, 0, 1);
    }

    check_collision(app);
    
    // Check if player won
    if (app->dots_remaining == 0) {
        app->won = true;
        app->needs_redraw = true;
        platform_play_sound(SOUND_WIN);
        platform_save_highscore(app->high_score);
    }
}

// Update game state (called every tick)
void app_update(struct App *app) {
    if (app->running == false || app->won || app->game_over) {
        return;
    }

    move_ghosts(app);
    check_collision(app);

    if (app->dots_remaining == 0 && app->won == false) {
        app->won = true;
        app->needs_redraw = true;
        platform_play_sound(SOUND_WIN);
        platform_save_highscore(app->high_score);
    }
}

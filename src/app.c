#include "app.h"
#include "platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ANSI color codes (work on both Windows 10+ and Unix) */
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

/* Maze layout (40 wide x 15 tall) */
static const char *LEVEL_TEMPLATE[MAP_HEIGHT] = {
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

static void copy_level(char map[MAP_HEIGHT][MAP_WIDTH + 1]) {
    for (int r = 0; r < MAP_HEIGHT; ++r) {
        strncpy(map[r], LEVEL_TEMPLATE[r], MAP_WIDTH + 1);
        map[r][MAP_WIDTH] = '\0';
    }
}

static unsigned int count_dots(const char map[MAP_HEIGHT][MAP_WIDTH + 1]) {
    unsigned int dots = 0;
    for (int r = 0; r < MAP_HEIGHT; ++r) {
        for (int c = 0; c < MAP_WIDTH; ++c) {
            if (map[r][c] == '.') {
                ++dots;
            }
        }
    }
    return dots;
}

static bool is_walkable(const App *app, int row, int col) {
    if (row < 0 || row >= MAP_HEIGHT || col < 0 || col >= MAP_WIDTH) {
        return false;
    }
    char c = app->map[row][col];
    /* Only walls (#) block movement. Dots (.) and eaten spaces ( ) are walkable */
    return c != '#';
}

static void reset_positions(App *app) {
    app->pacman = app->pacman_start;
    app->pacman_dir = 0;
    for (int i = 0; i < NUM_GHOSTS; ++i) {
        app->ghosts[i].pos = app->ghosts[i].start;
        app->ghosts[i].last_dir = -1;
    }
}

static void move_pacman(App *app, int dr, int dc) {
    int nr = app->pacman.row + dr;
    int nc = app->pacman.col + dc;
    if (!is_walkable(app, nr, nc)) {
        return;
    }
    app->pacman.row = nr;
    app->pacman.col = nc;
    app->needs_redraw = true;

    /* Track direction for ghost AI (0=up, 1=down, 2=left, 3=right) */
    if (dr == -1) app->pacman_dir = 0;
    else if (dr == 1) app->pacman_dir = 1;
    else if (dc == -1) app->pacman_dir = 2;
    else if (dc == 1) app->pacman_dir = 3;

    if (app->map[nr][nc] == '.') {
        app->map[nr][nc] = ' ';
        ++app->score;
        --app->dots_remaining;
        platform_play_sound(SOUND_EAT_DOT);
        
        /* Update high score in real-time */
        if (app->score > app->high_score) {
            app->high_score = app->score;
        }
    }
}

/* Calculate Manhattan distance between two positions */
static int manhattan_dist(int r1, int c1, int r2, int c2) {
    int dr = r1 - r2;
    int dc = c1 - c2;
    return (dr < 0 ? -dr : dr) + (dc < 0 ? -dc : dc);
}

/* Get the opposite direction (to avoid immediate backtracking) */
static int opposite_dir(int dir) {
    if (dir == 0) return 1;  /* up -> down */
    if (dir == 1) return 0;  /* down -> up */
    if (dir == 2) return 3;  /* left -> right */
    if (dir == 3) return 2;  /* right -> left */
    return -1;
}

/* Move a single ghost based on its behavior type */
static void move_single_ghost(App *app, Ghost *ghost) {
    static const int dirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    int valid_dirs[4];
    int valid_count = 0;
    
    /* Find all valid directions (not walls, avoid immediate backtrack if possible) */
    for (int i = 0; i < 4; ++i) {
        int nr = ghost->pos.row + dirs[i][0];
        int nc = ghost->pos.col + dirs[i][1];
        if (nr >= 0 && nr < MAP_HEIGHT && nc >= 0 && nc < MAP_WIDTH &&
            app->map[nr][nc] != '#') {
            valid_dirs[valid_count++] = i;
        }
    }
    
    if (valid_count == 0) return;
    
    /* Remove the opposite direction if we have other options (prevents oscillation) */
    int opp = opposite_dir(ghost->last_dir);
    int filtered_dirs[4];
    int filtered_count = 0;
    
    for (int i = 0; i < valid_count; ++i) {
        if (valid_dirs[i] != opp || valid_count == 1) {
            filtered_dirs[filtered_count++] = valid_dirs[i];
        }
    }
    
    int chosen_dir = -1;
    int target_row = app->pacman.row;
    int target_col = app->pacman.col;
    
    switch (ghost->type) {
    case GHOST_CHASER: {
        /* Red ghost: directly chase Pac-Man - pick direction that minimizes distance */
        int best_dist = 9999;
        for (int i = 0; i < filtered_count; ++i) {
            int d = filtered_dirs[i];
            int nr = ghost->pos.row + dirs[d][0];
            int nc = ghost->pos.col + dirs[d][1];
            int dist = manhattan_dist(nr, nc, target_row, target_col);
            if (dist < best_dist) {
                best_dist = dist;
                chosen_dir = d;
            }
        }
        break;
    }
    
    case GHOST_AMBUSHER: {
        /* Pink ghost: aim 4 tiles ahead of Pac-Man's direction */
        static const int ahead[4][2] = {{-4, 0}, {4, 0}, {0, -4}, {0, 4}};
        int aim_r = target_row + ahead[app->pacman_dir][0];
        int aim_c = target_col + ahead[app->pacman_dir][1];
        /* Clamp to map bounds */
        if (aim_r < 0) aim_r = 0;
        if (aim_r >= MAP_HEIGHT) aim_r = MAP_HEIGHT - 1;
        if (aim_c < 0) aim_c = 0;
        if (aim_c >= MAP_WIDTH) aim_c = MAP_WIDTH - 1;
        
        int best_dist = 9999;
        for (int i = 0; i < filtered_count; ++i) {
            int d = filtered_dirs[i];
            int nr = ghost->pos.row + dirs[d][0];
            int nc = ghost->pos.col + dirs[d][1];
            int dist = manhattan_dist(nr, nc, aim_r, aim_c);
            if (dist < best_dist) {
                best_dist = dist;
                chosen_dir = d;
            }
        }
        break;
    }
    
    case GHOST_FLANKER: {
        /* Cyan ghost: try to flank from the side perpendicular to Pac-Man's movement */
        int flank_r = target_row;
        int flank_c = target_col;
        if (app->pacman_dir == 0 || app->pacman_dir == 1) {
            /* Pac-Man moving vertically, flank horizontally */
            flank_c += (ghost->pos.col > target_col) ? 3 : -3;
        } else {
            /* Pac-Man moving horizontally, flank vertically */
            flank_r += (ghost->pos.row > target_row) ? 3 : -3;
        }
        if (flank_r < 0) flank_r = 0;
        if (flank_r >= MAP_HEIGHT) flank_r = MAP_HEIGHT - 1;
        if (flank_c < 0) flank_c = 0;
        if (flank_c >= MAP_WIDTH) flank_c = MAP_WIDTH - 1;
        
        int best_dist = 9999;
        for (int i = 0; i < filtered_count; ++i) {
            int d = filtered_dirs[i];
            int nr = ghost->pos.row + dirs[d][0];
            int nc = ghost->pos.col + dirs[d][1];
            int dist = manhattan_dist(nr, nc, flank_r, flank_c);
            if (dist < best_dist) {
                best_dist = dist;
                chosen_dir = d;
            }
        }
        break;
    }
    
    case GHOST_RANDOM:
    default: {
        /* Orange ghost: random movement with slight bias toward Pac-Man */
        if (rand() % 100 < 30) {
            /* 30% chance to chase */
            int best_dist = 9999;
            for (int i = 0; i < filtered_count; ++i) {
                int d = filtered_dirs[i];
                int nr = ghost->pos.row + dirs[d][0];
                int nc = ghost->pos.col + dirs[d][1];
                int dist = manhattan_dist(nr, nc, target_row, target_col);
                if (dist < best_dist) {
                    best_dist = dist;
                    chosen_dir = d;
                }
            }
        } else {
            /* 70% random */
            chosen_dir = filtered_dirs[rand() % filtered_count];
        }
        break;
    }
    }
    
    if (chosen_dir >= 0) {
        ghost->pos.row += dirs[chosen_dir][0];
        ghost->pos.col += dirs[chosen_dir][1];
        ghost->last_dir = chosen_dir;
        app->needs_redraw = true;
    }
}

/* Move all ghosts */
static void move_ghosts(App *app) {
    for (int i = 0; i < NUM_GHOSTS; ++i) {
        move_single_ghost(app, &app->ghosts[i]);
    }
}

static void check_collision(App *app) {
    for (int i = 0; i < NUM_GHOSTS; ++i) {
        if (app->pacman.row == app->ghosts[i].pos.row && 
            app->pacman.col == app->ghosts[i].pos.col) {
            --app->lives;
            if (app->lives == 0) {
                app->game_over = true;  /* Don't exit, just show game over */
                platform_play_sound(SOUND_GAME_OVER);
                /* Save high score on game over */
                if (app->score > 0) {
                    platform_save_highscore(app->high_score);
                }
            } else {
                platform_play_sound(SOUND_LOSE_LIFE);
                reset_positions(app);
            }
            app->needs_redraw = true;
            return; /* Only lose one life per collision check */
        }
    }
}

App app_create(void) {
    srand((unsigned int)time(NULL));
    
    /* Load high score from file */
    unsigned int saved_high_score = platform_load_highscore();
    
    App app = {
        .tick_count = 0,
        .score = 0,
        .high_score = saved_high_score,
        .lives = 3,
        .max_lives = 3,
        .dots_remaining = 0,
        .running = true,
        .won = false,
        .game_over = false,
        .needs_redraw = true,
        .pacman_start = {.row = 13, .col = 20},
        .pacman_dir = 0,
        .term_rows = 24,
        .term_cols = 80,
    };
    
    platform_play_sound(SOUND_START);
    
    /* Initialize ghosts with different starting positions and behaviors */
    /* Red ghost (Chaser) - top left area */
    app.ghosts[0].start = (Position){.row = 1, .col = 1};
    app.ghosts[0].type = GHOST_CHASER;
    app.ghosts[0].last_dir = -1;
    
    /* Pink ghost (Ambusher) - top right area */
    app.ghosts[1].start = (Position){.row = 1, .col = 38};
    app.ghosts[1].type = GHOST_AMBUSHER;
    app.ghosts[1].last_dir = -1;
    
    /* Cyan ghost (Flanker) - center area */
    app.ghosts[2].start = (Position){.row = 7, .col = 20};
    app.ghosts[2].type = GHOST_FLANKER;
    app.ghosts[2].last_dir = -1;
    
    /* Orange ghost (Random) - bottom area */
    app.ghosts[3].start = (Position){.row = 11, .col = 10};
    app.ghosts[3].type = GHOST_RANDOM;
    app.ghosts[3].last_dir = -1;
    
    copy_level(app.map);
    app.dots_remaining = count_dots(app.map);
    reset_positions(&app);
    platform_get_terminal_size(&app.term_rows, &app.term_cols);
    return app;
}

void app_destroy(App *app) {
    if (app) {
        app->running = false;
    }
}

/* Build entire frame into buffer, then single write call */
void app_render(App *app) {
    if (!app->needs_redraw) {
        return;
    }
    app->needs_redraw = false;

    platform_get_terminal_size(&app->term_rows, &app->term_cols);

    char *buf = app->frame_buffer;
    char *end = buf + FRAME_BUFFER_SIZE - 256;
    char *p = buf;

    /* Clear screen */
    p += sprintf(p, "\033[2J\033[H");

    /* Calculate centering */
    int content_h = MAP_HEIGHT + 8;
    int content_w = MAP_WIDTH + 4;
    int pad_top = (app->term_rows - content_h) / 2;
    int pad_left = (app->term_cols - content_w) / 2;
    if (pad_top < 0) pad_top = 0;
    if (pad_left < 0) pad_left = 0;

    /* Vertical padding */
    for (int i = 0; i < pad_top && p < end; ++i) {
        *p++ = '\n';
    }

    /* Title */
    for (int i = 0; i < pad_left && p < end; ++i) *p++ = ' ';
    p += sprintf(p, COLOR_BOLD COLOR_MAGENTA "================= PAC-MAN =================" COLOR_RESET "\n");

    /* Stats */
    for (int i = 0; i < pad_left && p < end; ++i) *p++ = ' ';
    p += sprintf(p, COLOR_CYAN " Score: " COLOR_BOLD COLOR_GREEN "%u" COLOR_RESET, app->score);
    p += sprintf(p, COLOR_CYAN "  Hi: " COLOR_BOLD COLOR_YELLOW "%u" COLOR_RESET, app->high_score);
    p += sprintf(p, COLOR_CYAN "  Lives: " COLOR_BOLD);
    if (app->lives > 0) {
        p += sprintf(p, COLOR_GREEN "%u" COLOR_RESET "/" COLOR_WHITE "%u", 
                     app->lives, app->max_lives);
    } else {
        p += sprintf(p, COLOR_RED "0" COLOR_RESET "/" COLOR_WHITE "%u", app->max_lives);
    }
    p += sprintf(p, COLOR_RESET COLOR_CYAN "  Dots: " COLOR_BOLD COLOR_WHITE "%u" COLOR_RESET "\n", 
                 app->dots_remaining);

    /* Controls */
    for (int i = 0; i < pad_left && p < end; ++i) *p++ = ' ';
    p += sprintf(p, COLOR_WHITE " WASD" COLOR_CYAN "=Move " COLOR_WHITE "R/Space" COLOR_CYAN "=Restart " COLOR_WHITE "Q" COLOR_CYAN "=Quit" COLOR_RESET "\n");

    /* Separator */
    for (int i = 0; i < pad_left && p < end; ++i) *p++ = ' ';
    p += sprintf(p, COLOR_BLUE "------------------------------------------" COLOR_RESET "\n");

    /* Map */
    for (int r = 0; r < MAP_HEIGHT && p < end; ++r) {
        for (int i = 0; i < pad_left; ++i) *p++ = ' ';
        *p++ = ' ';

        for (int c = 0; c < MAP_WIDTH && p < end; ++c) {
            if (app->pacman.row == r && app->pacman.col == c) {
                p += sprintf(p, COLOR_BOLD COLOR_YELLOW "C" COLOR_RESET);
            } else {
                /* Check if any ghost is at this position */
                int ghost_here = -1;
                for (int g = 0; g < NUM_GHOSTS; ++g) {
                    if (app->ghosts[g].pos.row == r && app->ghosts[g].pos.col == c) {
                        ghost_here = g;
                        break;
                    }
                }
                
                if (ghost_here >= 0) {
                    /* All ghosts show "G" with vibrant colors using 256-color mode */
                    switch (app->ghosts[ghost_here].type) {
                    case GHOST_CHASER:
                        /* Bright red with background highlight */
                        p += sprintf(p, "\033[1;38;5;196mG" COLOR_RESET);
                        break;
                    case GHOST_AMBUSHER:
                        /* Hot pink/magenta */
                        p += sprintf(p, "\033[1;38;5;201mG" COLOR_RESET);
                        break;
                    case GHOST_FLANKER:
                        /* Bright cyan */
                        p += sprintf(p, "\033[1;38;5;51mG" COLOR_RESET);
                        break;
                    case GHOST_RANDOM:
                        /* Bright orange */
                        p += sprintf(p, "\033[1;38;5;214mG" COLOR_RESET);
                        break;
                    }
                } else {
                    char tile = app->map[r][c];
                    if (tile == '#') {
                        p += sprintf(p, COLOR_BLUE "#" COLOR_RESET);
                    } else if (tile == '.') {
                        p += sprintf(p, COLOR_WHITE "." COLOR_RESET);
                    } else {
                        *p++ = ' ';
                    }
                }
            }
        }
        *p++ = '\n';
    }

    /* Bottom separator */
    for (int i = 0; i < pad_left && p < end; ++i) *p++ = ' ';
    p += sprintf(p, COLOR_BLUE "------------------------------------------" COLOR_RESET "\n");

    /* Status message */
    for (int i = 0; i < pad_left && p < end; ++i) *p++ = ' ';
    if (app->won) {
        p += sprintf(p, COLOR_BOLD COLOR_GREEN " *** MISSION COMPLETE! All dots cleared! ***" COLOR_RESET "\n");
    } else if (app->game_over) {
        p += sprintf(p, "\033[1;41;97m ========== MISSION FAILED ========== " COLOR_RESET "\n");
        for (int i = 0; i < pad_left && p < end; ++i) *p++ = ' ';
        p += sprintf(p, COLOR_BOLD COLOR_YELLOW " Press SPACE or R to try again!" COLOR_RESET "\n");
    } else {
        p += sprintf(p, COLOR_BOLD COLOR_YELLOW " C" COLOR_RESET "=Pac-Man ");
        p += sprintf(p, "\033[1;38;5;196mG" COLOR_RESET "=Chaser ");
        p += sprintf(p, "\033[1;38;5;201mG" COLOR_RESET "=Ambush ");
        p += sprintf(p, "\033[1;38;5;51mG" COLOR_RESET "=Flank ");
        p += sprintf(p, "\033[1;38;5;214mG" COLOR_RESET "=Random\n");
    }

    /* Single write call - no flicker */
    platform_write(buf, (int)(p - buf));
}

void app_handle_input(App *app, int cmd) {
    if (cmd == -1) {
        return;
    }

    if (cmd == 'q' || cmd == 'Q') {
        app->running = false;
        return;
    }

    /* Restart with R or Space */
    if (cmd == 'r' || cmd == 'R' || cmd == ' ') {
        copy_level(app->map);
        app->dots_remaining = count_dots(app->map);
        app->score = 0;
        /* Keep high_score - it persists across restarts */
        app->lives = app->max_lives;
        app->won = false;
        app->game_over = false;
        app->running = true;
        reset_positions(app);
        app->tick_count = 0;
        app->needs_redraw = true;
        platform_play_sound(SOUND_START);
        return;
    }

    /* Don't process movement if game over or won */
    if (app->game_over || app->won) {
        return;
    }

    switch (cmd) {
    case 'w': case 'W': move_pacman(app, -1, 0); break;
    case 's': case 'S': move_pacman(app, 1, 0); break;
    case 'a': case 'A': move_pacman(app, 0, -1); break;
    case 'd': case 'D': move_pacman(app, 0, 1); break;
    }

    check_collision(app);
    if (app->dots_remaining == 0) {
        app->won = true;
        app->needs_redraw = true;
        platform_play_sound(SOUND_WIN);
        platform_save_highscore(app->high_score);
    }
}

void app_update(App *app) {
    if (!app->running || app->won || app->game_over) {
        return;
    }

    ++app->tick_count;
    move_ghosts(app);
    check_collision(app);

    if (app->dots_remaining == 0 && !app->won) {
        app->won = true;
        app->needs_redraw = true;
        platform_play_sound(SOUND_WIN);
        platform_save_highscore(app->high_score);
    }
}

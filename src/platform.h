/*
 * Platform Abstraction Layer
 *
 * Provides cross-platform terminal handling for Console Pac-Man.
 *
 * Detected automatically via preprocessor:
 *   - Windows: Uses conio.h (_kbhit, _getch) and Windows Console API
 *   - Unix/Linux/macOS: Uses termios.h and POSIX calls
 *
 * Standard C libraries used: stdio.h, stdlib.h, time.h
 */

#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdbool.h>

/*
 * Initialize terminal for game mode:
 * - Disable line buffering (get keys immediately)
 * - Disable echo (don't show typed characters)
 * - Enable non-blocking input
 */
void platform_init(void);

/*
 * Restore terminal to original state.
 * Called automatically via atexit().
 */
void platform_cleanup(void);

/*
 * Enter fullscreen mode:
 * - Switch to alternate screen buffer (Unix)
 * - Hide cursor
 */
void platform_enter_fullscreen(void);

/*
 * Exit fullscreen mode:
 * - Restore main screen buffer
 * - Show cursor
 */
void platform_exit_fullscreen(void);

/*
 * Check if a key has been pressed (non-blocking).
 * Returns true if input is available.
 */
bool platform_kbhit(void);

/*
 * Read a single character from input (non-blocking).
 * Returns the character code, or -1 if no input available.
 */
int platform_getch(void);

/*
 * Get current time in milliseconds.
 * Uses monotonic clock where available.
 */
long platform_time_ms(void);

/*
 * Get terminal dimensions.
 */
void platform_get_terminal_size(int *rows, int *cols);

/*
 * Write buffer to screen (single atomic write for no flicker).
 */
void platform_write(const char *buf, int len);

/*
 * Clear screen and move cursor to home position.
 */
void platform_clear_screen(void);

/*
 * Sound effect types for the game.
 */
typedef enum SoundType {
    SOUND_EAT_DOT,      /* Eating a dot */
    SOUND_LOSE_LIFE,    /* Losing a life */
    SOUND_GAME_OVER,    /* Game over */
    SOUND_WIN,          /* Winning the game */
    SOUND_START         /* Game start/restart */
} SoundType;

/*
 * Play a sound effect (non-blocking).
 * Uses terminal bell or system sounds depending on platform.
 */
void platform_play_sound(SoundType type);

/*
 * Get the path to the high score file.
 * Returns the full path in buf (up to size chars).
 */
void platform_get_highscore_path(char *buf, int size);

/*
 * Save high score to file.
 * Returns true on success.
 */
bool platform_save_highscore(unsigned int score);

/*
 * Load high score from file.
 * Returns the saved high score, or 0 if none exists.
 */
unsigned int platform_load_highscore(void);

#endif /* PLATFORM_H */

/*
 * Platform code for handling different operating systems.
 * 
 * This file was made with help from ChatGPT because handling
 * the terminal on Windows and Mac/Linux is very different.
 */

#include <stdbool.h>

// Setup terminal for the game
void platform_init();

// Restore terminal to normal
void platform_cleanup();

// Enter fullscreen mode
void platform_enter_fullscreen();

// Exit fullscreen mode
void platform_exit_fullscreen();

// Check if a key was pressed
bool platform_kbhit();

// Get the key that was pressed
int platform_getch();

// Get current time in milliseconds
long platform_time_ms();

// Get terminal size
void platform_get_terminal_size(int *rows, int *cols);

// Write text to screen
void platform_write(const char *buf, int len);

// Clear the screen
void platform_clear_screen();

// Sound types
#define SOUND_EAT_DOT    0
#define SOUND_LOSE_LIFE  1
#define SOUND_GAME_OVER  2
#define SOUND_WIN        3
#define SOUND_START      4

typedef int SoundType;

// Play a sound
void platform_play_sound(SoundType type);

// Get path to high score file
void platform_get_highscore_path(char *buf, int size);

// Save high score
bool platform_save_highscore(unsigned int score);

// Load high score
unsigned int platform_load_highscore();

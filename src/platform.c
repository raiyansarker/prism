/*
 * This file was made with help from ChatGPT because handling
 * the terminal on Windows and Mac/Linux is very different.
 * It was hard for us to collaborate on this as I used a Mac
 * and others were on Windows.
 */

#include "platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Get the filename for each sound
const char *get_sound_filename(SoundType type) {
    if (type == SOUND_EAT_DOT) return "eat.wav";
    if (type == SOUND_LOSE_LIFE) return "lose.wav";
    if (type == SOUND_GAME_OVER) return "gameover.wav";
    if (type == SOUND_WIN) return "win.wav";
    if (type == SOUND_START) return "start.wav";
    return NULL;
}

/* ============================================================
 * WINDOWS CODE
 * ============================================================ */
#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <conio.h>
#include <mmsystem.h>
#include <direct.h>
#pragma comment(lib, "winmm.lib")

HANDLE g_hStdout;
HANDLE g_hStdin;
DWORD g_orig_stdout_mode;
DWORD g_orig_stdin_mode;
CONSOLE_CURSOR_INFO g_orig_cursor_info;

void platform_init() {
    g_hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    g_hStdin = GetStdHandle(STD_INPUT_HANDLE);

    // Save original settings
    GetConsoleMode(g_hStdout, &g_orig_stdout_mode);
    GetConsoleMode(g_hStdin, &g_orig_stdin_mode);
    GetConsoleCursorInfo(g_hStdout, &g_orig_cursor_info);

    // Enable colors
    DWORD stdout_mode = g_orig_stdout_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(g_hStdout, stdout_mode);

    // Disable line buffering
    DWORD stdin_mode = g_orig_stdin_mode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
    SetConsoleMode(g_hStdin, stdin_mode);

    atexit(platform_cleanup);
}

void platform_cleanup() {
    SetConsoleMode(g_hStdout, g_orig_stdout_mode);
    SetConsoleMode(g_hStdin, g_orig_stdin_mode);
    SetConsoleCursorInfo(g_hStdout, &g_orig_cursor_info);
}

void platform_enter_fullscreen() {
    // Hide cursor
    CONSOLE_CURSOR_INFO cursor_info;
    cursor_info.dwSize = 1;
    cursor_info.bVisible = FALSE;
    SetConsoleCursorInfo(g_hStdout, &cursor_info);

    platform_clear_screen();
}

void platform_exit_fullscreen() {
    // Show cursor again
    SetConsoleCursorInfo(g_hStdout, &g_orig_cursor_info);
    platform_clear_screen();
}

bool platform_kbhit() {
    return _kbhit() != 0;
}

int platform_getch() {
    if (_kbhit()) {
        int ch = _getch();
        // Handle arrow keys
        if (ch == 0 || ch == 224) {
            int ext = _getch();
            if (ext == 72) return 'w';  // Up
            if (ext == 80) return 's';  // Down
            if (ext == 75) return 'a';  // Left
            if (ext == 77) return 'd';  // Right
            return -1;
        }
        return ch;
    }
    return -1;
}

long platform_time_ms() {
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (long)((counter.QuadPart * 1000) / freq.QuadPart);
}

void platform_get_terminal_size(int *rows, int *cols) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(g_hStdout, &csbi)) {
        *cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    } else {
        *rows = 24;
        *cols = 80;
    }
}

void platform_write(const char *buf, int len) {
    DWORD written;
    WriteConsoleA(g_hStdout, buf, (DWORD)len, &written, NULL);
}

void platform_clear_screen() {
    const char *clear = "\033[2J\033[H";
    platform_write(clear, (int)strlen(clear));
}

void platform_play_sound(SoundType type) {
    const char *filename = get_sound_filename(type);
    if (filename == NULL) return;
    
    char path[512];
    snprintf(path, sizeof(path), "sounds\\%s", filename);
    PlaySoundA(path, NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
}

void platform_get_highscore_path(char *buf, int size) {
    snprintf(buf, size, "data");
}

bool platform_save_highscore(unsigned int score) {
    char path[512];
    platform_get_highscore_path(path, sizeof(path));
    FILE *f = fopen(path, "w");
    if (f == NULL) return false;
    fprintf(f, "%u\n", score);
    fclose(f);
    return true;
}

unsigned int platform_load_highscore() {
    char path[512];
    platform_get_highscore_path(path, sizeof(path));
    FILE *f = fopen(path, "r");
    if (f == NULL) return 0;
    unsigned int score = 0;
    if (fscanf(f, "%u", &score) != 1) {
        score = 0;
    }
    fclose(f);
    return score;
}

/* ============================================================
 * MAC/LINUX CODE
 * ============================================================ */
#else

#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

struct termios g_orig_termios;
volatile sig_atomic_t g_signal_received = 0;

void signal_handler(int sig) {
    g_signal_received = 1;
}

void platform_init() {
    if (isatty(STDIN_FILENO) == 0) {
        fprintf(stderr, "Error: stdin is not a terminal\n");
        exit(1);
    }

    if (tcgetattr(STDIN_FILENO, &g_orig_termios) == -1) {
        perror("tcgetattr");
        exit(1);
    }

    atexit(platform_cleanup);

    // Handle Ctrl+C properly
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Setup terminal for game
    struct termios raw = g_orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        exit(1);
    }
}

void platform_cleanup() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios);
}

void platform_enter_fullscreen() {
    const char *seq = "\033[?1049h\033[?25l";
    write(STDOUT_FILENO, seq, strlen(seq));
}

void platform_exit_fullscreen() {
    const char *seq = "\033[?25h\033[?1049l";
    write(STDOUT_FILENO, seq, strlen(seq));
}

bool platform_kbhit() {
    if (g_signal_received) {
        return true;
    }

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

int platform_getch() {
    if (g_signal_received) {
        return 'q';
    }

    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) == 1) {
        // Handle arrow keys
        if (c == '\033') {
            unsigned char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) == 1 && seq[0] == '[') {
                if (read(STDIN_FILENO, &seq[1], 1) == 1) {
                    if (seq[1] == 'A') return 'w';  // Up
                    if (seq[1] == 'B') return 's';  // Down
                    if (seq[1] == 'C') return 'd';  // Right
                    if (seq[1] == 'D') return 'a';  // Left
                }
            }
            return '\033';
        }
        return (int)c;
    }
    return -1;
}

long platform_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

void platform_get_terminal_size(int *rows, int *cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0) {
        *rows = ws.ws_row;
        *cols = ws.ws_col;
    } else {
        *rows = 24;
        *cols = 80;
    }
}

void platform_write(const char *buf, int len) {
    write(STDOUT_FILENO, buf, (size_t)len);
}

void platform_clear_screen() {
    const char *clear = "\033[2J\033[H";
    write(STDOUT_FILENO, clear, strlen(clear));
}

void platform_play_sound(SoundType type) {
    const char *filename = get_sound_filename(type);
    if (filename == NULL) return;
    
    char path[512];
    snprintf(path, sizeof(path), "sounds/%s", filename);
    
    char cmd[600];
#ifdef __APPLE__
    // Mac uses afplay
    snprintf(cmd, sizeof(cmd), "(afplay '%s' &) 2>/dev/null", path);
#else
    // Linux uses aplay or paplay
    snprintf(cmd, sizeof(cmd), 
        "(aplay -q '%s' 2>/dev/null || paplay '%s' 2>/dev/null &) 2>/dev/null", 
        path, path);
#endif
    system(cmd);
}

void platform_get_highscore_path(char *buf, int size) {
    snprintf(buf, (size_t)size, "data");
}

bool platform_save_highscore(unsigned int score) {
    char path[512];
    platform_get_highscore_path(path, sizeof(path));
    FILE *f = fopen(path, "w");
    if (f == NULL) return false;
    fprintf(f, "%u\n", score);
    fclose(f);
    return true;
}

unsigned int platform_load_highscore() {
    char path[512];
    platform_get_highscore_path(path, sizeof(path));
    FILE *f = fopen(path, "r");
    if (f == NULL) return 0;
    unsigned int score = 0;
    if (fscanf(f, "%u", &score) != 1) {
        score = 0;
    }
    fclose(f);
    return score;
}

#endif

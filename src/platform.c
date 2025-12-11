/*
 * Platform Abstraction Layer - Implementation
 *
 * Compile-time OS detection:
 *   _WIN32       -> Windows (32-bit and 64-bit)
 *   __APPLE__    -> macOS
 *   __linux__    -> Linux
 *   (default)    -> Other Unix/POSIX systems
 */

#include "platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ============================================================
 * SOUND FILE PATHS
 * Sounds are loaded from the sounds/ directory next to executable
 * ============================================================ */

/* Get the sound filename for a sound type */
static const char *get_sound_filename(SoundType type) {
    switch (type) {
    case SOUND_EAT_DOT:   return "eat.wav";
    case SOUND_LOSE_LIFE: return "lose.wav";
    case SOUND_GAME_OVER: return "gameover.wav";
    case SOUND_WIN:       return "win.wav";
    case SOUND_START:     return "start.wav";
    default:              return NULL;
    }
}

/* ============================================================
 * WINDOWS IMPLEMENTATION
 * ============================================================ */
#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <conio.h>
#include <mmsystem.h>  /* For PlaySound */
#include <direct.h>    /* For _mkdir */
#pragma comment(lib, "winmm.lib")

static HANDLE g_hStdout;
static HANDLE g_hStdin;
static DWORD g_orig_stdout_mode;
static DWORD g_orig_stdin_mode;
static CONSOLE_CURSOR_INFO g_orig_cursor_info;

void platform_init(void) {
    g_hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    g_hStdin = GetStdHandle(STD_INPUT_HANDLE);

    /* Save original console modes */
    GetConsoleMode(g_hStdout, &g_orig_stdout_mode);
    GetConsoleMode(g_hStdin, &g_orig_stdin_mode);
    GetConsoleCursorInfo(g_hStdout, &g_orig_cursor_info);

    /* Enable virtual terminal processing for ANSI codes */
    DWORD stdout_mode = g_orig_stdout_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(g_hStdout, stdout_mode);

    /* Disable line input and echo */
    DWORD stdin_mode = g_orig_stdin_mode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
    SetConsoleMode(g_hStdin, stdin_mode);

    atexit(platform_cleanup);
}

void platform_cleanup(void) {
    SetConsoleMode(g_hStdout, g_orig_stdout_mode);
    SetConsoleMode(g_hStdin, g_orig_stdin_mode);
    SetConsoleCursorInfo(g_hStdout, &g_orig_cursor_info);
}

void platform_enter_fullscreen(void) {
    /* Hide cursor */
    CONSOLE_CURSOR_INFO cursor_info;
    cursor_info.dwSize = 1;
    cursor_info.bVisible = FALSE;
    SetConsoleCursorInfo(g_hStdout, &cursor_info);

    /* Clear screen */
    platform_clear_screen();
}

void platform_exit_fullscreen(void) {
    /* Show cursor */
    SetConsoleCursorInfo(g_hStdout, &g_orig_cursor_info);

    /* Clear screen */
    platform_clear_screen();
}

bool platform_kbhit(void) {
    return _kbhit() != 0;
}

int platform_getch(void) {
    if (_kbhit()) {
        int ch = _getch();
        /* Handle arrow keys (return simplified codes) */
        if (ch == 0 || ch == 224) {
            int ext = _getch();
            switch (ext) {
                case 72: return 'w';  /* Up arrow -> w */
                case 80: return 's';  /* Down arrow -> s */
                case 75: return 'a';  /* Left arrow -> a */
                case 77: return 'd';  /* Right arrow -> d */
                default: return -1;
            }
        }
        return ch;
    }
    return -1;
}

long platform_time_ms(void) {
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

void platform_clear_screen(void) {
    /* Use ANSI escape codes (enabled via ENABLE_VIRTUAL_TERMINAL_PROCESSING) */
    const char *clear = "\033[2J\033[H";
    platform_write(clear, (int)strlen(clear));
}

void platform_play_sound(SoundType type) {
    const char *filename = get_sound_filename(type);
    if (!filename) return;
    
    /* Build path to sound file (sounds/ directory next to executable) */
    char path[512];
    snprintf(path, sizeof(path), "sounds\\%s", filename);
    
    /* Play sound asynchronously so it doesn't block the game */
    PlaySoundA(path, NULL, SND_FILENAME | SND_ASYNC | SND_NODEFAULT);
}

void platform_get_highscore_path(char *buf, int size) {
    const char *appdata = getenv("APPDATA");
    if (appdata) {
        snprintf(buf, size, "%s\\prism_highscore.dat", appdata);
    } else {
        snprintf(buf, size, "prism_highscore.dat");
    }
}

bool platform_save_highscore(unsigned int score) {
    char path[512];
    platform_get_highscore_path(path, sizeof(path));
    FILE *f = fopen(path, "w");
    if (!f) return false;
    fprintf(f, "%u\n", score);
    fclose(f);
    return true;
}

unsigned int platform_load_highscore(void) {
    char path[512];
    platform_get_highscore_path(path, sizeof(path));
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    unsigned int score = 0;
    if (fscanf(f, "%u", &score) != 1) {
        score = 0;
    }
    fclose(f);
    return score;
}

/* ============================================================
 * UNIX/LINUX/MACOS IMPLEMENTATION (POSIX)
 * ============================================================ */
#else

#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>

static struct termios g_orig_termios;
static volatile sig_atomic_t g_signal_received = 0;

static void signal_handler(int sig) {
    (void)sig;
    g_signal_received = 1;
}

void platform_init(void) {
    if (!isatty(STDIN_FILENO)) {
        fprintf(stderr, "Error: stdin is not a terminal\n");
        exit(EXIT_FAILURE);
    }

    if (tcgetattr(STDIN_FILENO, &g_orig_termios) == -1) {
        perror("tcgetattr");
        exit(EXIT_FAILURE);
    }

    atexit(platform_cleanup);

    /* Setup signal handlers for clean exit */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Enter raw mode: no echo, no canonical mode, non-blocking */
    struct termios raw = g_orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }
}

void platform_cleanup(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios);
}

void platform_enter_fullscreen(void) {
    /* Switch to alternate screen buffer and hide cursor */
    const char *seq = "\033[?1049h\033[?25l";
    write(STDOUT_FILENO, seq, strlen(seq));
}

void platform_exit_fullscreen(void) {
    /* Show cursor and restore main screen buffer */
    const char *seq = "\033[?25h\033[?1049l";
    write(STDOUT_FILENO, seq, strlen(seq));
}

bool platform_kbhit(void) {
    if (g_signal_received) {
        return true;  /* Treat signal as "input" to exit loop */
    }

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    struct timeval tv = {0, 0};  /* No wait */
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

int platform_getch(void) {
    if (g_signal_received) {
        return 'q';  /* Signal triggers quit */
    }

    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) == 1) {
        /* Handle escape sequences for arrow keys */
        if (c == '\033') {
            unsigned char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) == 1 && seq[0] == '[') {
                if (read(STDIN_FILENO, &seq[1], 1) == 1) {
                    switch (seq[1]) {
                        case 'A': return 'w';  /* Up arrow -> w */
                        case 'B': return 's';  /* Down arrow -> s */
                        case 'C': return 'd';  /* Right arrow -> d */
                        case 'D': return 'a';  /* Left arrow -> a */
                    }
                }
            }
            return '\033';
        }
        return (int)c;
    }
    return -1;
}

long platform_time_ms(void) {
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

void platform_clear_screen(void) {
    const char *clear = "\033[2J\033[H";
    write(STDOUT_FILENO, clear, strlen(clear));
}

void platform_play_sound(SoundType type) {
    const char *filename = get_sound_filename(type);
    if (!filename) return;
    
    /* Build path to sound file (sounds/ directory next to executable) */
    char path[512];
    snprintf(path, sizeof(path), "sounds/%s", filename);
    
    /* Play sound asynchronously in background */
    char cmd[600];
#ifdef __APPLE__
    /* macOS: use afplay */
    snprintf(cmd, sizeof(cmd), "(afplay '%s' &) 2>/dev/null", path);
#else
    /* Linux: try aplay (ALSA), paplay (PulseAudio), or fall back to nothing */
    snprintf(cmd, sizeof(cmd), 
        "(aplay -q '%s' 2>/dev/null || paplay '%s' 2>/dev/null &) 2>/dev/null", 
        path, path);
#endif
    system(cmd);
}

void platform_get_highscore_path(char *buf, int size) {
    const char *home = getenv("HOME");
    if (home) {
        snprintf(buf, (size_t)size, "%s/.prism_highscore", home);
    } else {
        snprintf(buf, (size_t)size, ".prism_highscore");
    }
}

bool platform_save_highscore(unsigned int score) {
    char path[512];
    platform_get_highscore_path(path, sizeof(path));
    FILE *f = fopen(path, "w");
    if (!f) return false;
    fprintf(f, "%u\n", score);
    fclose(f);
    return true;
}

unsigned int platform_load_highscore(void) {
    char path[512];
    platform_get_highscore_path(path, sizeof(path));
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    unsigned int score = 0;
    if (fscanf(f, "%u", &score) != 1) {
        score = 0;
    }
    fclose(f);
    return score;
}

#endif /* _WIN32 */

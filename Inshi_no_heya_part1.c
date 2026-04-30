#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <limits.h>
#include <stdarg.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#if !defined(__MINGW32__) && !defined(__MINGW64__)
int _stricmp(const char* lhs, const char* rhs);
#endif
#define strcasecmp _stricmp
#else
#include <strings.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <errno.h>
#endif

// Max size capped to 9x9 to ensure single digit row/col/val parsing
#define MAX_SIZE 9
#define STATS_FILE "inshi_stats.txt"

// Key Codes
#define KEY_UP 1001
#define KEY_DOWN 1002
#define KEY_LEFT 1003
#define KEY_RIGHT 1004
#define KEY_ENTER 1005
#define KEY_BACKSPACE 1006
#define AUDIO_STATUS_MAX 256

// Achievement Bitmasks
#define ACH_FIRST_WIN 1
#define ACH_NO_HINT 2
#define ACH_SPEED 4
#define ACH_HARD 8
#define ACH_BOSS 16
#define ACH_IRON_HEART 32
#define ACH_MEGA_GRID 64
#define ACH_SCORE_MASTER 128
#define ACH_NIGHTMARE_SPEED 256
#define ACH_STEADY_HAND 512
#define ACH_HINT_COMMANDER 1024
#define ACH_COMEBACK_SPIRIT 2048

bool USE_COLOR = true;

void clear_screen(void);
void pm_custom(int width);
void print_theme(const char* text, const char* kind);
void update_term_size(void);
void print_padded(const char* text, int width, const char* align, const char* kind);
void sleep_ms(int ms);
int kbhit_portable(void);
int read_key(void);
void set_frame_row_estimate(int rows);

typedef struct { int r, c; } Point;
typedef struct {
    int id; int product; int num_cells;
    Point cells[MAX_SIZE * MAX_SIZE];
} Room;

int run_menu_centered(void (*header_func)(), const char* title, const char** options, int num_options);
void load_fixed_puzzle(int choice, int* size, Room** out_rooms, int* num_rooms, int solution[MAX_SIZE][MAX_SIZE]);
void build_room_map(int size, Room rooms[], int num_rooms, int room_map[MAX_SIZE][MAX_SIZE]);
void generate_puzzle(int size, Room rooms[], int* num_rooms, int grid[MAX_SIZE][MAX_SIZE], int room_map[MAX_SIZE][MAX_SIZE]);
void build_labels(int size, Room rooms[], int num_rooms, int labels[MAX_SIZE][MAX_SIZE]);
void create_prefilled_board(int size, int solution[MAX_SIZE][MAX_SIZE], int player_grid[MAX_SIZE][MAX_SIZE], bool locked_cells[MAX_SIZE][MAX_SIZE], const char* difficulty);
void game_loop(int size, Room rooms[], int num_rooms, int solution[MAX_SIZE][MAX_SIZE], int room_map[MAX_SIZE][MAX_SIZE], int labels[MAX_SIZE][MAX_SIZE], int player_grid[MAX_SIZE][MAX_SIZE], bool locked_cells[MAX_SIZE][MAX_SIZE], const char* mode, const char* diff, int hearts, int initial_elapsed);
void game_loop_time_attack(int size, Room rooms[], int num_rooms, int solution[MAX_SIZE][MAX_SIZE], int room_map[MAX_SIZE][MAX_SIZE], int labels[MAX_SIZE][MAX_SIZE], int player_grid[MAX_SIZE][MAX_SIZE], bool locked_cells[MAX_SIZE][MAX_SIZE], int ai_difficulty, int ghost_grid_in[MAX_SIZE][MAX_SIZE], int initial_elapsed);
void game_loop_sprint(int size, int ai_difficulty);
void play_multi_player_menu(void);
void print_banner_centered(void);

typedef enum {
    AUDIO_NONE = 0,
    AUDIO_WINDOWS_COM,
    AUDIO_MPG123,
    AUDIO_FFPLAY
} AudioBackend;

typedef struct {
    const char* path;
    const char* label;
} TrackInfo;

// Stats Globals
int total_wins = 0;
int best_score = 0;
int unlocked_achievements = 0;
int control_mode = 1; // 1 = New (WASD/Arrows), 0 = Old (Command string)
int current_theme = 0; // 0 = Cyberpunk, 1 = Fantasy, 2 = Horror, 3 = Steampunk, 4 = Biopunk
int music_volume = 140;
int current_track = 6;
int current_pet = 0; // 0 = None, 1 = Chiikawa, 2 = Hachiware, 3 = Usagi, 4 = Momonga
int current_language = 0; // 0 = English, 1 = Traditional Chinese, 2 = Simplified Chinese, 3 = Japanese
static char pet_dialogue_text[256] = "";
static char pet_dialogue_speaker[64] = "";
static unsigned long long pet_dialogue_started_ms = 0;
static unsigned long long pet_dialogue_hide_ms = 0;
static unsigned long long pet_dialogue_last_hidden_ms = 0;
static bool pet_dialogue_visible = false;
AudioBackend audio_backend = AUDIO_NONE;
char audio_status[AUDIO_STATUS_MAX] = "Audio idle.";
int main_menu_selected = 0;

const char* track_files[] = {
    "Music/01 - Key.mp3", "Music/02 - Door.mp3", "Music/03 - Subwoofer Lullaby.mp3",
    "Music/04 - Death.mp3", "Music/05 - Living Mice.mp3", "Music/06 - Moog City.mp3",
    "Music/07 - Haggstrom.mp3", "Music/08 - Minecraft.mp3", "Music/09 - Oxygène.mp3",
    "Music/10 - Équinoxe.mp3", "Music/11 - Mice on Venus.mp3", "Music/12 - Dry Hands.mp3",
    "Music/13 - Wet Hands.mp3", "Music/14 - Clark.mp3", "Music/15 - Chris.mp3",
    "Music/16 - Thirteen.mp3", "Music/17 - Excuse.mp3", "Music/18 - Sweden.mp3",
    "Music/19 - Cat.mp3", "Music/20 - Dog.mp3", "Music/21 - Danny.mp3",
    "Music/22 - Beginning.mp3", "Music/23 - Droopy Likes Ricochet.mp3", "Music/24 - Droopy Likes Your Face.mp3"
};
int num_tracks = 25;
char game_base_dir[1024] = ".";
TrackInfo track_catalog[] = {
    {"Music/01 - Key.mp3", "01 - Key"},
    {"Music/02 - Door.mp3", "02 - Door"},
    {"Music/03 - Subwoofer Lullaby.mp3", "03 - Subwoofer Lullaby"},
    {"Music/04 - Death.mp3", "04 - Death"},
    {"Music/05 - Living Mice.mp3", "05 - Living Mice"},
    {"Music/06 - Moog City.mp3", "06 - Moog City"},
    {"Music/07 - Haggstrom.mp3", "07 - Haggstrom"},
    {"Music/08 - Minecraft.mp3", "08 - Minecraft"},
    {"Music/09 - Oxygène.mp3", "09 - Oxygène"},
    {"Music/10 - Équinoxe.mp3", "10 - Équinoxe"},
    {"Music/11 - Mice on Venus.mp3", "11 - Mice on Venus"},
    {"Music/12 - Dry Hands.mp3", "12 - Dry Hands"},
    {"Music/13 - Wet Hands.mp3", "13 - Wet Hands"},
    {"Music/14 - Clark.mp3", "14 - Clark"},
    {"Music/15 - Chris.mp3", "15 - Chris"},
    {"Music/16 - Thirteen.mp3", "16 - Thirteen"},
    {"Music/17 - Excuse.mp3", "17 - Excuse"},
    {"Music/18 - Sweden.mp3", "18 - Sweden"},
    {"Music/19 - Cat.mp3", "19 - Cat"},
    {"Music/20 - Dog.mp3", "20 - Dog"},
    {"Music/21 - Danny.mp3", "21 - Danny"},
    {"Music/22 - Beginning.mp3", "22 - Beginning"},
    {"Music/23 - Droopy Likes Ricochet.mp3", "23 - Droopy Likes Ricochet"},
    {"Music/24 - Droopy Likes Your Face.mp3", "24 - Droopy Likes Your Face"},
    {"Music/25 - Level Complete.mp3", "25 - Level Complete"}
};
const int track_catalog_count = (int)(sizeof(track_catalog) / sizeof(track_catalog[0]));
const char* hidden_victory_track_path = "Music/hidden - Super Mario Bros Level Complete.mp3";
const char* hidden_victory_track_label = "Hidden Victory";
int ui_anim_tick = 0;
bool victory_track_active = false;
static int frame_row_estimate = 0;

volatile sig_atomic_t sigint_flag = 0;

void handle_sigint(int sig) {
    (void)sig;
    sigint_flag = 1;
}

/* -------------------------------------------------------------------------- */
/* AUDIO SYSTEM                                                               */
/* -------------------------------------------------------------------------- */

#ifndef _WIN32
pid_t linux_audio_pid = 0;
#else
PROCESS_INFORMATION windows_audio_pi = {0};
#endif

void set_audio_status(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(audio_status, sizeof(audio_status), fmt, args);
    va_end(args);
}

#ifndef _WIN32
bool is_wsl_runtime() {
    if (getenv("WSL_INTEROP") != NULL || getenv("WSL_DISTRO_NAME") != NULL) return true;
    FILE* f = fopen("/proc/version", "r");
    if (!f) return false;
    char buf[512] = {0};
    (void)!fgets(buf, sizeof(buf), f);
    fclose(f);
    return strstr(buf, "Microsoft") != NULL || strstr(buf, "WSL") != NULL;
}

void linux_path_to_windows(const char* linux_path, char* out, size_t out_size) {
    if (strncmp(linux_path, "/mnt/", 5) == 0 && strlen(linux_path) > 6 && linux_path[6] == '/') {
        char drive = (char)toupper((unsigned char)linux_path[5]);
        snprintf(out, out_size, "%c:%s", drive, linux_path + 6);
        for (size_t i = 0; out[i] != '\0'; i++) if (out[i] == '/') out[i] = '\\';
    } else {
        strncpy(out, linux_path, out_size - 1);
        out[out_size - 1] = '\0';
    }
}
void sleep_ms(int ms);

bool file_exists_path(const char* path) {
    return access(path, F_OK) == 0;
}

bool command_exists(const char* name) {
    const char* path_env = getenv("PATH");
    if (!path_env || !*path_env) return false;
    char path_copy[8192];
    strncpy(path_copy, path_env, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';
    char* part = strtok(path_copy, ":");
    while (part) {
        char full[PATH_MAX];
        snprintf(full, sizeof(full), "%s/%s", part, name);
        if (access(full, X_OK) == 0) return true;
        part = strtok(NULL, ":");
    }
    return false;
}
#else
bool file_exists_path(const char* path) {
    DWORD attrs = GetFileAttributesA(path);
    return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}
#endif

void init_game_base_dir() {
#ifdef _WIN32
    char exe_path[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, exe_path, MAX_PATH);
    if (len > 0 && len < MAX_PATH) {
        exe_path[len] = '\0';
        char* slash = strrchr(exe_path, '\\');
        if (slash) {
            *slash = '\0';
            strncpy(game_base_dir, exe_path, sizeof(game_base_dir) - 1);
            game_base_dir[sizeof(game_base_dir) - 1] = '\0';
        }
    }
#else
    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len > 0) {
        exe_path[len] = '\0';
        char* slash = strrchr(exe_path, '/');
        if (slash) {
            *slash = '\0';
            strncpy(game_base_dir, exe_path, sizeof(game_base_dir) - 1);
            game_base_dir[sizeof(game_base_dir) - 1] = '\0';
        }
    }
#endif
}

void resolve_audio_path(const char* rel, char* out, size_t out_size) {
#ifdef _WIN32
    snprintf(out, out_size, "%s\\%s", game_base_dir, rel);
#else
    snprintf(out, out_size, "%s/%s", game_base_dir, rel);
#endif
}

const char* get_clean_track_display_name(int track_index) {
    if (track_index >= 0 && track_index < track_catalog_count) return track_catalog[track_index].label;
    return "Unknown Track";
}

int get_windows_volume() {
    if (music_volume < 0) return 0;
    if (music_volume > 100) return 100;
    return music_volume;
}

int get_mpg123_gain() {
    int gain = (music_volume * 32768) / 100;
    if (gain < 0) gain = 0;
    if (gain > 65536) gain = 65536;
    return gain;
}

double get_ffplay_volume_scale() {
    if (music_volume <= 0) return 0.0;
    return music_volume / 100.0;
}

void get_track_path(int track_index, char* out, size_t out_size) {
    if (track_index >= 0 && track_index < track_catalog_count) {
        resolve_audio_path(track_catalog[track_index].path, out, out_size);
        return;
    }
    const char* rel = track_files[0];
    if (track_index == 8) rel = "Music/09 - Oxygène.mp3";
    else if (track_index == 9) rel = "Music/10 - Équinoxe.mp3";
#ifdef _WIN32
    snprintf(out, out_size, "%s\\%s", game_base_dir, rel);
#else
    snprintf(out, out_size, "%s/%s", game_base_dir, rel);
#endif
}

const char* get_track_display_name(int track_index) {
    if (track_index < 0 || track_index >= num_tracks) return "Unknown Track";
    if (track_index == 8) return "09 - Oxygène.mp3";
    if (track_index == 9) return "10 - Équinoxe.mp3";
    return track_files[track_index] + 6;
}

void stop_audio() {
#ifdef _WIN32
    if (windows_audio_pi.hProcess) {
        TerminateProcess(windows_audio_pi.hProcess, 0);
        WaitForSingleObject(windows_audio_pi.hProcess, 250);
        CloseHandle(windows_audio_pi.hProcess);
        CloseHandle(windows_audio_pi.hThread);
        ZeroMemory(&windows_audio_pi, sizeof(windows_audio_pi));
    }
#else
    if (linux_audio_pid > 0) {
        kill(linux_audio_pid, SIGTERM);
        sleep_ms(20);
        kill(linux_audio_pid, SIGKILL);
        waitpid(linux_audio_pid, NULL, 0);
        linux_audio_pid = 0;
    }
#endif
    audio_backend = AUDIO_NONE;
}

void play_current_track() {
    stop_audio();
    char track_path[2048];
    if (current_track >= 0 && current_track < track_catalog_count) resolve_audio_path(track_catalog[current_track].path, track_path, sizeof(track_path));
    else get_track_path(current_track, track_path, sizeof(track_path));
    if (!file_exists_path(track_path)) {
        set_audio_status("Audio error: missing %s", get_clean_track_display_name(current_track));
        return;
    }
#ifdef _WIN32
    char ps_cmd[4096];
    char cmdline[4608];
    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&windows_audio_pi, sizeof(windows_audio_pi));
    snprintf(ps_cmd, sizeof(ps_cmd), "$wmplayer = New-Object -ComObject WMPlayer.OCX; $wmplayer.settings.volume = %d; $wmplayer.URL = '%s'; $wmplayer.controls.play(); while($wmplayer.playState -ne 1) { Start-Sleep -Milliseconds 100 }", get_windows_volume(), track_path);
    snprintf(cmdline, sizeof(cmdline), "powershell.exe -NoProfile -WindowStyle Hidden -Command \"%s\"", ps_cmd);
    if (CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &windows_audio_pi)) {
        audio_backend = AUDIO_WINDOWS_COM;
        set_audio_status("Audio backend: Windows COM");
    } else {
        set_audio_status("Audio error: Windows COM launch failed");
    }
#else
    bool wsl = is_wsl_runtime();
    bool has_mpg123 = command_exists("mpg123");
    bool has_ffplay = command_exists("ffplay");
    linux_audio_pid = fork();
    if (linux_audio_pid == 0) {
        int fd_in = open("/dev/null", O_RDONLY);
        int fd_out = open("/dev/null", O_WRONLY);
        dup2(fd_in, STDIN_FILENO); dup2(fd_out, STDOUT_FILENO); dup2(fd_out, STDERR_FILENO);
        close(fd_in); close(fd_out);

        if (has_mpg123) {
            char vol_str[32]; snprintf(vol_str, sizeof(vol_str), "%d", get_mpg123_gain());
            execlp("mpg123", "mpg123", "-q", "--loop", "-1", "-f", vol_str, track_path, NULL);
        }

        if (has_ffplay) {
            char filter_str[64]; snprintf(filter_str, sizeof(filter_str), "volume=%.2f", get_ffplay_volume_scale());
            execlp("ffplay", "ffplay", "-nodisp", "-autoexit", "-loop", "0", "-volume", "100", "-af", filter_str, track_path, NULL);
        }

        if (wsl) {
            char win_track_path[2048]; linux_path_to_windows(track_path, win_track_path, sizeof(win_track_path));
            char ps_cmd[4096];
            snprintf(ps_cmd, sizeof(ps_cmd), "$wmplayer = New-Object -ComObject WMPlayer.OCX; $wmplayer.settings.volume = %d; $wmplayer.URL = '%s'; $wmplayer.controls.play(); while($wmplayer.playState -ne 1) { Start-Sleep -Milliseconds 100 }", get_windows_volume(), win_track_path);
            execlp("powershell.exe", "powershell.exe", "-NoProfile", "-WindowStyle", "Hidden", "-Command", ps_cmd, NULL);
        }

        exit(1);
    }
    if (linux_audio_pid <= 0) set_audio_status("Audio error: failed to fork player");
    else if (has_mpg123) { audio_backend = AUDIO_MPG123; set_audio_status("Audio backend: mpg123"); }
    else if (has_ffplay) { audio_backend = AUDIO_FFPLAY; set_audio_status("Audio backend: ffplay"); }
    else if (wsl) { audio_backend = AUDIO_WINDOWS_COM; set_audio_status("Audio backend: Windows COM fallback"); }
    else set_audio_status("Audio unavailable: install mpg123 or ffplay");
#endif
}

void play_victory_track() {
    char track_path[2048];
    resolve_audio_path(hidden_victory_track_path, track_path, sizeof(track_path));
    if (!file_exists_path(track_path)) return;
    victory_track_active = true;
    stop_audio();
#ifdef _WIN32
    char ps_cmd[4096];
    char cmdline[4608];
    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&windows_audio_pi, sizeof(windows_audio_pi));
    snprintf(ps_cmd, sizeof(ps_cmd), "$wmplayer = New-Object -ComObject WMPlayer.OCX; $wmplayer.settings.volume = %d; $wmplayer.URL = '%s'; $wmplayer.controls.play(); while($wmplayer.playState -ne 1) { Start-Sleep -Milliseconds 100 }", get_windows_volume(), track_path);
    snprintf(cmdline, sizeof(cmdline), "powershell.exe -NoProfile -WindowStyle Hidden -Command \"%s\"", ps_cmd);
    if (CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &windows_audio_pi)) {
        audio_backend = AUDIO_WINDOWS_COM;
        set_audio_status("Victory track: %s", hidden_victory_track_label);
    }
#else
    bool wsl = is_wsl_runtime();
    bool has_mpg123 = command_exists("mpg123");
    bool has_ffplay = command_exists("ffplay");
    linux_audio_pid = fork();
    if (linux_audio_pid == 0) {
        int fd_in = open("/dev/null", O_RDONLY);
        int fd_out = open("/dev/null", O_WRONLY);
        dup2(fd_in, STDIN_FILENO); dup2(fd_out, STDOUT_FILENO); dup2(fd_out, STDERR_FILENO);
        close(fd_in); close(fd_out);
        if (has_mpg123) {
            char vol_str[32]; snprintf(vol_str, sizeof(vol_str), "%d", get_mpg123_gain());
            execlp("mpg123", "mpg123", "-q", "-f", vol_str, track_path, NULL);
        }
        if (has_ffplay) {
            char filter_str[64]; snprintf(filter_str, sizeof(filter_str), "volume=%.2f", get_ffplay_volume_scale());
            execlp("ffplay", "ffplay", "-nodisp", "-autoexit", "-volume", "100", "-af", filter_str, track_path, NULL);
        }
        if (wsl) {
            char win_track_path[2048]; linux_path_to_windows(track_path, win_track_path, sizeof(win_track_path));
            char ps_cmd[4096];
            snprintf(ps_cmd, sizeof(ps_cmd), "$wmplayer = New-Object -ComObject WMPlayer.OCX; $wmplayer.settings.volume = %d; $wmplayer.URL = '%s'; $wmplayer.controls.play(); while($wmplayer.playState -ne 1) { Start-Sleep -Milliseconds 100 }", get_windows_volume(), win_track_path);
            execlp("powershell.exe", "powershell.exe", "-NoProfile", "-WindowStyle", "Hidden", "-Command", ps_cmd, NULL);
        }
        exit(1);
    }
    if (linux_audio_pid > 0) set_audio_status("Victory track: %s", hidden_victory_track_label);
#endif
}

void restore_background_music() {
    if (!victory_track_active) return;
    victory_track_active = false;
    play_current_track();
}

void set_audio_volume() { play_current_track(); }
bool handle_music_hotkeys(int c);

/* -------------------------------------------------------------------------- */
/* TERMINAL & REAL-TIME INPUT UTILITIES                                       */
/* -------------------------------------------------------------------------- */

#ifdef _WIN32
DWORD orig_console_mode = 0; HANDLE hOut = INVALID_HANDLE_VALUE;
void init_terminal() {
    hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) { GetConsoleMode(hOut, &orig_console_mode); SetConsoleOutputCP(CP_UTF8); }
}
void set_terminal_mode(int enable) {
    if (hOut == INVALID_HANDLE_VALUE) return;
    if (enable) { DWORD dwMode = orig_console_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING; SetConsoleMode(hOut, dwMode); } 
    else { SetConsoleMode(hOut, orig_console_mode); }
}

void flush_stdin_buffer() {
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hIn != INVALID_HANDLE_VALUE) FlushConsoleInputBuffer(hIn);
}
#else
struct termios orig_termios;
void init_terminal() { tcgetattr(STDIN_FILENO, &orig_termios); }
void set_terminal_mode(int enable) {
    if (enable) {
        struct termios raw = orig_termios; raw.c_lflag &= ~(ICANON | ECHO); raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    } else { tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios); }
}

void flush_stdin_buffer() {
    tcflush(STDIN_FILENO, TCIFLUSH);
}
#endif

void restore_terminal() {
    const char* reset_str = "\033[0m\033[?1049l\033[?25h"; 
#ifdef _WIN32
    if (hOut != INVALID_HANDLE_VALUE) { DWORD written; WriteConsoleA(hOut, reset_str, strlen(reset_str), &written, NULL); }
#else
    (void)!write(STDOUT_FILENO, reset_str, strlen(reset_str));
    int stty_rc = system("stty sane 2>/dev/null");
    (void)stty_rc;
#endif
    set_terminal_mode(0);
}

void cleanup_all() { stop_audio(); restore_terminal(); }

void begin_text_entry_screen(const char* title) {
    set_terminal_mode(0);
    flush_stdin_buffer();
    clear_screen();
    update_term_size();
    if (title && title[0] != '\0') {
        printf("\n");
        pm_custom((int)strlen(title));
        print_theme(title, "title");
        printf("\n\n");
    }
    fflush(stdout);
}

void end_text_entry_screen() {
    printf("\033[H\033[2J");
    fflush(stdout);
    flush_stdin_buffer();
    set_terminal_mode(1);
}

void begin_soft_frame() {
    printf("\033[H\033[J");
}

void end_soft_frame() {
    fflush(stdout);
}

void set_frame_row_estimate(int rows) {
    frame_row_estimate = rows < 0 ? 0 : rows;
}

void wait_for_enter_or_sigint() {
    while (!sigint_flag) {
        if (kbhit_portable()) {
            int c = read_key();
            if (c == KEY_ENTER) break;
            if (c == -1) break;
        } else {
            sleep_ms(25);
        }
    }
}

void sleep_ms(int ms) {
#ifdef _WIN32
    Sleep(ms);
#else
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000000 }; nanosleep(&ts, NULL);
#endif
}

static unsigned long long now_ms(void) {
#ifdef _WIN32
    return (unsigned long long)GetTickCount();
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long long)tv.tv_sec * 1000ULL + (unsigned long long)tv.tv_usec / 1000ULL;
#endif
}

int kbhit_portable() {
#ifdef _WIN32
    return _kbhit();
#else
    struct timeval tv; fd_set fds;
    while(1) {
        tv.tv_sec = 0; tv.tv_usec = 0;
        FD_ZERO(&fds); FD_SET(STDIN_FILENO, &fds);
        int ret = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
        if (ret == -1) {
            if (errno == EINTR) {
                if (sigint_flag) return 1;
                continue; // Prevent crash when returning from background
            }
            return 0;
        }
        return ret > 0;
    }
#endif
}

int read_key() {
#ifdef _WIN32
    int c = _getch();
    if (c == 224 || c == 0) {
        c = _getch();
        if (c == 72) return KEY_UP;
        if (c == 80) return KEY_DOWN;
        if (c == 75) return KEY_LEFT;
        if (c == 77) return KEY_RIGHT;
        return 0;
    }
    if (c == '\r' || c == '\n') return KEY_ENTER;
    if (c == '\b') return KEY_BACKSPACE;
    return c;
#else
    unsigned char c; int n;
    while (1) {
        n = read(STDIN_FILENO, &c, 1);
        if (n == -1) {
            if (errno == EINTR) {
                if (sigint_flag) return 0;
                continue;
            }
            return -1; // Error state
        }
        if (n == 0) return -1; // EOF (Terminal suspended/detached)
        break;
    }
    if (c == 27) {
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
        if (flags == -1) return 27;
        if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) == -1) return 27;

        unsigned char seq[16]; int seq_len = 0;
        for (int i = 0; i < (int)sizeof(seq); i++) {
            unsigned char b; int r = read(STDIN_FILENO, &b, 1);
            if (r == 1) { seq[seq_len++] = b; if ((b >= 'A' && b <= 'Z') || b == '~') break; continue; }
            if (r == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                struct timeval tv = {0, 15000}; fd_set fds; FD_ZERO(&fds); FD_SET(STDIN_FILENO, &fds);
                if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) continue;
            }
            break;
        }
        fcntl(STDIN_FILENO, F_SETFL, flags);
        if (seq_len >= 2 && (seq[0] == '[' || seq[0] == 'O')) {
            unsigned char last = seq[seq_len - 1];
            if (last == 'A') return KEY_UP; if (last == 'B') return KEY_DOWN;
            if (last == 'C') return KEY_RIGHT; if (last == 'D') return KEY_LEFT;
        }
        return 27;
    }
    if (c == '\n' || c == '\r') return KEY_ENTER; if (c == 127 || c == '\b') return KEY_BACKSPACE; return c;
#endif
}

/* -------------------------------------------------------------------------- */
/* THEMES & UI FORMATTING                                                     */
/* -------------------------------------------------------------------------- */

int term_w = 80, term_h = 24;

void update_term_size() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        term_w = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        term_h = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
#else
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        term_w = w.ws_col;
        term_h = w.ws_row;
    }
#endif
    if (term_w < 40) term_w = 40;
    if (term_h < 10) term_h = 10;
}

void pm_custom(int width) {
    int margin = (term_w - width) / 2;
    if (margin > 0) {
        for(int i=0; i<margin; i++) putchar(' ');
    }
}

void pm_board_custom(int board_width, int left_extra) {
    int margin = (term_w - board_width) / 2 - left_extra;
    if (margin < 0) margin = 0;
    for (int i = 0; i < margin; i++) putchar(' ');
}

const char* get_pet_name() {
    static const char* pet_names[] = {"None", "Chiikawa", "Hachiware", "Usagi", "Momonga"};
    if (current_pet < 0 || current_pet > 4) return "None";
    return pet_names[current_pet];
}

const char* get_language_name() {
    static const char* names[] = {"English", "繁體中文", "简体中文", "日本語"};
    if (current_language < 0 || current_language > 3) return "English";
    return names[current_language];
}

const char* tr4(const char* en, const char* zh_hk, const char* zh_cn, const char* jp) {
    switch (current_language) {
        case 1: return zh_hk;
        case 2: return zh_cn;
        case 3: return jp;
        default: return en;
    }
}

typedef struct {
    const char* en;
    const char* zh_hk;
    const char* zh_cn;
    const char* jp;
} UiText;

static const char* localize_ui_text_exact(const char* text) {
    static const UiText map[] = {
        {"Single Player", "單人模式", "单人模式", "シングルプレイ"},
        {"Arcade Mode", "街機模式", "街机模式", "アーケードモード"},
        {"Profile & Settings", "個人檔案與設定", "个人资料与设置", "プロフィールと設定"},
        {"Import / Build custom game", "匯入 / 建立自訂遊戲", "导入 / 创建自定义游戏", "インポート / カスタム作成"},
        {"Help", "說明", "帮助", "ヘルプ"},
        {"Quit", "離開", "退出", "終了"},
        {"START GAME", "開始遊戲", "开始游戏", "ゲーム開始"},
        {"BACK", "返回", "返回", "戻る"},
        {"Fixed Room", "固定房", "固定房", "固定ルーム"},
        {"Random Room", "隨機房", "随机房", "ランダムルーム"},
        {"Easy", "簡單", "简单", "やさしい"},
        {"Median", "中等", "中等", "普通"},
        {"Hard", "困難", "困难", "難しい"},
        {"Nightmare", "夢魘", "梦魇", "ナイトメア"},
        {"Normal", "普通", "普通", "ノーマル"},
        {"Time Attack", "計時對戰", "计时对战", "タイムアタック"},
        {"Puzzle Sprint", "拼圖衝刺", "拼图冲刺", "パズルスプリント"},
        {"Daily Challenge", "每日挑戰", "每日挑战", "デイリーチャレンジ"},
        {"Hardcore Run", "硬核闖關", "硬核闯关", "ハードコアラン"},
        {"RUN CLOSED CLEANLY", "已安全結束", "已安全结束", "正常終了"},
        {"Thanks for playing Inshi-no-Heya", "感謝遊玩 Inshi-no-Heya", "感谢游玩 Inshi-no-Heya", "Inshi-no-Heya を遊んでくれてありがとう"},
        {"MUSIC PLAYER", "音樂播放器", "音乐播放器", "音楽プレイヤー"},
        {"OPTIONS MENU", "選項選單", "选项菜单", "オプションメニュー"},
        {"Commands: WASD/Arrows | 1-9 fill | 0 clear | [H]int | [C]heck | [O]ption | [Q]uit", "指令: WASD/方向鍵 | 1-9填入 | 0清除 | [H]提示 | [C]檢查 | [O]選項 | [Q]離開", "指令: WASD/方向键 | 1-9填入 | 0清除 | [H]提示 | [C]检查 | [O]选项 | [Q]退出", "操作: WASD/矢印 | 1-9入力 | 0消去 | [H]ヒント | [C]チェック | [O]オプション | [Q]終了"},
        {"Commands: 'row col val' | 'hint' | 'check' | 'clear' | 'idk' | 'option' | 'exit'", "指令: '列 行 值' | 'hint' | 'check' | 'clear' | 'idk' | 'option' | 'exit'", "指令: '行 列 值' | 'hint' | 'check' | 'clear' | 'idk' | 'option' | 'exit'", "操作: '行 列 値' | 'hint' | 'check' | 'clear' | 'idk' | 'option' | 'exit'"},
        {"Commands: WASD/Arrows | 1-9 fill | 0 clear | [C]heck | [O]ption | [Q]uit", "指令: WASD/方向鍵 | 1-9填入 | 0清除 | [C]檢查 | [O]選項 | [Q]離開", "指令: WASD/方向键 | 1-9填入 | 0清除 | [C]检查 | [O]选项 | [Q]退出", "操作: WASD/矢印 | 1-9入力 | 0消去 | [C]チェック | [O]オプション | [Q]終了"},
        {"Commands: 'row col val' | 'check' | 'clear' | 'option' | 'exit'", "指令: '列 行 值' | 'check' | 'clear' | 'option' | 'exit'", "指令: '行 列 值' | 'check' | 'clear' | 'option' | 'exit'", "操作: '行 列 値' | 'check' | 'clear' | 'option' | 'exit'"},
        {"Enter Command: ", "輸入指令: ", "输入指令: ", "コマンド入力: "},
        {"Time:", "時間:", "时间:", "時間:"},
        {"Hints used:", "已用提示:", "已用提示:", "使用ヒント:"},
        {"Hearts:", "愛心:", "爱心:", "ハート:"},
        {"Companion Preview", "夥伴預覽", "伙伴预览", "相棒プレビュー"},
        {"Achievements Preview", "成就預覽", "成就预览", "実績プレビュー"},
        {"Latest Milestones", "最新里程碑", "最新里程碑", "最新マイルストーン"},
        {"No achievements yet. Your first win will unlock one.", "尚未解鎖成就，首次勝利後會解鎖。", "尚未解锁成就，首次胜利后会解锁。", "まだ実績はありません。初勝利で解除されます。"},
        {"[W/S] Move   [A/D or Enter] Change   [Q] Back", "[W/S] 移動   [A/D或Enter] 更改   [Q] 返回", "[W/S] 移动   [A/D或Enter] 更改   [Q] 返回", "[W/S] 移動   [A/D/Enter] 変更   [Q] 戻る"},
        {"Room Type: Fixed Room", "房間類型: 固定房", "房间类型: 固定房", "ルームタイプ: 固定"},
        {"Room Type: Random Room", "房間類型: 隨機房", "房间类型: 随机房", "ルームタイプ: ランダム"},
        {"Board Size:", "棋盤大小:", "棋盘大小:", "盤面サイズ:"},
        {"Difficulty:", "難度:", "难度:", "難易度:"},
        {"Game Mode:", "模式:", "模式:", "モード:"},
        {"Ghost AI Diff:", "幽靈AI難度:", "幽灵AI难度:", "ゴーストAI難易度:"},
        {"PLAYER", "玩家", "玩家", "プレイヤー"},
        {"GHOST AI", "幽靈AI", "幽灵AI", "ゴーストAI"},
        {"Press Enter...", "按 Enter 繼續...", "按 Enter 继续...", "Enterで続行..."},
        {"Press Enter to return.", "按 Enter 返回。", "按 Enter 返回。", "Enterで戻る。"},
        {"Press Enter to return to menu...", "按 Enter 返回主選單...", "按 Enter 返回主菜单...", "Enterでメニューに戻る..."},
        {"Switched to WASD/Arrows mode.", "已切換為 WASD/方向鍵模式。", "已切换为 WASD/方向键模式。", "WASD/矢印モードに切り替えました。"},
        {"Wrong! -1 Heart. Hearts left: %d", "錯誤! -1 愛心。剩餘: %d", "错误! -1 爱心。剩余: %d", "ミス! ハート-1。残り: %d"},
        {"That cell is a locked hint.", "該格是鎖定提示格。", "该格是锁定提示格。", "そのマスはヒントで固定されています。"},
        {"Board cleared.", "棋盤已清空。", "棋盘已清空。", "盤面をクリアしました。"},
        {"Conflicts highlighted in red.", "衝突已以紅色標示。", "冲突已以红色标示。", "競合を赤で表示しました。"},
        {"Locked hint.", "鎖定提示格。", "锁定提示格。", "固定ヒントです。"},
        {"Invalid input.", "輸入無效。", "输入无效。", "無効な入力です。"}
    };
    int n = (int)(sizeof(map) / sizeof(map[0]));
    for (int i = 0; i < n; i++) {
        if (strcmp(text, map[i].en) == 0) return tr4(map[i].en, map[i].zh_hk, map[i].zh_cn, map[i].jp);
    }
    return text;
}

const char* localize_ui_text(const char* text) {
    static char with_newline[1024];
    static char temp[1024];
    if (!text || text[0] == '\0' || current_language == 0) return text;
    size_t len = strlen(text);
    if (len > 0 && text[len - 1] == '\n') {
        size_t base_len = len - 1;
        if (base_len >= sizeof(temp)) base_len = sizeof(temp) - 1;
        memcpy(temp, text, base_len);
        temp[base_len] = '\0';
        const char* core = localize_ui_text_exact(temp);
        snprintf(with_newline, sizeof(with_newline), "%s\n", core);
        return with_newline;
    }
    return localize_ui_text_exact(text);
}

static int utf8_char_len(unsigned char c) {
    if (c < 0x80) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

static int utf8_display_width(const char* s) {
    if (!s) return 0;
    int w = 0;
    const unsigned char* p = (const unsigned char*)s;
    while (*p) {
        int n = utf8_char_len(*p);
        int i;
        for (i = 1; i < n; i++) {
            if (p[i] == '\0') { n = i; break; }
        }
        w += (*p < 0x80) ? 1 : 2;
        p += n;
    }
    return w;
}

static void utf8_copy_cols(char* dst, size_t dst_size, const char* src, int max_cols) {
    if (!dst || dst_size == 0) return;
    dst[0] = '\0';
    if (!src || max_cols <= 0) return;

    size_t out = 0;
    int cols = 0;
    const unsigned char* p = (const unsigned char*)src;
    while (*p && cols < max_cols) {
        int n = utf8_char_len(*p);
        int cw = (*p < 0x80) ? 1 : 2;
        if (cols + cw > max_cols) break;
        if (out + (size_t)n >= dst_size) break;
        for (int i = 0; i < n && p[i]; i++) dst[out++] = (char)p[i];
        cols += cw;
        p += n;
    }
    dst[out] = '\0';
}

void draw_companion(int tick) {
    if (term_h < 15 || term_w < 40) return;
    if (current_pet == 0) return;
    if (frame_row_estimate > 0 && frame_row_estimate + 8 > term_h) return;

    int frame = tick % 2;
    const char* chiikawa1[7] = {
        "   / \\--/ \\",
        "  ( \\/  \\/ )",
        " (  o  w  o )",
        " /          \\",
        "(  (      )  )",
        " \\  '----'  /",
        "  '--'  '--'"
    };
    const char* chiikawa2[7] = {
        "   / \\--/ \\",
        "  ( \\/  \\/ )",
        " (  -  w  - )",
        " /          \\",
        "(  (      )  )",
        " \\  '----'  /",
        "  '--'  '--'"
    };
    const char* hachiware1[7] = {
        "   / \\~~/ \\",
        "  ( \\/  \\/ )",
        " (  o  w  o )",
        " /   == ==  \\",
        "(  (      )  )",
        " \\  '----'  /",
        "  '--'  '--'"
    };
    const char* hachiware2[7] = {
        "   / \\~~/ \\",
        "  ( \\/  \\/ )",
        " (  -  w  - )",
        " /   == ==  \\",
        "(  (      )  )",
        " \\  '----'  /",
        "  '--'  '--'"
    };
    const char* usagi1[7] = {
        "   /\\    /\\",
        "  /  \\--/  \\",
        " (   o w o  )",
        " /          \\",
        "(  (      )  )",
        " \\  '----'  /",
        "  '--'  '--'"
    };
    const char* usagi2[7] = {
        "   /\\    /\\",
        "  /  \\--/  \\",
        " (   - w -  )",
        " /          \\",
        "(  (      )  )",
        " \\  '----'  /",
        "  '--'  '--'"
    };
    const char* momonga1[7] = {
        "   /\\____/\\",
        "  ( / .. \\ )",
        " (  o  w  o )",
        " /   ====   \\",
        "(  (      )  )",
        " \\  '----'  /",
        "  '--'  '--'"
    };
    const char* momonga2[7] = {
        "   /\\____/\\",
        "  ( / .. \\ )",
        " (  -  w  - )",
        " /   ====   \\",
        "(  (      )  )",
        " \\  '----'  /",
        "  '--'  '--'"
    };

    const char* const* pet = chiikawa1;
    const char* color = "1;37";
    if (current_pet == 1) pet = (frame == 0) ? chiikawa1 : chiikawa2;
    else if (current_pet == 2) { pet = (frame == 0) ? hachiware1 : hachiware2; color = "1;36"; }
    else if (current_pet == 3) { pet = (frame == 0) ? usagi1 : usagi2; color = "1;35"; }
    else if (current_pet == 4) { pet = (frame == 0) ? momonga1 : momonga2; color = "1;33"; }

    int x = term_w - 30;
    int y = term_h - 10;
    if (x < 1) x = 1;
    if (y < 1) y = 1;
    for (int i = 0; i < 7; i++) {
        if (y + i > term_h) break;
        printf("\033[%d;%dH\033[%sm%s\033[0m", y + i, x, color, pet[i]);
    }
}

void draw_companion_preview_centered(int width) {
    const char* color = "1;33";
    const char* lines[7];

    if (current_pet == 1) {
        color = "1;37";
        lines[0] = "   / \\--/ \\";
        lines[1] = "  ( \\/  \\/ )";
        lines[2] = " (  o  w  o )";
        lines[3] = " /          \\";
        lines[4] = "(  (      )  )";
        lines[5] = " \\  '----'  /";
        lines[6] = "  '--'  '--'";
    } else if (current_pet == 2) {
        color = "1;36";
        lines[0] = "   / \\~~/ \\";
        lines[1] = "  ( \\/  \\/ )";
        lines[2] = " (  o  w  o )";
        lines[3] = " /   == ==  \\";
        lines[4] = "(  (      )  )";
        lines[5] = " \\  '----'  /";
        lines[6] = "  '--'  '--'";
    } else if (current_pet == 3) {
        color = "1;35";
        lines[0] = "   /\\    /\\";
        lines[1] = "  /  \\--/  \\";
        lines[2] = " (   o w o  )";
        lines[3] = " /          \\";
        lines[4] = "(  (      )  )";
        lines[5] = " \\  '----'  /";
        lines[6] = "  '--'  '--'";
    } else if (current_pet == 4) {
        color = "1;33";
        lines[0] = "   /\\____/\\";
        lines[1] = "  ( / .. \\ )";
        lines[2] = " (  o  w  o )";
        lines[3] = " /   ====   \\";
        lines[4] = "(  (      )  )";
        lines[5] = " \\  '----'  /";
        lines[6] = "  '--'  '--'";
    } else {
        color = "1;30";
        lines[0] = "     [ none ]";
        lines[1] = "   [  quiet  ]";
        lines[2] = "  [  to be   ]";
        lines[3] = "  [  cuddled ]";
        lines[4] = "   [________]";
        lines[5] = "             ";
        lines[6] = "             ";
    }

    for (int i = 0; i < 7; i++) {
        pm_custom(width);
        printf("\033[%sm", color);
        print_padded(lines[i], width, "center", "");
        printf("\033[0m\n");
    }
}

// Global Custom Loop definitions
#define MENU_LOOP_BEGIN(tick_var) \
    int tick_var = 0; \
    while(1) { \
        while(!kbhit_portable()) { \
            update_term_size(); \
            begin_soft_frame();

#define MENU_LOOP_END(tick_var) \
            draw_companion(tick_var++); \
            fflush(stdout); \
            int _w = 0; \
            while(_w < 320) { if(kbhit_portable() || sigint_flag) break; sleep_ms(40); _w += 40; } \
            if(kbhit_portable() || sigint_flag) break; \
        } \
        if(sigint_flag) { restore_terminal(); exit(0); } \
        int c = read_key(); \
        if(c == -1) { restore_terminal(); exit(1); } // Protect against EOF infinite loops

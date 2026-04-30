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
int _stricmp(const char* lhs, const char* rhs);
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
    fgets(buf, sizeof(buf), f);
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
    write(STDOUT_FILENO, reset_str, strlen(reset_str));
    system("stty sane 2>/dev/null");
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

void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

const char* get_theme_color(const char* kind) {
    if (current_theme == 1) {
        if (strcmp(kind, "title") == 0) return "1;33";
        if (strcmp(kind, "accent") == 0) return "1;32";
        if (strcmp(kind, "info") == 0) return "37";
        if (strcmp(kind, "success") == 0) return "1;32";
        if (strcmp(kind, "warning") == 0) return "1;33";
        if (strcmp(kind, "danger") == 0) return "1;31";
        if (strcmp(kind, "muted") == 0) return "2;37";
        if (strcmp(kind, "value") == 0) return "1;36";
        if (strcmp(kind, "locked") == 0) return "1;30";
        if (strcmp(kind, "clue") == 0) return "1;33";
        if (strcmp(kind, "header") == 0) return "1;32";
        if (strcmp(kind, "border") == 0) return "33";
        if (strcmp(kind, "cursor") == 0) return "102;30";
    } else if (current_theme == 2) {
        if (strcmp(kind, "title") == 0) return "1;31";
        if (strcmp(kind, "accent") == 0) return "31";
        if (strcmp(kind, "info") == 0) return "37";
        if (strcmp(kind, "success") == 0) return "1;31";
        if (strcmp(kind, "warning") == 0) return "1;33";
        if (strcmp(kind, "danger") == 0) return "1;31";
        if (strcmp(kind, "muted") == 0) return "2;31";
        if (strcmp(kind, "value") == 0) return "1;37";
        if (strcmp(kind, "locked") == 0) return "1;30";
        if (strcmp(kind, "clue") == 0) return "1;31";
        if (strcmp(kind, "header") == 0) return "1;30";
        if (strcmp(kind, "border") == 0) return "31";
        if (strcmp(kind, "cursor") == 0) return "41;37";
    } else if (current_theme == 3) {
        if (strcmp(kind, "title") == 0) return "1;33";
        if (strcmp(kind, "accent") == 0) return "33";
        if (strcmp(kind, "info") == 0) return "37";
        if (strcmp(kind, "success") == 0) return "32";
        if (strcmp(kind, "warning") == 0) return "1;31";
        if (strcmp(kind, "danger") == 0) return "31";
        if (strcmp(kind, "muted") == 0) return "2;33";
        if (strcmp(kind, "value") == 0) return "1;37";
        if (strcmp(kind, "locked") == 0) return "1;30";
        if (strcmp(kind, "clue") == 0) return "1;33";
        if (strcmp(kind, "header") == 0) return "33";
        if (strcmp(kind, "border") == 0) return "33";
        if (strcmp(kind, "cursor") == 0) return "43;30";
    } else if (current_theme == 4) {
        if (strcmp(kind, "title") == 0) return "1;32";
        if (strcmp(kind, "accent") == 0) return "35";
        if (strcmp(kind, "info") == 0) return "32";
        if (strcmp(kind, "success") == 0) return "1;32";
        if (strcmp(kind, "warning") == 0) return "1;33";
        if (strcmp(kind, "danger") == 0) return "1;31";
        if (strcmp(kind, "muted") == 0) return "2;32";
        if (strcmp(kind, "value") == 0) return "1;35";
        if (strcmp(kind, "locked") == 0) return "1;30";
        if (strcmp(kind, "clue") == 0) return "35";
        if (strcmp(kind, "header") == 0) return "1;32";
        if (strcmp(kind, "border") == 0) return "32";
        if (strcmp(kind, "cursor") == 0) return "42;30";
    }

    if (strcmp(kind, "title") == 0) return "1;36";
    if (strcmp(kind, "accent") == 0) return "1;35";
    if (strcmp(kind, "info") == 0) return "36";
    if (strcmp(kind, "success") == 0) return "32";
    if (strcmp(kind, "warning") == 0) return "33";
    if (strcmp(kind, "danger") == 0) return "31";
    if (strcmp(kind, "muted") == 0) return "2";
    if (strcmp(kind, "value") == 0) return "32;1";
    if (strcmp(kind, "locked") == 0) return "36;1";
    if (strcmp(kind, "clue") == 0) return "33;1";
    if (strcmp(kind, "header") == 0) return "35;1";
    if (strcmp(kind, "border") == 0) return "34";
    if (strcmp(kind, "cursor") == 0) return "103;30";
    return "";
}

void print_theme(const char* text, const char* kind) {
    const char* shown = localize_ui_text(text ? text : "");
    if (USE_COLOR) printf("\033[%sm%s\033[0m", get_theme_color(kind), shown);
    else printf("%s", shown);
}

void print_padded(const char* text, int width, const char* align, const char* kind) {
    char buf[512];
    char shown_text[512];
    if (!text) text = "";
    text = localize_ui_text(text);
    if (!align) align = "ljust";
    if (width < 0) width = 0;
    if (term_w > 2 && width > term_w - 2) width = term_w - 2;

    int text_w = utf8_display_width(text);
    int shown = text_w > width ? width : text_w;
    if (shown < 0) shown = 0;
    utf8_copy_cols(shown_text, sizeof(shown_text), text, shown);
    int shown_w = utf8_display_width(shown_text);

    int left = 0;
    int right = 0;
    if (strcmp(align, "center") == 0) {
        left = (width - shown_w) / 2;
        right = width - shown_w - left;
    } else if (strcmp(align, "ljust") == 0) {
        right = width - shown_w;
    } else {
        left = width - shown_w;
    }

    if (left < 0) left = 0;
    if (right < 0) right = 0;

    int pos = 0;
    int max = (int)sizeof(buf) - 1;
    for (int i = 0; i < left && pos < max; i++) buf[pos++] = ' ';
    for (int i = 0; shown_text[i] != '\0' && pos < max; i++) buf[pos++] = shown_text[i];
    for (int i = 0; i < right && pos < max; i++) buf[pos++] = ' ';
    buf[pos] = '\0';
    print_theme(buf, kind);
}

void format_duration(int seconds, char* out) { sprintf(out, "%02d:%02d", seconds / 60, seconds % 60); }

const char* get_ach_text(int mask) {
    switch(mask) {
        case ACH_FIRST_WIN: return "First Win: Clear any room once."; case ACH_NO_HINT: return "No Hints Win: Win without using hint command.";
        case ACH_SPEED: return "Speed Runner: Win in 3 minutes or less."; case ACH_HARD: return "Hard Conqueror: Clear a hard random room.";
        case ACH_BOSS: return "Boss Solver: Clear a 5x5 hard random room.";
        case ACH_IRON_HEART: return "Iron Heart: Clear a heart challenge with hearts remaining.";
        case ACH_MEGA_GRID: return "Mega Grid: Clear a 7x7 or larger room.";
        case ACH_SCORE_MASTER: return "Score Master: Earn 1800 points or more in one win.";
        case ACH_NIGHTMARE_SPEED: return "Nightmare Blitz: Beat random nightmare in 5 minutes or less.";
        case ACH_STEADY_HAND: return "Steady Hand: Clear any room in 10 minutes or less.";
        case ACH_HINT_COMMANDER: return "Hint Commander: Use 3+ hints and still win.";
        case ACH_COMEBACK_SPIRIT: return "Comeback Spirit: Win with exactly 1 heart left.";
    } return "";
}

void print_banner_centered() {
    update_term_size();
    int total_height = 5 + 1 + 4; // Height of banner + box
    int v_pad = (term_h - total_height) / 2 - 4; // Shift it up slightly so options fit
    if (v_pad > 0) for(int i=0; i<v_pad; i++) printf("\n");

    const char* lines[] = {
        "  _____ _   _ ____  _   _ ___   _   _  ___    _   _ _____ __   __",
        " |_   _| \\ | / ___|| | | |_ _| | \\ | |/ _ \\  | | | | ____|\\ \\ / /",
        "   | | |  \\| \\___ \\| |_| || |  |  \\| | | | | | |_| |  _|   \\ V / ",
        "   | | | |\\  |___) |  _  || |  | |\\  | |_| | |  _  | |___   | |  ",
        "   |_| |_| \\_|____/|_| |_|___| |_| \\_|\\___/  |_| |_|_____|  |_|  "
    };
    const char* colors_cp[] = {"36", "35", "33", "32", "34"}; const char* colors_ft[] = {"33", "32", "37", "32", "33"};
    const char* colors_hr[] = {"31", "31", "30", "31", "30"}; const char* colors_sp[] = {"33", "1;33", "37", "1;33", "33"};
    const char* colors_bp[] = {"32", "35", "32", "35", "32"};
    const char** c = colors_cp;
    int banner_w = 0;
    for (int i = 0; i < 5; i++) {
        int l = (int)strlen(lines[i]);
        if (l > banner_w) banner_w = l;
    }
    
    if(current_theme == 1) c = colors_ft; else if(current_theme == 2) c = colors_hr; else if(current_theme == 3) c = colors_sp; else if(current_theme == 4) c = colors_bp;
    for(int i=0; i<5; i++) {
        pm_custom(banner_w);
        printf("\033[1;%sm%s\033[0m\n", c[i % 5], lines[i]);
    }
    printf("\n"); 
    pm_custom(36); print_theme("+----------------------------------+\n", "border");
    pm_custom(36); print_theme("|           INSHI-NO-HEYA          |\n", "accent");
    pm_custom(36); print_theme("|", "border");
    print_padded(tr4("  Solve the room. Own the board. ", "   解開房間，掌控棋盤。   ", "   解开房间，掌控棋盘。   ", "  へやを解いて、盤面を制覇。 "), 34, "center", "info");
    print_theme("|\n", "border");
    pm_custom(36); print_theme("+----------------------------------+\n\n", "border");
}

void play_join_animation() {
    update_term_size();
    const char* frames[] = {
        "INITIATING LINK...",
        "DECRYPTING ROOM...",
        "GENERATING CELLS...",
        "LINK ESTABLISHED!"
    };
    for(int i=0; i<4; i++) {
        if (sigint_flag) break;
        printf("\033[H\033[J");
        for(int v=0; v<term_h/2; v++) printf("\n");
        pm_custom(strlen(frames[i]));
        print_theme(frames[i], "success");
        fflush(stdout);
        sleep_ms(300);
    }
    printf("\033[H\033[J");
    fflush(stdout);
}

void play_exit_animation() {
    update_term_size();
    const char* frames[] = {
        "SAVING RUN...",
        "CLOSING DOORS...",
        "FAREWELL, DUNGEON...",
        "SEE YOU NEXT TIME..."
    };
    for (int i = 0; i < 4; i++) {
        printf("\033[H\033[J");
        for (int v = 0; v < term_h / 2 - 2; v++) printf("\n");
        pm_custom(strlen(frames[i]));
        print_theme(frames[i], i < 2 ? "warning" : "accent");
        printf("\n");
        sleep_ms(260);
    }
    printf("\033[H\033[J");
    for (int v = 0; v < term_h / 2 - 4; v++) printf("\n");
    pm_custom(48); print_theme("+----------------------------------------------+\n", "border");
    pm_custom(48); print_theme("|              RUN CLOSED CLEANLY              |\n", "title");
    pm_custom(48); print_theme("|        Thanks for playing Inshi-no-Heya      |\n", "info");
    pm_custom(48); print_theme("+----------------------------------------------+\n", "border");
    fflush(stdout);
    sleep_ms(800);
}

static bool prompt_int_value(const char* label, int min_value, int max_value, int* out_value) {
    char line[64];
    while (1) {
        pm_custom(30);
        printf("%s", label);
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) return false;

        char* end = NULL;
        long value = strtol(line, &end, 10);
        while (end && *end != '\0' && isspace((unsigned char)*end)) end++;
        if (end == line || (end && *end != '\0')) {
            pm_custom(30);
            printf("\033[1;31mPlease enter a whole number.\033[0m\n");
            continue;
        }
        if (value < min_value || value > max_value) {
            pm_custom(30);
            printf("\033[1;31mValue must be between %d and %d.\033[0m\n", min_value, max_value);
            continue;
        }
        *out_value = (int)value;
        return true;
    }
}

static void draw_dialogue_box(int width, const char* speaker, const char* text) {
    if (!text || text[0] == '\0') return;
    if (!speaker || speaker[0] == '\0') speaker = "Guide";
    int box_w = width > 74 ? 74 : width;
    if (box_w < 40) box_w = 40;
    char line[256];
    static char last_line[256] = "";
    static unsigned long long line_started_ms = 0;
    snprintf(line, sizeof(line), "%s: %s", speaker, text);
    unsigned long long now = now_ms();
    if (strcmp(last_line, line) != 0) {
        snprintf(last_line, sizeof(last_line), "%s", line);
        line_started_ms = now;
    }
    int len = (int)strlen(line);
    unsigned long long elapsed = now - line_started_ms;
    int reveal = len;
    if (elapsed < 1000ULL) {
        double t = (double)elapsed / 1000.0;
        double smooth = t * t * (3.0 - 2.0 * t);
        reveal = (int)(smooth * (double)len);
    }
    if (reveal < 1) reveal = 1;
    if (reveal > len) reveal = len;
    pm_custom(box_w);
    print_theme("+", "border");
    for (int i = 0; i < box_w - 2; i++) print_theme("-", "border");
    print_theme("+\n", "border");
    pm_custom(box_w);
    print_theme("| ", "border");
    char shown[256];
    if (reveal < len) {
        snprintf(shown, sizeof(shown), "%.*s_", reveal, line);
    } else {
        snprintf(shown, sizeof(shown), "%s", line);
    }
    print_padded(shown, box_w - 4, "ljust", "info");
    print_theme(" |\n", "border");
    pm_custom(box_w);
    print_theme("+", "border");
    for (int i = 0; i < box_w - 2; i++) print_theme("-", "border");
    print_theme("+\n", "border");
}

static void queue_pet_dialogue(const char* text) {
    if (current_pet == 0 || !text || text[0] == '\0') return;
    unsigned long long now = now_ms();
    if (pet_dialogue_last_hidden_ms != 0 && now < pet_dialogue_last_hidden_ms + 5000ULL) return;
    snprintf(pet_dialogue_speaker, sizeof(pet_dialogue_speaker), "%s", get_pet_name());
    snprintf(pet_dialogue_text, sizeof(pet_dialogue_text), "%s", text);
    pet_dialogue_started_ms = now;
    pet_dialogue_hide_ms = now + 4000ULL;
    pet_dialogue_visible = true;
}

static void draw_pet_dialogue_box(int width) {
    if (current_pet == 0 || !pet_dialogue_visible || pet_dialogue_text[0] == '\0') return;
    unsigned long long now = now_ms();
    if (now >= pet_dialogue_hide_ms) {
        pet_dialogue_visible = false;
        pet_dialogue_last_hidden_ms = now;
        pet_dialogue_text[0] = '\0';
        return;
    }

    draw_dialogue_box(width, pet_dialogue_speaker, pet_dialogue_text);
}

static void set_pet_message(int correct_streak, int idle_stage, const char* fallback) {
    if (current_pet == 0) return;
    if (correct_streak == 3) queue_pet_dialogue(tr4("Combo x3. Perfect rhythm.", "連擊 x3！節奏完美。", "连击 x3！节奏完美。", "コンボ x3！完璧なリズム。"));
    else if (idle_stage == 1) queue_pet_dialogue(tr4("Stuck? Re-check the clue corner and one room at a time.", "卡住了？先重看角落提示，再逐個房間推。", "卡住了？先重看角落提示，再逐个房间推。", "詰まった？左上のヒントから順に見直そう。"));
    else if (idle_stage == 2) queue_pet_dialogue(tr4("Try the smallest possible room product first.", "先從乘積最小的房間開始。", "先从乘积最小的房间开始。", "まず積が小さい部屋から試そう。"));
    else queue_pet_dialogue(fallback);
}

static void print_centered_colored_row(const char* bg, const char* fg, const char* text, int width) {
    char clipped[512];
    if (!text) text = "";
    utf8_copy_cols(clipped, sizeof(clipped), text, width - 2);
    int text_w = utf8_display_width(clipped);
    if (text_w > width) text_w = width;
    int left = (width - text_w) / 2;
    int right = width - left - text_w;
    if (left < 0) left = 0;
    if (right < 0) right = 0;
    printf("%s", bg);
    for (int i = 0; i < left; i++) putchar(' ');
    printf("%s%s\033[0m", fg, clipped);
    printf("%s", bg);
    for (int i = 0; i < right; i++) putchar(' ');
    printf("\033[0m\n");
}

static void overlay_line_text(char* line, int width, int x, const char* text) {
    if (!line || !text || width <= 0) return;
    for (int i = 0; text[i] != '\0'; i++) {
        int px = x + i;
        if (px >= 0 && px < width) line[px] = text[i];
    }
}

static void overlay_center_text(char* line, int width, const char* text) {
    if (!text) return;
    int len = (int)strlen(text);
    int x = (width - len) / 2;
    overlay_line_text(line, width, x, text);
}

static void draw_summary_table(const char* title, const char* subtitle, const char* keys[], const char* values[], int row_count, int width) {
    if (width < 72) width = 72;
    int key_w = 24;
    int val_w = width - key_w - 7;
    if (val_w < 20) val_w = 20;

    pm_custom(width);
    print_theme("+", "border");
    for (int i = 0; i < width - 2; i++) print_theme("-", "border");
    print_theme("+\n", "border");

    if (title && title[0]) {
        pm_custom(width);
        print_theme("| ", "border");
        print_padded(title, width - 4, "center", "title");
        print_theme(" |\n", "border");
    }
    if (subtitle && subtitle[0]) {
        pm_custom(width);
        print_theme("| ", "border");
        print_padded(subtitle, width - 4, "center", "info");
        print_theme(" |\n", "border");
    }

    pm_custom(width);
    print_theme("|", "border");
    for (int i = 0; i < width - 2; i++) print_theme("-", "border");
    print_theme("|\n", "border");

    for (int i = 0; i < row_count; i++) {
        char left[64];
        char right[256];
        snprintf(left, sizeof(left), "%s", keys[i] ? keys[i] : "");
        snprintf(right, sizeof(right), "%s", values[i] ? values[i] : "");

        pm_custom(width);
        print_theme("| ", "border");
        print_padded(left, key_w, "ljust", "header");
        print_theme(" | ", "border");
        print_padded(right, val_w, "ljust", "value");
        print_theme(" |\n", "border");
    }

    pm_custom(width);
    print_theme("+", "border");
    for (int i = 0; i < width - 2; i++) print_theme("-", "border");
    print_theme("+\n", "border");
}

static void render_fullscreen_result_frame(bool win, const char* headline, const char* line1, const char* line2, const char* line3, int frame) {
    update_term_size();
    const char* style = win ? "\033[48;5;229m\033[38;5;94m" : "\033[48;5;255m\033[38;5;240m";
    const char* particles = "*.+o";
    int panel_w = term_w > 78 ? 78 : term_w - 4;
    if (panel_w < 44) panel_w = term_w - 2;
    if (panel_w < 20) panel_w = 20;
    int panel_h = 9;
    int panel_x = (term_w - panel_w) / 2;
    if (panel_x < 1) panel_x = 1;
    int panel_y = term_h / 2 - panel_h / 2;
    if (panel_y < 1) panel_y = 1;
    if (panel_y + panel_h >= term_h) panel_y = term_h - panel_h - 1;
    if (panel_y < 1) panel_y = 1;

    char* line = (char*)malloc((size_t)term_w + 1);
    if (!line) return;

    printf("\033[H\033[?25l");
    for (int row = 0; row < term_h; row++) {
        memset(line, ' ', (size_t)term_w);
        line[term_w] = '\0';

        for (int col = 0; col < term_w; col++) {
            if (win) {
                if (row < term_h / 2 && ((col * 11 + row * 7 + frame * 13) % 83 == 0)) {
                    line[col] = particles[(col + row + frame) & 3];
                }
            } else if (((col * 5 + row * 3 + frame * 9) % 97) == 0) {
                line[col] = '.';
            }
        }

        if (row >= panel_y && row < panel_y + panel_h) {
            int left = panel_x;
            int right = panel_x + panel_w - 1;
            if (left >= 0 && left < term_w) line[left] = '|';
            if (right >= 0 && right < term_w) line[right] = '|';
            if (row == panel_y || row == panel_y + panel_h - 1) {
                for (int col = left; col <= right && col < term_w; col++) {
                    if (col >= 0) line[col] = '-';
                }
                if (left >= 0 && left < term_w) line[left] = '+';
                if (right >= 0 && right < term_w) line[right] = '+';
            }
            if (row == panel_y + 1) overlay_center_text(line, term_w, headline);
            if (row == panel_y + 2) overlay_center_text(line, term_w, line1);
            if (row == panel_y + 3) overlay_center_text(line, term_w, line2);
            if (row == panel_y + 4) overlay_center_text(line, term_w, line3);
            if (row == panel_y + 6) {
                if (win) {
                    overlay_center_text(line, term_w, (frame % 2 == 0) ? " /\\_/\\    Yay!" : " /\\_/\\   *spin*");
                } else {
                    overlay_center_text(line, term_w, (frame % 2 == 0) ? " /\\_/\\   Hmm..." : " /\\_/\\   Retry?");
                }
            }
            if (row == panel_y + 7) {
                overlay_center_text(line, term_w, (frame % 2 == 0) ? "( ^.^ )  / > < \\" : "( ^o^ )  \\ > < /");
            }
        }

        printf("%s%s\033[0m", style, line);
        if (row < term_h - 1) putchar('\n');
    }
    free(line);
    fflush(stdout);
}

static void play_fullscreen_result_animation(bool win, const char* headline, const char* line1, const char* line2, const char* line3) {
    for (int frame = 0; frame < 25; frame++) {
        if (sigint_flag) break;
        render_fullscreen_result_frame(win, headline, line1, line2, line3, frame);
        sleep_ms(100);
    }
    printf("\033[?25h");
    fflush(stdout);
}

static const char* arcade_size_labels[] = {"3x3", "4x4", "5x5", "6x6", "7x7", "8x8", "9x9"};
static const char* get_arcade_diff_label_localized(int idx) {
    switch (idx) {
        case 0: return tr4("Easy", "簡單", "简单", "やさしい");
        case 1: return tr4("Median", "中等", "中等", "普通");
        default: return tr4("Hard", "困難", "困难", "難しい");
    }
}
static const char* get_arcade_ai_label_localized(int idx) {
    switch (idx) {
        case 0: return tr4("Close", "關閉", "关闭", "なし");
        case 1: return tr4("Easy", "簡單", "简单", "やさしい");
        case 2: return tr4("Normal", "普通", "普通", "ノーマル");
        case 3: return tr4("Hard", "困難", "困难", "難しい");
        default: return tr4("Nightmare", "夢魘", "梦魇", "ナイトメア");
    }
}

static void render_arcade_spin_frame(int frame, int size_idx, int diff_idx, int ai_idx, const char* footer) {
    update_term_size();
    const char* bg = "\033[48;5;22m";
    const char* panel_bg = "\033[48;5;58m";
    const char* panel_fg = "\033[1;97m";
    const char* accent_fg = "\033[1;93m";
    const char* muted_fg = "\033[1;92m";
    int panel_h = 10;
    int panel_top = term_h / 2 - panel_h / 2;
    if (panel_top < 2) panel_top = 2;
    if (panel_top + panel_h >= term_h) panel_top = term_h - panel_h - 1;
    if (panel_top < 1) panel_top = 1;
    int wheel = frame % 6;

    printf("\033[H\033[?25l");
    for (int row = 0; row < term_h; row++) {
        const char* row_bg = bg;
        if (row >= panel_top && row < panel_top + panel_h) {
            if (row == panel_top || row == panel_top + panel_h - 1) {
                printf("%s", panel_bg);
                for (int i = 0; i < term_w; i++) putchar(' ');
                printf("\033[0m\n");
            } else if (row == panel_top + 1) {
                print_centered_colored_row(panel_bg, accent_fg, "MINECRAFT TIGER MACHINE", term_w);
            } else if (row == panel_top + 2) {
                print_centered_colored_row(panel_bg, panel_fg, tr4("Rolling board, diff, and ghost style...", "滾動選擇棋盤、難度與幽靈AI...", "滚动选择棋盘、难度与幽灵AI...", "盤面・難易度・ゴーストAIを回転中..."), term_w);
            } else if (row == panel_top + 6) {
                char slot_line[160];
                snprintf(slot_line, sizeof(slot_line), "|  %-8s |  %-10s |  %-10s |",
                    arcade_size_labels[size_idx], get_arcade_diff_label_localized(diff_idx), get_arcade_ai_label_localized(ai_idx));
                print_centered_colored_row(panel_bg, panel_fg, slot_line, term_w);
            } else if (row == panel_top + 7) {
                char reel[128];
                snprintf(reel, sizeof(reel), "  reel  %c   reel  %c   reel  %c  ",
                    "|/-\\"[wheel % 4], "|/-\\"[(wheel + 1) % 4], "|/-\\"[(wheel + 2) % 4]);
                print_centered_colored_row(panel_bg, muted_fg, reel, term_w);
            } else if (row == panel_top + 8) {
                print_centered_colored_row(panel_bg, muted_fg, footer, term_w);
            } else {
                printf("%s", panel_bg);
                for (int i = 0; i < term_w; i++) putchar(' ');
                printf("\033[0m\n");
            }
            continue;
        }

        printf("%s", row_bg);
        for (int i = 0; i < term_w; i++) putchar(' ');
        printf("\033[0m\n");
    }
    fflush(stdout);
}

static void play_arcade_spin_animation(int size_idx, int diff_idx, int ai_idx) {
    for (int frame = 0; frame < 18; frame++) {
        char footer[96];
        snprintf(footer, sizeof(footer), "%s %s | %s | %s",
            tr4("Spinning...", "轉動中...", "转动中...", "回転中..."),
            arcade_size_labels[(size_idx + frame) % 7],
            get_arcade_diff_label_localized((diff_idx + frame) % 3),
            get_arcade_ai_label_localized((ai_idx + frame) % 5));
        render_arcade_spin_frame(frame, (size_idx + frame) % 7, (diff_idx + frame) % 3, (ai_idx + frame) % 5, footer);
        sleep_ms(frame < 10 ? 70 : 100);
    }
    char footer[96];
    snprintf(footer, sizeof(footer), "%s %s | %s | %s",
        tr4("Locked in:", "已鎖定:", "已锁定:", "確定:"),
        arcade_size_labels[size_idx], get_arcade_diff_label_localized(diff_idx), get_arcade_ai_label_localized(ai_idx));
    render_arcade_spin_frame(18, size_idx, diff_idx, ai_idx, footer);
    sleep_ms(2000);
    printf("\033[?25h");
    fflush(stdout);
}

static void build_progress_bar(char* out, size_t out_size, int filled, int width) {
    if (!out || out_size == 0 || width < 1) return;
    if (filled < 0) filled = 0;
    if (filled > width) filled = width;
    int pos = 0;
    for (int i = 0; i < width && pos + 1 < (int)out_size; i++) {
        out[pos++] = (i < filled) ? '#' : '-';
    }
    out[pos] = '\0';
}

static void render_sprint_finish_frame(bool win, int p_idx, int g_idx, int frame, const char* headline, const char* line1, const char* line2, const char* line3) {
    update_term_size();
    const char* bg = win ? "\033[48;5;24m" : "\033[48;5;60m";
    const char* panel_bg = win ? "\033[48;5;23m" : "\033[48;5;61m";
    const char* main_fg = "\033[1;97m";
    const char* accent_fg = win ? "\033[1;93m" : "\033[1;95m";
    const char* muted_fg = win ? "\033[1;37m" : "\033[1;33m";
    int panel_h = 11;
    int panel_top = term_h / 2 - panel_h / 2 - 1;
    if (panel_top < 2) panel_top = 2;
    if (panel_top + panel_h >= term_h) panel_top = term_h - panel_h - 1;
    if (panel_top < 1) panel_top = 1;
    int p_fill = p_idx > 5 ? 5 : p_idx;
    int g_fill = g_idx > 5 ? 5 : g_idx;

    printf("\033[H\033[?25l");
    for (int row = 0; row < term_h; row++) {
        const char* row_bg = bg;
        if (row >= panel_top && row < panel_top + panel_h) {
            if (row == panel_top || row == panel_top + panel_h - 1) {
                printf("%s", panel_bg);
                for (int i = 0; i < term_w; i++) putchar(' ');
                printf("\033[0m\n");
            } else if (row == panel_top + 1) {
                char head[160];
                snprintf(head, sizeof(head), "  %s  ", headline);
                print_centered_colored_row(panel_bg, accent_fg, head, term_w);
            } else if (row == panel_top + 2) {
                print_centered_colored_row(panel_bg, main_fg, line1, term_w);
            } else if (row == panel_top + 4) {
                char bar[16];
                char line[160];
                build_progress_bar(bar, sizeof(bar), p_fill, 5);
                snprintf(line, sizeof(line), "PLAYER  [%s] %d / 5", bar, p_idx);
                print_centered_colored_row(panel_bg, muted_fg, line, term_w);
            } else if (row == panel_top + 5) {
                char bar[16];
                char line[160];
                build_progress_bar(bar, sizeof(bar), g_fill, 5);
                snprintf(line, sizeof(line), "GHOST   [%s] %d / 5", bar, g_idx);
                print_centered_colored_row(panel_bg, muted_fg, line, term_w);
            } else if (row == panel_top + 6) {
                print_centered_colored_row(panel_bg, accent_fg, line2, term_w);
            } else if (row == panel_top + 7) {
                print_centered_colored_row(panel_bg, accent_fg, line3, term_w);
            } else {
                printf("%s", panel_bg);
                for (int i = 0; i < term_w; i++) putchar(' ');
                printf("\033[0m\n");
            }
            continue;
        }

        printf("%s", row_bg);
        for (int i = 0; i < term_w; i++) {
            if (win && ((i + row + frame) % 31 == 0)) putchar('*');
            else if (!win && ((i + row + frame) % 41 == 0)) putchar('.');
            else putchar(' ');
        }
        printf("\033[0m\n");
    }
    fflush(stdout);
}

static void play_sprint_finish_animation(bool win, int p_idx, int g_idx, const char* headline, const char* line1, const char* line2, const char* line3) {
    for (int frame = 0; frame < 25; frame++) {
        if (sigint_flag) break;
        render_sprint_finish_frame(win, p_idx, g_idx, frame, headline, line1, line2, line3);
        sleep_ms(100);
    }
    printf("\033[?25h");
    fflush(stdout);
}

static const char* get_arcade_diff_key(int diff_idx) {
    switch (diff_idx) {
        case 0: return "easy";
        case 1: return "median";
        default: return "hard";
    }
}

static int get_arcade_hearts(int diff_idx) {
    (void)diff_idx;
    return -1;
}

static void launch_arcade_selected_run(int run_mode, int size_idx, int diff_idx, int ai_idx) {
    int size = size_idx + 3;
    const char* diff_str = get_arcade_diff_key(diff_idx);
    int ai_diff = ai_idx; // 0 = no ghost AI

    if (run_mode == 0) {
        Room* rooms_ptr = NULL;
        Room rooms[MAX_SIZE * MAX_SIZE];
        int num_rooms = 0;
        int grid[MAX_SIZE][MAX_SIZE], room_map[MAX_SIZE][MAX_SIZE], labels[MAX_SIZE][MAX_SIZE];
        int player_grid[MAX_SIZE][MAX_SIZE];
        bool locked_cells[MAX_SIZE][MAX_SIZE];

        if (ai_diff == 0 && (size == 3 || size == 5)) {
            load_fixed_puzzle(size == 3 ? 1 : 2, &size, &rooms_ptr, &num_rooms, grid);
            memcpy(rooms, rooms_ptr, sizeof(Room) * num_rooms);
            build_room_map(size, rooms, num_rooms, room_map);
            build_labels(size, rooms, num_rooms, labels);
            create_prefilled_board(size, grid, player_grid, locked_cells, "median");
            play_join_animation();
            game_loop(size, rooms, num_rooms, grid, room_map, labels, player_grid, locked_cells, "fixed", "median", -1, 0);
        } else {
            generate_puzzle(size, rooms, &num_rooms, grid, room_map);
            build_labels(size, rooms, num_rooms, labels);
            create_prefilled_board(size, grid, player_grid, locked_cells, diff_str);
            play_join_animation();
            if (ai_diff <= 0) game_loop(size, rooms, num_rooms, grid, room_map, labels, player_grid, locked_cells, "random", diff_str, get_arcade_hearts(diff_idx), 0);
            else game_loop_time_attack(size, rooms, num_rooms, grid, room_map, labels, player_grid, locked_cells, ai_diff, NULL, 0);
        }
    } else {
        Room rooms[MAX_SIZE * MAX_SIZE];
        int num_rooms;
        int grid[MAX_SIZE][MAX_SIZE], room_map[MAX_SIZE][MAX_SIZE], labels[MAX_SIZE][MAX_SIZE];
        int player_grid[MAX_SIZE][MAX_SIZE];
        bool locked_cells[MAX_SIZE][MAX_SIZE];
        generate_puzzle(size, rooms, &num_rooms, grid, room_map);
        build_labels(size, rooms, num_rooms, labels);
        create_prefilled_board(size, grid, player_grid, locked_cells, diff_str);
        play_join_animation();
        game_loop_sprint(size, ai_diff);
    }
}

void play_arcade_mode_menu() {
    int run_mode = 0;
    int size_idx = 2;
    int diff_idx = 1;
    int ai_idx = 2;
    int selected = 0;

    while (1) {
        update_term_size();
        begin_soft_frame();
        int table_w = 86;
        int block_h = 16;
        int v_pad = (term_h - block_h) / 2;
        if (v_pad < 0) v_pad = 0;
        for (int i = 0; i < v_pad; i++) putchar('\n');

        pm_custom(table_w); print_theme("+----------------------------------------------------------------------------------+\n", "border");
        pm_custom(table_w); print_padded(tr4("ARCADE MODE", "街機模式", "街机模式", "アーケードモード"), table_w, "center", "title"); printf("\n");
        pm_custom(table_w); print_theme("+--------------------+-------------------------------------------------------------+\n", "border");

        char row[256];
        snprintf(row, sizeof(row), "| %-18s | %-59s |", tr4("Game Mode", "遊戲模式", "游戏模式", "ゲームモード"),
            run_mode == 0 ? tr4("Arcade Run", "街機闖關", "街机关卡", "アーケードラン")
                          : tr4("Puzzle Sprint", "拼圖衝刺", "拼图冲刺", "パズルスプリント"));
        pm_custom(table_w); print_padded(row, table_w, "center", selected == 0 ? "cursor" : "info"); printf("\n");
        snprintf(row, sizeof(row), "| %-18s | %-59s |", tr4("Board Size", "棋盤大小", "棋盘大小", "盤面サイズ"), arcade_size_labels[size_idx]);
        pm_custom(table_w); print_padded(row, table_w, "center", selected == 1 ? "cursor" : "info"); printf("\n");
        snprintf(row, sizeof(row), "| %-18s | %-59s |", tr4("Difficulty", "難度", "难度", "難易度"), get_arcade_diff_label_localized(diff_idx));
        pm_custom(table_w); print_padded(row, table_w, "center", selected == 2 ? "cursor" : "info"); printf("\n");
        snprintf(row, sizeof(row), "| %-18s | %-59s |", tr4("Ghost AI Diff", "幽靈AI難度", "幽灵AI难度", "ゴーストAI難易度"), get_arcade_ai_label_localized(ai_idx));
        pm_custom(table_w); print_padded(row, table_w, "center", selected == 3 ? "cursor" : "info"); printf("\n");
        snprintf(row, sizeof(row), "| %-18s | %-59s |",
            tr4("Spin Machine", "老虎機", "老虎机", "スピンマシン"),
            tr4("Randomize all settings", "隨機設定全部選項", "随机设定全部选项", "すべての設定をランダム化"));
        pm_custom(table_w); print_padded(row, table_w, "center", selected == 4 ? "cursor" : "info"); printf("\n");
        snprintf(row, sizeof(row), "| %-18s | %-59s |",
            tr4(">>> START GAME <<<", ">>> 開始遊戲 <<<", ">>> 开始游戏 <<<", ">>> ゲーム開始 <<<"),
            tr4("Press Enter to launch with current setup", "按 Enter 立即以目前設定開始", "按 Enter 立即以当前设定开始", "Enterで現在設定を開始"));
        pm_custom(table_w); print_padded(row, table_w, "center", selected == 5 ? "success" : "clue"); printf("\n");
        snprintf(row, sizeof(row), "| %-18s | %-59s |",
            tr4("Back", "返回", "返回", "戻る"),
            tr4("Return to the main menu", "回到主選單", "回到主菜单", "メインメニューへ戻る"));
        pm_custom(table_w); print_padded(row, table_w, "center", selected == 6 ? "cursor" : "info"); printf("\n");
        pm_custom(table_w); print_theme("+----------------------------------------------------------------------------------+\n", "border");
        char desc[256];
        snprintf(desc, sizeof(desc), "%s  %s %s  AI %s",
            run_mode == 0
                ? tr4("Single-board arcade run. Close = no ghost AI.", "單局街機闖關。Close = 無幽靈AI。", "单局街机关卡。Close = 无幽灵AI。", "単発アーケード。Close = ゴーストなし。")
                : tr4("Fast sprint across five boards. Close = no ghost AI.", "五關連續衝刺。Close = 無幽靈AI。", "五关连续冲刺。Close = 无幽灵AI。", "5面スプリント。Close = ゴーストなし。"),
            tr4("Board", "棋盤", "棋盘", "盤面"), arcade_size_labels[size_idx], get_arcade_ai_label_localized(ai_idx));
        pm_custom(table_w); print_padded(desc, table_w, "center", "clue"); printf("\n");
        pm_custom(table_w); print_padded(" ", table_w, "center", "info"); printf("\n");
        if (selected == 5) {
            pm_custom(table_w); print_padded(tr4("[ ENTER ] START GAME NOW", "[ ENTER ] 立即開始遊戲", "[ ENTER ] 立即开始游戏", "[ ENTER ] 今すぐ開始"), table_w, "center", "success"); printf("\n");
        } else {
            pm_custom(table_w); print_padded(tr4("[W/S] Move  [A/D] Change  [Enter] Select  [Q] Back", "[W/S] 移動  [A/D] 更改  [Enter] 選擇  [Q] 返回", "[W/S] 移动  [A/D] 更改  [Enter] 选择  [Q] 返回", "[W/S] 移動  [A/D] 変更  [Enter] 決定  [Q] 戻る"), table_w, "center", "info"); printf("\n");
        }
        fflush(stdout);
        draw_companion(0);

        int c = read_key();
        if (c == -1) { restore_terminal(); exit(1); }
        if (sigint_flag) { restore_terminal(); exit(0); }
        if (c == KEY_UP || c == 'w' || c == 'W') selected = (selected + 6) % 7;
        else if (c == KEY_DOWN || c == 's' || c == 'S') selected = (selected + 1) % 7;
        else if (c == KEY_LEFT || c == 'a' || c == 'A') {
            if (selected == 0) run_mode = (run_mode + 1) % 2;
            else if (selected == 1) size_idx = (size_idx + 6) % 7;
            else if (selected == 2) diff_idx = (diff_idx + 2) % 3;
            else if (selected == 3) ai_idx = (ai_idx + 4) % 5;
        } else if (c == KEY_RIGHT || c == 'd' || c == 'D') {
            if (selected == 0) run_mode = (run_mode + 1) % 2;
            else if (selected == 1) size_idx = (size_idx + 1) % 7;
            else if (selected == 2) diff_idx = (diff_idx + 1) % 3;
            else if (selected == 3) ai_idx = (ai_idx + 1) % 5;
        } else if (c == KEY_ENTER) {
            if (selected == 4) {
                int spin_size = rand() % 7;
                int spin_diff = rand() % 3;
                int spin_ai = rand() % 5;
                play_arcade_spin_animation(spin_size, spin_diff, spin_ai);
                run_mode = rand() % 2;
                size_idx = spin_size;
                diff_idx = spin_diff;
                ai_idx = spin_ai;
            } else if (selected == 5) {
                launch_arcade_selected_run(run_mode, size_idx, diff_idx, ai_idx);
            } else if (selected == 6) {
                break;
            }
        } else if (c == 'q' || c == 'Q' || c == 27) {
            break;
        }
    }
}

/* -------------------------------------------------------------------------- */
/* STATS & SCORING MANAGEMENT                                                 */
/* -------------------------------------------------------------------------- */

void load_stats() {
    FILE* f = fopen(STATS_FILE, "r");
    if (f) {
        int read = fscanf(f, "wins=%d\nscore=%d\nachievements=%d\nmode=%d\ntheme=%d\nvol=%d\ntrack=%d\npet=%d\nlang=%d\n", 
            &total_wins, &best_score, &unlocked_achievements, &control_mode, &current_theme, &music_volume, &current_track, &current_pet, &current_language);
        if (read < 9) {
            rewind(f);
            read = fscanf(f, "wins=%d\nscore=%d\nachievements=%d\nmode=%d\ntheme=%d\nvol=%d\ntrack=%d\npet=%d\n",
                &total_wins, &best_score, &unlocked_achievements, &control_mode, &current_theme, &music_volume, &current_track, &current_pet);
            if (read < 8) {
                rewind(f);
                fscanf(f, "wins=%d\nscore=%d\nachievements=%d\nmode=%d\ntheme=%d\nvol=%d\ntrack=%d\n",
                    &total_wins, &best_score, &unlocked_achievements, &control_mode, &current_theme, &music_volume, &current_track);
                current_pet = 0;
            }
            current_language = 0;
        }
        {
            if (control_mode > 1 || control_mode < 0) control_mode = 1;
            if (current_theme > 4 || current_theme < 0) current_theme = 0;
            if (music_volume < 0 || music_volume > 200) music_volume = 140;
            if (current_track < 0 || current_track >= num_tracks) current_track = 6;
            if (current_pet < 0 || current_pet > 4) current_pet = 0;
            if (current_language < 0 || current_language > 3) current_language = 0;
        }
        fclose(f);
    }
}
void save_stats() {
    FILE* f = fopen(STATS_FILE, "w");
    if (f) {
        fprintf(f, "wins=%d\nscore=%d\nachievements=%d\nmode=%d\ntheme=%d\nvol=%d\ntrack=%d\npet=%d\nlang=%d\n", 
            total_wins, best_score, unlocked_achievements, control_mode, current_theme, music_volume, current_track, current_pet, current_language);
        fclose(f);
    }
}

int compute_score(int size, double elapsed_seconds, int hints_used, int hearts_left, const char* mode, const char* difficulty) {
    int base = 700 + (size * 140) + (size * size * 18);
    int time_penalty = (int)(elapsed_seconds / 4.0);
    int mode_bonus = 0, difficulty_bonus = 0, speed_bonus = 0, efficiency_bonus = 0, heart_bonus = 0;

    if (strcmp(mode, "random") == 0) mode_bonus = 140;
    else if (strcmp(mode, "custom") == 0) mode_bonus = 60;

    if (strcmp(difficulty, "nightmare") == 0) difficulty_bonus = 750;
    else if (strcmp(difficulty, "super_hard") == 0) difficulty_bonus = 560;
    else if (strcmp(difficulty, "hard") == 0) difficulty_bonus = 320;
    else if (strcmp(difficulty, "median") == 0) difficulty_bonus = 170;
    else if (strcmp(difficulty, "fixed") == 0) difficulty_bonus = 80;
    else if (strcmp(difficulty, "custom") == 0) difficulty_bonus = 220;

    if (elapsed_seconds <= 90.0) speed_bonus = 420;
    else if (elapsed_seconds <= 180.0) speed_bonus = 280;
    else if (elapsed_seconds <= 300.0) speed_bonus = 160;
    else if (elapsed_seconds <= 600.0) speed_bonus = 60;

    if (hints_used == 0) efficiency_bonus = 240;
    else {
        efficiency_bonus = 180 - (hints_used * 45);
        if (efficiency_bonus < 0) efficiency_bonus = 0;
    }

    if (hearts_left > 0) heart_bonus = hearts_left * 85;

    {
        int score = base + mode_bonus + difficulty_bonus + speed_bonus + efficiency_bonus + heart_bonus - time_penalty;
        return score < 100 ? 100 : score;
    }
}

int update_achievements(int size, double elapsed_seconds, int hints_used, int hearts_left, int score, const char* mode, const char* difficulty) {
    int newly_unlocked = 0;
    #define UNLOCK(flag) if (!(unlocked_achievements & (flag))) { unlocked_achievements |= (flag); newly_unlocked |= (flag); }
    UNLOCK(ACH_FIRST_WIN); if (hints_used == 0) { UNLOCK(ACH_NO_HINT); } if (elapsed_seconds <= 180.0) { UNLOCK(ACH_SPEED); }
    if (strcmp(mode, "random") == 0 && (strcmp(difficulty, "hard") == 0 || strcmp(difficulty, "super_hard") == 0 || strcmp(difficulty, "nightmare") == 0)) {
        UNLOCK(ACH_HARD); if (size >= 5) { UNLOCK(ACH_BOSS); }
    }
    if (hearts_left > 0) { UNLOCK(ACH_IRON_HEART); }
    if (size >= 7) { UNLOCK(ACH_MEGA_GRID); }
    if (score >= 1800) { UNLOCK(ACH_SCORE_MASTER); }
    if (strcmp(mode, "random") == 0 && strcmp(difficulty, "nightmare") == 0 && elapsed_seconds <= 300.0) { UNLOCK(ACH_NIGHTMARE_SPEED); }
    if (elapsed_seconds <= 600.0) { UNLOCK(ACH_STEADY_HAND); }
    if (hints_used >= 3) { UNLOCK(ACH_HINT_COMMANDER); }
    if (hearts_left == 1) { UNLOCK(ACH_COMEBACK_SPIRIT); }
    return newly_unlocked;
}

/* -------------------------------------------------------------------------- */
/* PUZZLE LOGIC & DRAWING                                                     */
/* -------------------------------------------------------------------------- */

bool check_win(int size, int player_grid[MAX_SIZE][MAX_SIZE], Room rooms[], int num_rooms) {
    for(int r=0; r<size; r++) for(int c=0; c<size; c++) if (player_grid[r][c] == 0) return false;
    for(int i=0; i<size; i++) {
        bool row_seen[MAX_SIZE + 1] = {false}, col_seen[MAX_SIZE + 1] = {false};
        for(int j=0; j<size; j++) {
            int r_val = player_grid[i][j], c_val = player_grid[j][i];
            if (r_val < 1 || r_val > size || row_seen[r_val]) return false;
            row_seen[r_val] = true;
            if (c_val < 1 || c_val > size || col_seen[c_val]) return false;
            col_seen[c_val] = true;
        }
    }
    for(int i=0; i<num_rooms; i++) {
        int product = 1; for(int j=0; j<rooms[i].num_cells; j++) product *= player_grid[rooms[i].cells[j].r][rooms[i].cells[j].c];
        if (product != rooms[i].product) return false;
    } return true;
}

int find_room_index_by_id(Room rooms[], int num_rooms, int room_id) {
    for (int i = 0; i < num_rooms; i++) if (rooms[i].id == room_id) return i;
    return -1;
}

void compute_conflicts(int size, int player_grid[MAX_SIZE][MAX_SIZE], Room rooms[], int num_rooms, int room_map[MAX_SIZE][MAX_SIZE], bool conflicts[MAX_SIZE][MAX_SIZE]) {
    memset(conflicts, 0, sizeof(bool) * MAX_SIZE * MAX_SIZE);
    for (int r = 0; r < size; r++) {
        for (int c = 0; c < size; c++) {
            int cell = player_grid[r][c];
            if (cell == 0) continue;
            if (cell < 1 || cell > size) { conflicts[r][c] = true; continue; }
            for (int cc = c + 1; cc < size; cc++) if (player_grid[r][cc] == cell) { conflicts[r][c] = true; conflicts[r][cc] = true; }
            for (int rr = r + 1; rr < size; rr++) if (player_grid[rr][c] == cell) { conflicts[r][c] = true; conflicts[rr][c] = true; }
        }
    }
    for (int i = 0; i < num_rooms; i++) {
        int product = 1;
        bool full = true;
        for (int j = 0; j < rooms[i].num_cells; j++) {
            int cell = player_grid[rooms[i].cells[j].r][rooms[i].cells[j].c];
            if (cell == 0) { full = false; break; }
            product *= cell;
        }
        if (full && product != rooms[i].product) {
            for (int j = 0; j < rooms[i].num_cells; j++) conflicts[rooms[i].cells[j].r][rooms[i].cells[j].c] = true;
        } else if (!full) {
            int partial = 1;
            for (int j = 0; j < rooms[i].num_cells; j++) {
                int cell = player_grid[rooms[i].cells[j].r][rooms[i].cells[j].c];
                if (cell > 0) partial *= cell;
            }
            if (rooms[i].product % partial != 0) {
                for (int j = 0; j < rooms[i].num_cells; j++) if (player_grid[rooms[i].cells[j].r][rooms[i].cells[j].c] > 0) conflicts[rooms[i].cells[j].r][rooms[i].cells[j].c] = true;
            }
        }
    }
    (void)room_map;
}

int get_cell_candidates(int size, int row, int col, int player_grid[MAX_SIZE][MAX_SIZE], Room rooms[], int num_rooms, int room_map[MAX_SIZE][MAX_SIZE], int* out_vals, int out_cap) {
    if (player_grid[row][col] != 0) return 0;
    int count = 0;
    int room_idx = find_room_index_by_id(rooms, num_rooms, room_map[row][col]);
    for (int val = 1; val <= size; val++) {
        bool used = false;
        for (int i = 0; i < size; i++) {
            if (player_grid[row][i] == val || player_grid[i][col] == val) { used = true; break; }
        }
        if (used) continue;
        if (room_idx >= 0) {
            int partial = 1;
            int empty_count = 0;
            for (int j = 0; j < rooms[room_idx].num_cells; j++) {
                int rr = rooms[room_idx].cells[j].r;
                int cc = rooms[room_idx].cells[j].c;
                int cell = (rr == row && cc == col) ? val : player_grid[rr][cc];
                if (cell == 0) empty_count++;
                else partial *= cell;
            }
            if (rooms[room_idx].product % partial != 0) continue;
            if (empty_count == 0 && partial != rooms[room_idx].product) continue;
        }
        if (count < out_cap) out_vals[count] = val;
        count++;
    }
    return count;
}

bool reveal_hint_cell(int size, int solution[MAX_SIZE][MAX_SIZE], int player_grid[MAX_SIZE][MAX_SIZE], bool locked_cells[MAX_SIZE][MAX_SIZE], char* sys_msg, size_t sys_msg_size) {
    Point empties[MAX_SIZE * MAX_SIZE];
    int ecnt = 0;
    for (int r = 0; r < size; r++) for (int c = 0; c < size; c++) if (player_grid[r][c] == 0 && !locked_cells[r][c]) empties[ecnt++] = (Point){r, c};
    if (ecnt == 0) {
        snprintf(sys_msg, sys_msg_size, "No empty cells left for a reveal hint.");
        return false;
    }
    Point p = empties[rand() % ecnt];
    player_grid[p.r][p.c] = solution[p.r][p.c];
    locked_cells[p.r][p.c] = true;
    snprintf(sys_msg, sys_msg_size, "Reveal hint: row %d col %d is %d.", p.r + 1, p.c + 1, solution[p.r][p.c]);
    return true;
}

void apply_progressive_hint(int size, int solution[MAX_SIZE][MAX_SIZE], int player_grid[MAX_SIZE][MAX_SIZE], bool locked_cells[MAX_SIZE][MAX_SIZE], Room rooms[], int num_rooms, int room_map[MAX_SIZE][MAX_SIZE], int hint_number, bool conflicts[MAX_SIZE][MAX_SIZE], char* sys_msg, size_t sys_msg_size) {
    bool local_conflicts[MAX_SIZE][MAX_SIZE];
    compute_conflicts(size, player_grid, rooms, num_rooms, room_map, local_conflicts);
    memcpy(conflicts, local_conflicts, sizeof(local_conflicts));

    if (hint_number % 3 == 1) {
        for (int r = 0; r < size; r++) for (int c = 0; c < size; c++) {
            if (local_conflicts[r][c]) {
                snprintf(sys_msg, sys_msg_size, "Check row %d col %d. Something there conflicts with row/col or room product.", r + 1, c + 1);
                return;
            }
        }
        snprintf(sys_msg, sys_msg_size, "No conflicts yet. Your next hint will narrow a forced cell.");
        return;
    }

    if (hint_number % 3 == 2) {
        int best_r = -1, best_c = -1, best_count = MAX_SIZE + 1;
        int best_vals[MAX_SIZE];
        int best_vals_count = 0;
        for (int r = 0; r < size; r++) for (int c = 0; c < size; c++) {
            if (player_grid[r][c] != 0 || locked_cells[r][c]) continue;
            int vals[MAX_SIZE];
            int count = get_cell_candidates(size, r, c, player_grid, rooms, num_rooms, room_map, vals, MAX_SIZE);
            if (count > 0 && count < best_count) {
                best_r = r; best_c = c; best_count = count;
                best_vals_count = count > MAX_SIZE ? MAX_SIZE : count;
                memcpy(best_vals, vals, sizeof(int) * best_vals_count);
            }
        }
        if (best_r >= 0) {
            char cand_buf[64] = "";
            for (int i = 0; i < best_vals_count; i++) {
                char tmp[8];
                snprintf(tmp, sizeof(tmp), "%s%d", i == 0 ? "" : "/", best_vals[i]);
                strncat(cand_buf, tmp, sizeof(cand_buf) - strlen(cand_buf) - 1);
            }
            snprintf(sys_msg, sys_msg_size, "Candidate hint: row %d col %d -> %s", best_r + 1, best_c + 1, cand_buf);
            return;
        }
        snprintf(sys_msg, sys_msg_size, "No clean candidate cell found yet. Try checking conflicts first.");
        return;
    }

    reveal_hint_cell(size, solution, player_grid, locked_cells, sys_msg, sys_msg_size);
}

void draw_grid(int size, int room_map[MAX_SIZE][MAX_SIZE], int labels[MAX_SIZE][MAX_SIZE], 
               int player_grid[MAX_SIZE][MAX_SIZE], bool locked_cells[MAX_SIZE][MAX_SIZE], bool conflicts[MAX_SIZE][MAX_SIZE], int cursor_r, int cursor_c) {
    int board_w = size * 6 + 1;
    pm_board_custom(board_w, 5); printf("     "); for(int i=0; i<size; i++) { char buf[10]; snprintf(buf, sizeof(buf), "%d", i+1); print_padded(buf, 6, "center", "header"); } printf("\033[K\n");
    pm_board_custom(board_w, 5); char border_top[256] = "    "; for(int i=0; i<size; i++) strcat(border_top, "______"); strcat(border_top, "_"); print_theme(border_top, "border"); printf("\033[K\n");

    for(int r=0; r<size; r++) {
        pm_board_custom(board_w, 5); print_theme("    |", "border");
        for(int c=0; c<size; c++) {
            bool has_right = (c == size - 1 || room_map[r][c] != room_map[r][c + 1]); char lbl[10] = "";
            if (labels[r][c] != 0) snprintf(lbl, sizeof(lbl), "%d", labels[r][c]);
            if (lbl[0] != '\0') print_padded(lbl, 5, "ljust", "clue"); else printf("     ");
            print_theme(has_right ? "|" : " ", "border");
        } printf("\033[K\n");

        pm_board_custom(board_w, 5); char r2h[10]; snprintf(r2h, sizeof(r2h), " %2d |", r + 1); print_theme(r2h, "header");
        for(int c=0; c<size; c++) {
            int cell = player_grid[r][c]; bool has_right = (c == size - 1 || room_map[r][c] != room_map[r][c + 1]); char dt[10]; const char* ds;
            if (r == cursor_r && c == cursor_c) {
                if (cell == 0) snprintf(dt, sizeof(dt), "{.}");
                else snprintf(dt, sizeof(dt), "{%d}", cell);
                ds = "cursor";
            }
            else if (conflicts != NULL && conflicts[r][c] && cell != 0) {
                snprintf(dt, sizeof(dt), "!%d!", cell);
                ds = "danger";
            }
            else if (cell == 0) { strcpy(dt, "."); ds = "muted"; } else if (locked_cells[r][c]) { snprintf(dt, sizeof(dt), "[%d]", cell); ds = "locked"; } 
            else { snprintf(dt, sizeof(dt), "%d", cell); ds = "value"; }
            print_padded(dt, 5, "center", ds); print_theme(has_right ? "|" : " ", "border");
        } printf("\033[K\n");

        pm_board_custom(board_w, 5); print_theme("    |", "border");
        for(int c=0; c<size; c++) {
            bool has_bottom = (r == size - 1 || room_map[r][c] != room_map[r+1][c]); bool has_right = (c == size - 1 || room_map[r][c] != room_map[r][c+1]); bool br_corner = (r < size - 1 && room_map[r+1][c] != room_map[r+1][c+1]);
            print_theme(has_bottom ? "_____" : "     ", "border"); print_theme((has_right || br_corner || has_bottom) ? "|" : " ", "border");
        } printf("\033[K\n");
    }
}

void draw_dual_grid(int size, 
                    int room_map1[MAX_SIZE][MAX_SIZE], int labels1[MAX_SIZE][MAX_SIZE], int grid1[MAX_SIZE][MAX_SIZE], bool locked1[MAX_SIZE][MAX_SIZE], bool conflicts1[MAX_SIZE][MAX_SIZE], int cursor_r1, int cursor_c1,
                    int room_map2[MAX_SIZE][MAX_SIZE], int labels2[MAX_SIZE][MAX_SIZE], int grid2[MAX_SIZE][MAX_SIZE], bool locked2[MAX_SIZE][MAX_SIZE], bool conflicts2[MAX_SIZE][MAX_SIZE], int cursor_r2, int cursor_c2) {
    int single_board_w = size * 6 + 1;
    int total_w = single_board_w * 2 + 17;
    pm_board_custom(total_w, 5);
    printf("     "); for(int i=0; i<size; i++) { char buf[10]; snprintf(buf, sizeof(buf), "%d", i+1); print_padded(buf, 6, "center", "header"); }
    printf("   |   "); 
    printf("     "); for(int i=0; i<size; i++) { char buf[10]; snprintf(buf, sizeof(buf), "%d", i+1); print_padded(buf, 6, "center", "header"); }
    printf("\033[K\n"); 
    
    pm_board_custom(total_w, 5); char border_top[256] = "    "; for(int i=0; i<size; i++) strcat(border_top, "______"); strcat(border_top, "_");
    print_theme(border_top, "border"); printf("   |   "); print_theme(border_top, "border"); printf("\033[K\n");

    for(int r=0; r<size; r++) {
        pm_board_custom(total_w, 5); print_theme("    |", "border");
        for(int c=0; c<size; c++) {
            bool has_right = (c == size - 1 || room_map1[r][c] != room_map1[r][c + 1]); char lbl[10] = "";
            if (labels1[r][c] != 0) snprintf(lbl, sizeof(lbl), "%d", labels1[r][c]);
            if (lbl[0] != '\0') print_padded(lbl, 5, "ljust", "clue"); else printf("     "); print_theme(has_right ? "|" : " ", "border");
        }
        printf("   |   "); print_theme("    |", "border");
        for(int c=0; c<size; c++) {
            bool has_right = (c == size - 1 || room_map2[r][c] != room_map2[r][c + 1]); char lbl[10] = "";
            if (labels2[r][c] != 0) snprintf(lbl, sizeof(lbl), "%d", labels2[r][c]);
            if (lbl[0] != '\0') print_padded(lbl, 5, "ljust", "clue"); else printf("     "); print_theme(has_right ? "|" : " ", "border");
        } printf("\033[K\n");

        pm_board_custom(total_w, 5); char r2h[10]; snprintf(r2h, sizeof(r2h), " %2d |", r + 1); print_theme(r2h, "header");
        for(int c=0; c<size; c++) {
            int cell = grid1[r][c]; bool has_right = (c == size - 1 || room_map1[r][c] != room_map1[r][c + 1]); char dt[10]; const char* ds;
            if (r == cursor_r1 && c == cursor_c1) { if (cell == 0) strcpy(dt, "."); else snprintf(dt, sizeof(dt), "%d", cell); ds = "cursor"; } 
            else if (conflicts1 != NULL && conflicts1[r][c] && cell != 0) { snprintf(dt, sizeof(dt), "%d", cell); ds = "danger"; }
            else if (cell == 0) { strcpy(dt, "."); ds = "muted"; } else if (locked1[r][c]) { snprintf(dt, sizeof(dt), "[%d]", cell); ds = "locked"; } 
            else { snprintf(dt, sizeof(dt), "%d", cell); ds = "value"; } print_padded(dt, 5, "center", ds); print_theme(has_right ? "|" : " ", "border");
        }
        printf("   |   "); print_theme(r2h, "header");
        for(int c=0; c<size; c++) {
            int cell = grid2[r][c]; bool has_right = (c == size - 1 || room_map2[r][c] != room_map2[r][c + 1]); char dt[10]; const char* ds;
            if (r == cursor_r2 && c == cursor_c2) { if (cell == 0) strcpy(dt, "."); else snprintf(dt, sizeof(dt), "%d", cell); ds = "cursor"; } 
            else if (conflicts2 != NULL && conflicts2[r][c] && cell != 0) { snprintf(dt, sizeof(dt), "%d", cell); ds = "danger"; }
            else if (cell == 0) { strcpy(dt, "."); ds = "muted"; } else if (locked2[r][c]) { snprintf(dt, sizeof(dt), "[%d]", cell); ds = "locked"; } 
            else { snprintf(dt, sizeof(dt), "%d", cell); ds = "value"; } print_padded(dt, 5, "center", ds); print_theme(has_right ? "|" : " ", "border");
        } printf("\033[K\n");

        pm_board_custom(total_w, 5); print_theme("    |", "border");
        for(int c=0; c<size; c++) {
            bool has_bottom = (r == size - 1 || room_map1[r][c] != room_map1[r+1][c]); bool has_right = (c == size - 1 || room_map1[r][c] != room_map1[r][c+1]); bool br_corner = (r < size - 1 && room_map1[r+1][c] != room_map1[r+1][c+1]);
            print_theme(has_bottom ? "_____" : "     ", "border"); print_theme((has_right || br_corner || has_bottom) ? "|" : " ", "border");
        }
        printf("   |   "); print_theme("    |", "border");
        for(int c=0; c<size; c++) {
            bool has_bottom = (r == size - 1 || room_map2[r][c] != room_map2[r+1][c]); bool has_right = (c == size - 1 || room_map2[r][c] != room_map2[r][c+1]); bool br_corner = (r < size - 1 && room_map2[r+1][c] != room_map2[r+1][c+1]);
            print_theme(has_bottom ? "_____" : "     ", "border"); print_theme((has_right || br_corner || has_bottom) ? "|" : " ", "border");
        } printf("\033[K\n");
    }
}

void build_room_map(int size, Room rooms[], int num_rooms, int room_map[MAX_SIZE][MAX_SIZE]) {
    (void)size;
    memset(room_map, 0, MAX_SIZE * MAX_SIZE * sizeof(int));
    for(int i=0; i<num_rooms; i++) for(int j=0; j<rooms[i].num_cells; j++) room_map[rooms[i].cells[j].r][rooms[i].cells[j].c] = rooms[i].id;
}
void build_labels(int size, Room rooms[], int num_rooms, int labels[MAX_SIZE][MAX_SIZE]) {
    (void)size;
    memset(labels, 0, MAX_SIZE * MAX_SIZE * sizeof(int));
    for(int i=0; i<num_rooms; i++) {
        int min_r = rooms[i].cells[0].r, min_c = rooms[i].cells[0].c;
        for(int j=1; j<rooms[i].num_cells; j++) if (rooms[i].cells[j].r < min_r || (rooms[i].cells[j].r == min_r && rooms[i].cells[j].c < min_c)) { min_r = rooms[i].cells[j].r; min_c = rooms[i].cells[j].c; }
        labels[min_r][min_c] = rooms[i].product;
    }
}
void generate_puzzle(int size, Room rooms[], int* num_rooms, int grid[MAX_SIZE][MAX_SIZE], int room_map[MAX_SIZE][MAX_SIZE]) {
    int base[MAX_SIZE]; for(int i=0; i<size; i++) base[i] = i+1;
    for(int r=0; r<size; r++) for(int c=0; c<size; c++) grid[r][c] = base[(r + c) % size];
    for(int i=size-1; i>0; i--) { int j = rand() % (i + 1); for(int c=0; c<size; c++) { int tmp = grid[i][c]; grid[i][c] = grid[j][c]; grid[j][c] = tmp; } }
    for(int i=size-1; i>0; i--) { int j = rand() % (i + 1); for(int r=0; r<size; r++) { int tmp = grid[r][i]; grid[r][i] = grid[r][j]; grid[r][j] = tmp; } }
    bool visited[MAX_SIZE][MAX_SIZE] = {0}; int r_id = 1; *num_rooms = 0;
    for(int r=0; r<size; r++) {
        for(int c=0; c<size; c++) {
            if (visited[r][c]) continue;
            Room nr = {0};
            nr.id = r_id;
            nr.product = 1;
            nr.num_cells = 0;
            nr.cells[nr.num_cells++] = (Point){r, c};
            visited[r][c] = true;
            int tgt = (rand() % (size > 5 ? 5 : size)) + 1;
            for(int step=1; step<tgt; step++) {
                Point nbrs[MAX_SIZE * MAX_SIZE]; int nb_cnt = 0;
                for(int i=0; i<nr.num_cells; i++) {
                    int dr[] = {-1, 1, 0, 0}; int dc[] = {0, 0, -1, 1};
                    for(int d=0; d<4; d++) {
                        int rr = nr.cells[i].r + dr[d], cc = nr.cells[i].c + dc[d];
                        if (rr>=0 && rr<size && cc>=0 && cc<size && !visited[rr][cc]) nbrs[nb_cnt++] = (Point){rr, cc};
                    }
                }
                if (nb_cnt == 0) break;
                Point next = nbrs[rand() % nb_cnt]; nr.cells[nr.num_cells++] = next; visited[next.r][next.c] = true;
            }
            for(int i=0; i<nr.num_cells; i++) { int rr = nr.cells[i].r, cc = nr.cells[i].c; nr.product *= grid[rr][cc]; room_map[rr][cc] = r_id; }
            rooms[(*num_rooms)++] = nr; r_id++;
        }
    }
}
void create_prefilled_board(int size, int solution[MAX_SIZE][MAX_SIZE], int player_grid[MAX_SIZE][MAX_SIZE], bool locked_cells[MAX_SIZE][MAX_SIZE], const char* difficulty) {
    float ratio = (strcmp(difficulty, "easy") == 0) ? 0.60 : (strcmp(difficulty, "median") == 0 ? 0.35 : 0.0);
    int total = size * size; int fill_count = (int)(total * ratio + 0.5);
    if (strcmp(difficulty, "hard") != 0 && strcmp(difficulty, "super_hard") != 0 && strcmp(difficulty, "nightmare") != 0 && fill_count < 1) fill_count = 1;
    Point pos[MAX_SIZE * MAX_SIZE]; int idx = 0;
    for(int r=0; r<size; r++) for(int c=0; c<size; c++) { pos[idx++] = (Point){r, c}; player_grid[r][c] = 0; locked_cells[r][c] = false; }
    for(int i=total-1; i>0; i--) { int j = rand() % (i + 1); Point tmp = pos[i]; pos[i] = pos[j]; pos[j] = tmp; }
    for(int i=0; i<fill_count; i++) { player_grid[pos[i].r][pos[i].c] = solution[pos[i].r][pos[i].c]; locked_cells[pos[i].r][pos[i].c] = true; }
}

bool ghost_take_turn(int size, int grid[MAX_SIZE][MAX_SIZE], int solution[MAX_SIZE][MAX_SIZE], Room rooms[], int num_rooms, int room_map[MAX_SIZE][MAX_SIZE], int ai_difficulty) {
    if (ai_difficulty <= 0) return false;
    Point choices[MAX_SIZE * MAX_SIZE];
    int weights[MAX_SIZE * MAX_SIZE];
    int count = 0;
    for (int r = 0; r < size; r++) for (int c = 0; c < size; c++) {
        if (grid[r][c] != 0) continue;
        int vals[MAX_SIZE];
        int cand_count = get_cell_candidates(size, r, c, grid, rooms, num_rooms, room_map, vals, MAX_SIZE);
        if (cand_count <= 0) cand_count = size;
        choices[count] = (Point){r, c};
        if (ai_difficulty <= 1) weights[count] = 1;
        else if (ai_difficulty == 2) weights[count] = (size + 2) - cand_count;
        else weights[count] = ((size + 2) - cand_count) * 2;
        if (weights[count] < 1) weights[count] = 1;
        count++;
    }
    if (count == 0) return false;

    int pick = 0;
    if (ai_difficulty <= 1) {
        pick = rand() % count;
    } else if (ai_difficulty == 2) {
        int total = 0;
        for (int i = 0; i < count; i++) total += weights[i];
        int roll = rand() % total;
        for (int i = 0; i < count; i++) { roll -= weights[i]; if (roll < 0) { pick = i; break; } }
    } else {
        int best = 0;
        for (int i = 1; i < count; i++) if (weights[i] > weights[best]) best = i;
        pick = best;
        if (ai_difficulty >= 4 && count > 1 && rand() % 100 < 35) {
            int second = rand() % count;
            grid[choices[second].r][choices[second].c] = solution[choices[second].r][choices[second].c];
        }
    }
    grid[choices[pick].r][choices[pick].c] = solution[choices[pick].r][choices[pick].c];
    return true;
}

void load_fixed_puzzle(int choice, int* size, Room** out_rooms, int* num_rooms, int solution[MAX_SIZE][MAX_SIZE]) {
    static Room r3[3] = { {1, 2, 3, {{0,0}, {0,1}, {1,1}}}, {2, 18, 4, {{0,2}, {1,2}, {2,1}, {2,2}}}, {3, 6, 2, {{1,0}, {2,0}}} };
    static Room r5[9] = { {1, 360, 5, {{0,0},{0,1},{1,0},{1,1},{2,1}}}, {2, 40, 3, {{0,2},{1,2},{2,2}}}, {3, 2, 2, {{0,3},{0,4}}}, {4, 5, 1, {{1,3}}}, {5, 180, 5, {{1,4},{2,3},{2,4},{3,4},{4,4}}}, {6, 1, 1, {{2,0}}}, {7, 40, 4, {{3,0},{3,1},{3,2},{4,0}}}, {8, 8, 2, {{3,3},{4,3}}}, {9, 3, 2, {{4,1},{4,2}}} };
    int sol3[3][3] = { {1,2,3}, {3,1,2}, {2,3,1} }; int sol5[5][5] = { {3,4,5,1,2}, {2,3,4,5,1}, {1,5,2,3,4}, {5,2,1,4,3}, {4,1,3,2,5} };
    if (choice == 1) { *size = 3; *num_rooms = 3; *out_rooms = r3; for(int r=0; r<3; r++) for(int c=0; c<3; c++) solution[r][c] = sol3[r][c]; }
    else { *size = 5; *num_rooms = 9; *out_rooms = r5; for(int r=0; r<5; r++) for(int c=0; c<5; c++) solution[r][c] = sol5[r][c]; }
}

/* -------------------------------------------------------------------------- */
/* IMPORT / EXPORT LOGIC                                                      */
/* -------------------------------------------------------------------------- */

static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
void base64_encode(const unsigned char *src, int len, char *out) {
    int i = 0, j = 0;
    for (; i < len - 2; i += 3) {
        out[j++] = b64_table[(src[i] >> 2) & 0x3F]; out[j++] = b64_table[((src[i] & 0x03) << 4) | ((src[i + 1] & 0xF0) >> 4)];
        out[j++] = b64_table[((src[i + 1] & 0x0F) << 2) | ((src[i + 2] & 0xC0) >> 6)]; out[j++] = b64_table[src[i + 2] & 0x3F];
    }
    if (i < len) {
        out[j++] = b64_table[(src[i] >> 2) & 0x3F];
        if (i == len - 1) { out[j++] = b64_table[(src[i] & 0x03) << 4]; out[j++] = '='; } 
        else { out[j++] = b64_table[((src[i] & 0x03) << 4) | ((src[i + 1] & 0xF0) >> 4)]; out[j++] = b64_table[(src[i + 1] & 0x0F) << 2]; }
        out[j++] = '=';
    }
    out[j] = '\0';
}
int base64_decode(const char *src, unsigned char *out) {
    int len = strlen(src); if (len == 0) return 0;
    int i = 0, j = 0, b[4];
    for (i = 0; i < len; i += 4) {
        for (int k = 0; k < 4; k++) {
            if (i + k >= len || src[i + k] == '=') b[k] = 0;
            else { const char *p = strchr(b64_table, src[i + k]); if (!p) return 0; b[k] = p - b64_table; }
        }
        out[j++] = (b[0] << 2) | (b[1] >> 4);
        if (i + 2 < len && src[i + 2] != '=') out[j++] = ((b[1] & 0x0F) << 4) | (b[2] >> 2);
        if (i + 3 < len && src[i + 3] != '=') out[j++] = ((b[2] & 0x03) << 6) | b[3];
    }
    out[j] = '\0'; return 1;
}

bool appendf_safe(char* buf, size_t buf_size, int* len, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int written = vsnprintf(buf + *len, buf_size - *len, fmt, args);
    va_end(args);
    if (written < 0 || *len + written >= (int)buf_size) return false;
    *len += written;
    return true;
}

bool append_char_safe(char* buf, size_t buf_size, int* len, char ch) {
    if (*len + 1 >= (int)buf_size) return false;
    buf[*len] = ch;
    (*len)++;
    buf[*len] = '\0';
    return true;
}

void export_game(int size, int num_rooms, Room rooms[], int solution[MAX_SIZE][MAX_SIZE], int player_grid[MAX_SIZE][MAX_SIZE], bool locked_cells[MAX_SIZE][MAX_SIZE], int hearts, int elapsed, int mp_mode, int ai_diff, int ghost_grid[MAX_SIZE][MAX_SIZE], char* out_b64) {
    char buf[16384] = "";
    int len = 0;
    if (!appendf_safe(buf, sizeof(buf), &len, "%d.%d.%d.%d.%d|", size, hearts, elapsed, mp_mode, ai_diff)) {
        out_b64[0] = '\0';
        return;
    }
    for (int i = 0; i < num_rooms; i++) {
        if (i > 0 && !append_char_safe(buf, sizeof(buf), &len, '_')) { out_b64[0] = '\0'; return; }
        if (!appendf_safe(buf, sizeof(buf), &len, "%d:", rooms[i].product)) { out_b64[0] = '\0'; return; }
        for (int j = 0; j < rooms[i].num_cells; j++) {
            if (!append_char_safe(buf, sizeof(buf), &len, (char)('a' + rooms[i].cells[j].r)) ||
                !append_char_safe(buf, sizeof(buf), &len, (char)('a' + rooms[i].cells[j].c))) {
                out_b64[0] = '\0';
                return;
            }
        }
    }
    if (!append_char_safe(buf, sizeof(buf), &len, '|')) { out_b64[0] = '\0'; return; }
    for(int r=0; r<size; r++) for(int c=0; c<size; c++) if (!append_char_safe(buf, sizeof(buf), &len, (char)('a' + solution[r][c]))) { out_b64[0] = '\0'; return; }
    if (!append_char_safe(buf, sizeof(buf), &len, '|')) { out_b64[0] = '\0'; return; }
    for(int r=0; r<size; r++) for(int c=0; c<size; c++) if (!append_char_safe(buf, sizeof(buf), &len, (char)('a' + player_grid[r][c]))) { out_b64[0] = '\0'; return; }
    if (!append_char_safe(buf, sizeof(buf), &len, '|')) { out_b64[0] = '\0'; return; }
    for(int r=0; r<size; r++) for(int c=0; c<size; c++) if (!append_char_safe(buf, sizeof(buf), &len, locked_cells[r][c] ? '1' : '0')) { out_b64[0] = '\0'; return; }
    
    if (mp_mode > 0 && ghost_grid != NULL) {
        if (!append_char_safe(buf, sizeof(buf), &len, '|')) { out_b64[0] = '\0'; return; }
        for(int r=0; r<size; r++) for(int c=0; c<size; c++) if (!append_char_safe(buf, sizeof(buf), &len, (char)('a' + ghost_grid[r][c]))) { out_b64[0] = '\0'; return; }
    }
    base64_encode((unsigned char*)buf, strlen(buf), out_b64);
}

bool import_game(const char* b64_str, int* size, int* num_rooms, Room rooms[], int solution[MAX_SIZE][MAX_SIZE], int player_grid[MAX_SIZE][MAX_SIZE], bool locked_cells[MAX_SIZE][MAX_SIZE], int* hearts, int* elapsed, int* mp_mode, int* ai_diff, int ghost_grid[MAX_SIZE][MAX_SIZE]) {
    char clean_code[32768] = ""; int c_idx = 0;
    for(int i = 0; b64_str[i] != '\0'; i++) {
        if (!isspace((unsigned char)b64_str[i])) {
            if (c_idx >= (int)sizeof(clean_code) - 1) return false;
            clean_code[c_idx++] = b64_str[i];
        }
    }
    clean_code[c_idx] = '\0'; unsigned char buf[32768];
    if (!base64_decode(clean_code, buf)) return false;
    char* ptr = (char*)buf; int offset = 0;
    
    *mp_mode = 0; *ai_diff = 1;
    int scanned = sscanf(ptr, "%d.%d.%d.%d.%d%n", size, hearts, elapsed, mp_mode, ai_diff, &offset);
    if (scanned == 5 && ptr[offset] == '|') { ptr += offset + 1; }
    else {
        scanned = sscanf(ptr, "%d.%d.%d%n", size, hearts, elapsed, &offset);
        if (scanned == 3 && (ptr[offset] == '|' || ptr[offset] == '-')) { ptr += offset + 1; }
        else return false;
    }
    
    if (*size < 3 || *size > MAX_SIZE) return false;
    *num_rooms = 0;
    while (*ptr && *ptr != '|' && *ptr != '-') {
        if (*num_rooms >= MAX_SIZE * MAX_SIZE) return false;
        rooms[*num_rooms].id = *num_rooms + 1; int roffset = 0;
        if (sscanf(ptr, "%d:%n", &rooms[*num_rooms].product, &roffset) != 1 || roffset == 0) return false;
        ptr += roffset; rooms[*num_rooms].num_cells = 0;
        while (*ptr && *ptr != '_' && *ptr != '|' && *ptr != '-') {
            if (rooms[*num_rooms].num_cells >= MAX_SIZE * MAX_SIZE) return false;
            int r = *ptr - 'a'; ptr++; if(!*ptr) return false; int c = *ptr - 'a'; ptr++;
            if (r < 0 || r >= *size || c < 0 || c >= *size) return false;
            rooms[*num_rooms].cells[rooms[*num_rooms].num_cells].r = r; rooms[*num_rooms].cells[rooms[*num_rooms].num_cells].c = c; rooms[*num_rooms].num_cells++;
        }
        (*num_rooms)++; if (*ptr == '_') ptr++;
    }
    if (*ptr != '|' && *ptr != '-') return false;
    ptr++;
    for(int r=0; r<*size; r++) for(int c=0; c<*size; c++) {
        if (!*ptr) return false;
        int val = *ptr - 'a';
        ptr++;
        if (val < 0 || val > *size) return false;
        solution[r][c] = val;
    }
    if (*ptr != '|' && *ptr != '-') return false;
    ptr++;
    for(int r=0; r<*size; r++) for(int c=0; c<*size; c++) {
        if (!*ptr) return false;
        int val = *ptr - 'a';
        ptr++;
        if (val < 0 || val > *size) return false;
        player_grid[r][c] = val;
    }
    if (*ptr != '|' && *ptr != '-') return false;
    ptr++;
    for(int r=0; r<*size; r++) for(int c=0; c<*size; c++) { if (!*ptr) return false; locked_cells[r][c] = (*ptr == '1'); ptr++; }
    
    if (*mp_mode > 0 && *ptr == '|') {
        ptr++;
        for(int r=0; r<*size; r++) for(int c=0; c<*size; c++) {
            if (!*ptr) return false;
            ghost_grid[r][c] = *ptr - 'a';
            if (ghost_grid[r][c] < 0 || ghost_grid[r][c] > *size) return false;
            ptr++;
        }
    }
    return true;
}

/* -------------------------------------------------------------------------- */
/* PRINT MENUS & INFO DISPLAYS                                                */
/* -------------------------------------------------------------------------- */

int run_menu_centered(void (*header_func)(), const char* title, const char** options, int num_options) {
    int selected = 0; 
    bool remember_selection = (title == NULL && num_options == 6 && strcmp(options[0], "Single Player") == 0);
    if (remember_selection) selected = main_menu_selected;
    int max_opt_len = 0;
    for(int i=0; i<num_options; i++) {
        const char* opt = localize_ui_text(options[i]);
        int l = utf8_display_width(opt);
        if (l > max_opt_len) max_opt_len = l;
    }
    int block_w = max_opt_len + 20;

    MENU_LOOP_BEGIN(tick)
        set_frame_row_estimate((title ? 3 : 1) + num_options + 8);
        if (header_func) header_func();
        else {
            int v_pad = (term_h - (num_options + 4)) / 2;
            if (v_pad > 0) for(int i=0; i<v_pad; i++) printf("\n");
        }
        if (title) { pm_custom(utf8_display_width(localize_ui_text(title))); print_theme(title, "title"); printf("\n\n"); }
        for(int i=0; i<num_options; i++) {
            pm_custom(block_w);
            char buf[256];
            const char* opt = localize_ui_text(options[i]);
            if (i == selected) { 
                snprintf(buf, sizeof(buf), ">>>>>   %s   <<<<<", opt);
                print_padded(buf, block_w, "center", "cursor"); 
            } 
            else { 
                snprintf(buf, sizeof(buf), "%s", opt);
                print_padded(buf, block_w, "center", "info"); 
            }
            printf("\n");
        }
    MENU_LOOP_END(tick)
        if (c == KEY_UP || c == 'w' || c == 'W') selected = (selected - 1 + num_options) % num_options;
        else if (c == KEY_DOWN || c == 's' || c == 'S') selected = (selected + 1) % num_options;
        else if (c >= '1' && c <= '0' + (num_options < 9 ? num_options : 9)) { selected = c - '1'; break; }
        else if (c == KEY_ENTER) break;
        else if (c == 27 || c == 'q' || c == 'Q') { selected = -1; break; }
    }
    if (remember_selection && selected >= 0) main_menu_selected = selected;
    return selected;
}

void draw_help_demo(int tick) {
    static int room_map[MAX_SIZE][MAX_SIZE] = {
        {1, 1, 2, 0, 0, 0, 0, 0, 0},
        {3, 1, 2, 0, 0, 0, 0, 0, 0},
        {3, 2, 2, 0, 0, 0, 0, 0, 0}
    };
    static int labels[MAX_SIZE][MAX_SIZE] = {
        {2, 0, 18, 0, 0, 0, 0, 0, 0},
        {6, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0}
    };
    static int stage0[MAX_SIZE][MAX_SIZE] = {
        {0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0}
    };
    static int stage1[MAX_SIZE][MAX_SIZE] = {
        {1, 2, 0, 0, 0, 0, 0, 0, 0},
        {0, 1, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0}
    };
    static int stage2[MAX_SIZE][MAX_SIZE] = {
        {1, 2, 2, 0, 0, 0, 0, 0, 0},
        {0, 1, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0}
    };
    static int stage3[MAX_SIZE][MAX_SIZE] = {
        {1, 2, 3, 0, 0, 0, 0, 0, 0},
        {3, 1, 2, 0, 0, 0, 0, 0, 0},
        {0, 3, 1, 0, 0, 0, 0, 0, 0}
    };
    static int stage4[MAX_SIZE][MAX_SIZE] = {
        {1, 2, 3, 0, 0, 0, 0, 0, 0},
        {3, 1, 2, 0, 0, 0, 0, 0, 0},
        {2, 3, 0, 0, 0, 0, 0, 0, 0}
    };
    static int stage5[MAX_SIZE][MAX_SIZE] = {
        {1, 2, 3, 0, 0, 0, 0, 0, 0},
        {3, 1, 2, 0, 0, 0, 0, 0, 0},
        {2, 3, 1, 0, 0, 0, 0, 0, 0}
    };
    int phase = (tick / 8) % 6;
    int subframe = tick % 8;
    int cursor_r = -1, cursor_c = -1;
    int player_grid[MAX_SIZE][MAX_SIZE] = {0};
    bool locked[MAX_SIZE][MAX_SIZE] = {0};
    bool conflicts[MAX_SIZE][MAX_SIZE] = {0};
    Room* demo_rooms = NULL;
    int solution[MAX_SIZE][MAX_SIZE] = {0};
    int demo_size = 0;
    int demo_room_count = 0;
    const char* header = "";
    const char* subtitle = "";
    const char* why_line = "";
    const char* move_line = "";
    const char* border_a = (tick / 2) % 2 == 0 ? "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-" : "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=";
    const char* border_b = (tick / 2) % 2 == 0 ? "~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*" : "*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~";
    const char* transition_bar[8] = {
        "[>.......]",
        "[=>......]",
        "[==>.....]",
        "[===>....]",
        "[====>...]",
        "[=====>..]",
        "[======>.]",
        "[=======>]"
    };

    load_fixed_puzzle(1, &demo_size, &demo_rooms, &demo_room_count, solution);
    if (phase == 0) {
        memcpy(player_grid, stage0, sizeof(stage0));
        header = tr4("Step 1: Read the room clues before typing anything.", "步驟1：先讀房間提示再輸入。", "步骤1：先读房间提示再输入。", "手順1: まず部屋ヒントを読む。");
        subtitle = tr4("In a 3x3 board, every row and every column must contain 1, 2, and 3 exactly once.", "3x3棋盤中，每行每列都必須剛好有1、2、3。", "3x3棋盘中，每行每列都必须刚好有1、2、3。", "3x3では各行各列に1,2,3を一度ずつ。");
        why_line = tr4("Start by scanning the clue corners. They tell you which cells are tied together as one room.", "先看角落提示，可判斷哪些格子屬於同一房間。", "先看角落提示，可判断哪些格子属于同一房间。", "角ヒントから同じ部屋のマスを把握しよう。");
        move_line = tr4("Focus move: identify the three rooms and the numbers each row still needs.", "重點：先辨識三個房間與每行缺的數字。", "重点：先辨识三个房间与每行缺的数字。", "重点: 3つの部屋と各行の不足数字を確認。");
        cursor_r = 0; cursor_c = 0;
    } else if (phase == 1) {
        memcpy(player_grid, stage1, sizeof(stage1));
        header = tr4("Step 2: Use row + column information to force a number.", "步驟2：用行列資訊推唯一解。", "步骤2：用行列信息推唯一解。", "手順2: 行と列の情報で確定させる。");
        subtitle = tr4("Row 1 already has 1 and 2, so the last cell in that row must be 3.", "第1行已有1和2，最後一格必為3。", "第1行已有1和2，最后一格必为3。", "1行目は1と2があるので残りは3。");
        why_line = tr4("A forced move is the safest move. If only one digit fits both row and column, place it.", "唯一可行的數字最安全，能放就放。", "唯一可行的数字最安全，能放就放。", "確定手が最も安全。行列両方に合う数字を置く。");
        move_line = tr4("Focus move: lock row 1 col 3 with a 3 because the row has only one missing digit.", "重點：第1行第3列填3。", "重点：第1行第3列填3。", "重点: 1行3列を3で確定。");
        cursor_r = 0; cursor_c = 2;
    } else if (phase == 2) {
        memcpy(player_grid, stage2, sizeof(stage2));
        header = tr4("Wrong move example: putting 2 at row 1 col 3 loses immediately.", "錯誤示例：第1行第3列放2會立刻衝突。", "错误示例：第1行第3列放2会立刻冲突。", "誤り例: 1行3列に2を置くと即矛盾。");
        subtitle = tr4("You now have two 2s in row 1, and the clue-18 room cannot multiply correctly. Red cells mean conflict.", "第1行出現兩個2，且18房間乘積錯誤；紅格即衝突。", "第1行出现两个2，且18房间乘积错误；红格即冲突。", "1行目に2が重複し、18部屋の積も不一致。赤は競合。");
        why_line = tr4("Bad placements break two systems at once: uniqueness and room product. That is why conflicts matter.", "錯誤會同時破壞唯一性與房間乘積。", "错误会同时破坏唯一性与房间乘积。", "誤配置は一意性と積条件を同時に壊す。");
        move_line = tr4("Focus move: notice the failure quickly, then undo before you build more mistakes on top of it.", "重點：先快速發現，再立刻撤銷。", "重点：先快速发现，再立刻撤销。", "重点: 早く気づいてすぐ戻す。");
        cursor_r = 0; cursor_c = 2;
    } else if (phase == 3) {
        memcpy(player_grid, stage3, sizeof(stage3));
        header = tr4("Step 3: Each finished room must match its product clue.", "步驟3：完成的房間要符合乘積提示。", "步骤3：完成的房间要符合乘积提示。", "手順3: 完成部屋は積ヒント一致が必須。");
        subtitle = tr4("The clue 18 room now contains 3, 2, 3, 1. That multiplies to 18, so this region is valid.", "18房間為3,2,3,1，乘積=18，合法。", "18房间为3,2,3,1，乘积=18，合法。", "18部屋は3,2,3,1で積18。正しい。");
        why_line = tr4("Room clues are not decoration. They are the second half of the puzzle logic after rows and columns.", "房間提示是核心規則，不是裝飾。", "房间提示是核心规则，不是装饰。", "部屋ヒントは装飾ではなく主要ルール。");
        move_line = tr4("Focus move: validate the full room after placing a number, not just the row that changed.", "重點：放數後要檢查整個房間。", "重点：放数后要检查整个房间。", "重点: 数字を置いたら部屋全体を確認。");
        cursor_r = 2; cursor_c = 0;
    } else if (phase == 4) {
        memcpy(player_grid, stage4, sizeof(stage4));
        header = tr4("Step 4: When only one value fits a row and a column, place it with confidence.", "步驟4：行列同時唯一時可直接填入。", "步骤4：行列同时唯一时可直接填入。", "手順4: 行列で一意なら自信を持って置く。");
        subtitle = tr4("Bottom row already has 2 and 3, so the last cell must be 1. Column 3 also agrees.", "底行已有2與3，最後必為1；第3列也一致。", "底行已有2与3，最后必为1；第3列也一致。", "最下段は2と3があり残りは1。3列目も一致。");
        why_line = tr4("When row logic and column logic point to the same digit, you have a strong confirmed move.", "行列同時指向同一數字時最可靠。", "行列同时指向同一数字时最可靠。", "行と列が同じ数字を示す時が最も強い。");
        move_line = tr4("Focus move: use double confirmation. A digit is strongest when row and column logic agree.", "重點：使用雙重確認再落子。", "重点：使用双重确认再落子。", "重点: 二重確認で確定させる。");
        cursor_r = 2; cursor_c = 2;
    } else {
        memcpy(player_grid, stage5, sizeof(stage5));
        header = tr4("Step 5: This is a full win state.", "步驟5：這是完整通關狀態。", "步骤5：这是完整通关状态。", "手順5: これで完全クリア。");
        subtitle = tr4("You win only when the board is full, every row and column is unique, and every room product matches.", "滿盤、行列唯一、房間乘積皆正確才算勝利。", "满盘、行列唯一、房间乘积皆正确才算胜利。", "盤面完成・行列一意・部屋積一致で勝利。");
        why_line = tr4("A solved board satisfies all three checks together: full board, unique lines, correct products.", "解題需同時滿足三條件。", "解题需同时满足三条件。", "解答は3条件を同時に満たす必要がある。");
        move_line = tr4("Focus move: before you celebrate, do one final scan for duplicate digits and broken room products.", "重點：慶祝前再掃一次重複與乘積。", "重点：庆祝前再扫一次重复与乘积。", "重点: 祝う前に重複と積を最終確認。");
        cursor_r = 2; cursor_c = 2;
    }

    compute_conflicts(3, player_grid, demo_rooms, demo_room_count, room_map, conflicts);
    ui_anim_tick = tick;

    pm_custom(92); print_theme(tr4("Animated Tutorial: watch the 3x3 room solve itself\n", "動態教學：觀看 3x3 自動解題\n", "动态教学：观看 3x3 自动解题\n", "アニメチュートリアル: 3x3の解き方\n"), "title");
    pm_custom(88); print_theme(border_a, "border"); printf("\n");
    pm_custom(88); print_padded(header, 88, "center", phase == 2 ? "warning" : "accent"); printf("\n");
    pm_custom(88); print_padded(subtitle, 88, "center", "info"); printf("\n");
    pm_custom(88); print_theme(border_b, phase == 2 ? "warning" : "border"); printf("\n\n");
    pm_custom(92); print_theme(tr4("Step Track  ", "步驟追蹤  ", "步骤追踪  ", "ステップ進行  "), "header");
    for (int i = 0; i < 6; i++) {
        char chip[24];
        if (i == phase) {
            snprintf(chip, sizeof(chip), "%c%d%c", subframe % 2 == 0 ? '{' : '<', i + 1, subframe % 2 == 0 ? '}' : '>');
            print_theme(chip, "cursor");
        } else if (i < phase) {
            snprintf(chip, sizeof(chip), "[%d]", i + 1);
            print_theme(chip, "success");
        } else {
            snprintf(chip, sizeof(chip), "(%d)", i + 1);
            print_theme(chip, "muted");
        }
        printf(" ");
    }
    printf(" ");
    print_theme(transition_bar[subframe], phase == 2 ? "warning" : "clue");
    printf("\n");
    pm_custom(92); print_theme(tr4("Transition cue: ", "轉場提示: ", "转场提示: ", "遷移ヒント: "), "header"); print_theme(subframe < 3 ? tr4("thinking...", "思考中...", "思考中...", "思考中...") : (subframe < 6 ? tr4("placing...", "填入中...", "填入中...", "配置中...") : tr4("checking...", "檢查中...", "检查中...", "確認中...")), phase == 2 ? "warning" : "info"); printf("\n\n");
    draw_grid(3, room_map, labels, player_grid, locked, conflicts, cursor_r, cursor_c);
    printf("\n");
    pm_custom(92); print_theme(tr4("Tutorial Notes\n", "教學重點\n", "教学重点\n", "チュートリアル要点\n"), "header");
    pm_custom(92); print_theme(tr4("  clue corner        -> top-left number of a room\n", "  角落提示            -> 房間左上提示數字\n", "  角落提示            -> 房间左上提示数字\n", "  角ヒント            -> 部屋左上の数字\n"), "info");
    pm_custom(92); print_theme(tr4("  highlighted cell   -> the move this step is teaching\n", "  高亮格子            -> 此步驟示範落子\n", "  高亮格子            -> 此步骤示范落子\n", "  ハイライトセル      -> この手順の着手\n"), "info");
    pm_custom(92); print_theme(tr4("  red cell           -> conflict caused by a bad placement\n", "  紅色格子            -> 錯誤落子造成衝突\n", "  红色格子            -> 错误落子造成冲突\n", "  赤セル              -> 誤配置による競合\n"), "warning");
    pm_custom(92); print_theme(tr4("  focus move         -> ", "  重點動作            -> ", "  重点动作            -> ", "  重点手              -> "), "clue"); print_theme(move_line, "info"); printf("\n");
    pm_custom(92); print_theme(tr4("  why this matters   -> ", "  為何重要            -> ", "  为何重要            -> ", "  なぜ重要か          -> "), "clue"); print_theme(why_line, "info"); printf("\n");
}

void print_help_screen() {
    MENU_LOOP_BEGIN(tick)
        int help_w = term_w > 4 ? term_w - 4 : term_w;
        bool compact = (term_w < 112 || term_h < 42);
        int help_panel_w = help_w > 96 ? 96 : help_w;
        set_frame_row_estimate(compact ? 18 : 34);

        if (!compact) {
            draw_help_demo(tick);
            printf("\n");
            pm_custom(help_panel_w); print_padded(tr4("=== Controls ===", "=== 操作 ===", "=== 操作 ===", "=== 操作 ==="), help_panel_w, "center", "accent"); printf("\n");
            pm_custom(help_panel_w); print_padded(tr4("Move       : WASD / Arrows", "移動       : WASD / 方向鍵", "移动       : WASD / 方向键", "移動       : WASD / 矢印"), help_panel_w, "ljust", "info"); printf("\n");
            pm_custom(help_panel_w); print_padded(tr4("Fill/Clear : 1-9 fill, 0/Backspace clear", "填入/清除  : 1-9填入，0/Backspace清除", "填入/清除  : 1-9填入，0/Backspace清除", "入力/消去  : 1-9入力、0/Backspace消去"), help_panel_w, "ljust", "info"); printf("\n");
            pm_custom(help_panel_w); print_padded(tr4("Quick keys : H hint, C check, O options/export, Q quit", "快捷鍵     : H提示、C檢查、O選項/匯出、Q離開", "快捷键     : H提示、C检查、O选项/导出、Q退出", "キー       : Hヒント、C確認、O設定/書出、Q終了"), help_panel_w, "ljust", "info"); printf("\n");
            pm_custom(help_panel_w); print_padded(tr4("Old mode   : row col val | hint | check | clear | option | exit", "舊模式     : row col val | hint | check | clear | option | exit", "旧模式     : row col val | hint | check | clear | option | exit", "旧モード   : row col val | hint | check | clear | option | exit"), help_panel_w, "ljust", "info"); printf("\n");
            pm_custom(help_panel_w); print_padded(tr4("Music      : N next, P previous, +/- volume", "音樂       : N下一首、P上一首、+/-音量", "音乐       : N下一首、P上一首、+/-音量", "音楽       : N次、P前、+/-音量"), help_panel_w, "ljust", "info"); printf("\n");
            printf("\n");
            pm_custom(help_panel_w); print_padded(tr4("=== Tips for New Players ===", "=== 新手提示 ===", "=== 新手提示 ===", "=== 初心者ヒント ==="), help_panel_w, "center", "header"); printf("\n");
            pm_custom(help_panel_w); print_padded(tr4("1) Finish forced cells first, then re-check room products.", "1) 先填唯一解，再回查房間乘積。", "1) 先填唯一解，再回查房间乘积。", "1) 確定マスから埋め、次に部屋の積を確認。"), help_panel_w, "ljust", "info"); printf("\n");
            pm_custom(help_panel_w); print_padded(tr4("2) If room product looks impossible, check row/column duplicates.", "2) 若房間乘積不可能，先檢查行列重複。", "2) 若房间乘积不可能，先检查行列重复。", "2) 積が不可能なら行列重複を先に確認。"), help_panel_w, "ljust", "info"); printf("\n");
            pm_custom(help_panel_w); print_padded(tr4("3) Watch the step tracker: thinking -> placing -> checking.", "3) 觀看步驟動畫: 思考 -> 填入 -> 檢查。", "3) 观看步骤动画: 思考 -> 填入 -> 检查。", "3) ステップ表示: 思考 -> 配置 -> 確認。"), help_panel_w, "ljust", "info"); printf("\n");
            pm_custom(help_panel_w); print_padded(tr4("4) Tutorial auto-loops every 1.5s for repeated review.", "4) 教學每1.5秒自動循環，可反覆觀察。", "4) 教学每1.5秒自动循环，可反复观察。", "4) チュートリアルは1.5秒ごとに自動ループ。"), help_panel_w, "ljust", "info"); printf("\n");
        } else {
            pm_custom(help_w); print_theme(tr4("Help (compact view)\n", "說明（精簡模式）\n", "帮助（精简模式）\n", "ヘルプ（簡易表示）\n"), "title");
            pm_custom(help_w); print_theme(tr4("Resize terminal for animated tutorial.\n\n", "放大終端可查看動態教學。\n\n", "放大终端可查看动态教学。\n\n", "ターミナルを広げるとアニメ解説を表示。\n\n"), "muted");
            pm_custom(help_panel_w); print_padded(tr4("=== Controls ===", "=== 操作 ===", "=== 操作 ===", "=== 操作 ==="), help_panel_w, "center", "accent"); printf("\n");
            pm_custom(help_panel_w); print_padded(tr4("Move       : WASD / Arrows", "移動       : WASD / 方向鍵", "移动       : WASD / 方向键", "移動       : WASD / 矢印"), help_panel_w, "ljust", "info"); printf("\n");
            pm_custom(help_panel_w); print_padded(tr4("Fill/Clear : 1-9 / 0-Backspace", "填入/清除  : 1-9 / 0-Backspace", "填入/清除  : 1-9 / 0-Backspace", "入力/消去  : 1-9 / 0-Backspace"), help_panel_w, "ljust", "info"); printf("\n");
            pm_custom(help_panel_w); print_padded(tr4("Actions    : H hint, C check, O option, Q quit", "功能       : H提示、C檢查、O選項、Q離開", "功能       : H提示、C检查、O选项、Q退出", "機能       : Hヒント、C確認、O設定、Q終了"), help_panel_w, "ljust", "info"); printf("\n");
            pm_custom(help_panel_w); print_padded(tr4("=== Tips ===", "=== 提示 ===", "=== 提示 ===", "=== ヒント ==="), help_panel_w, "center", "header"); printf("\n");
            pm_custom(help_panel_w); print_padded(tr4("Start with forced cells; re-check products each step.", "先填唯一解，每步都回查房間乘積。", "先填唯一解，每步都回查房间乘积。", "確定マス優先、各手で積を再確認。"), help_panel_w, "ljust", "info"); printf("\n");
        }
        printf("\n");
        pm_custom(help_w); print_padded("Press Enter to return.", help_w, "center", "clue");
    MENU_LOOP_END(tick)
        if (c == KEY_ENTER) break;
    }
}

void celebrate_win(int size, double elapsed_seconds, int score, int unlocked, int hints_used, int hearts_left, const char* mode, const char* difficulty) {
    (void)size;
    char t_str[32];
    char score_msg[128];
    char meta_msg[128];
    char reward_msg[128];
    format_duration((int)elapsed_seconds, t_str);
    snprintf(score_msg, sizeof(score_msg), "Time %s | Score %d", t_str, score);
    snprintf(meta_msg, sizeof(meta_msg), "Mode %s | Diff %s | Hearts %d | Hints %d", mode, difficulty, hearts_left, hints_used);
    if (unlocked) snprintf(reward_msg, sizeof(reward_msg), "Unlocked %d new achievement%s", unlocked, unlocked == 1 ? "" : "s");
    else snprintf(reward_msg, sizeof(reward_msg), "No new achievements this round");

    play_fullscreen_result_animation(true,
        "PLAYER VICTORY",
        "The room is complete and the logic holds.",
        score_msg,
        meta_msg);

    {
        char time_value[64];
        char score_value[64];
        char mode_value[64];
        char diff_value[64];
        char hearts_value[64];
        char hints_value[64];
        const char* keys[] = {"Result", "Time", "Score", "Mode", "Diff", "Hearts", "Hints", "Rewards"};
        const char* values[] = {"You solved the board cleanly.", time_value, score_value, mode_value, diff_value, hearts_value, hints_value, reward_msg};
        snprintf(time_value, sizeof(time_value), "%s", t_str);
        snprintf(score_value, sizeof(score_value), "%d", score);
        snprintf(mode_value, sizeof(mode_value), "%s", mode);
        snprintf(diff_value, sizeof(diff_value), "%s", difficulty);
        snprintf(hearts_value, sizeof(hearts_value), "%d", hearts_left);
        snprintf(hints_value, sizeof(hints_value), "%d", hints_used);
        printf("\033[H\033[2J");
        draw_summary_table("RESULT SUMMARY", "Single player result", keys, values, 8, 82);
        printf("\n");
    }
    if (unlocked) {
        int masks[] = {ACH_FIRST_WIN, ACH_NO_HINT, ACH_SPEED, ACH_HARD, ACH_BOSS, ACH_IRON_HEART, ACH_MEGA_GRID, ACH_SCORE_MASTER, ACH_NIGHTMARE_SPEED, ACH_STEADY_HAND, ACH_HINT_COMMANDER, ACH_COMEBACK_SPIRIT};
        pm_custom(96); print_padded("New achievements unlocked:", 96, "center", "clue"); printf("\n");
        for (int i = 0; i < 12; i++) {
            if (unlocked & masks[i]) {
                char amsg[256];
                snprintf(amsg, sizeof(amsg), "- %s", get_ach_text(masks[i]));
                pm_custom(96); print_padded(amsg, 96, "center", "info"); printf("\n");
            }
        }
    }
}

/* -------------------------------------------------------------------------- */
/* OPTION & MUSIC GUI INTERFACES                                              */
/* -------------------------------------------------------------------------- */

void music_player_menu() {
    int selected = current_track;
    MENU_LOOP_BEGIN(tick)
        pm_custom(46); print_theme("==============================================\n", "border");
        pm_custom(46); print_theme("                 MUSIC PLAYER                 \n", "title");
        pm_custom(46); print_theme("==============================================\n\n", "border");
        
        char vol_buf[64]; snprintf(vol_buf, sizeof(vol_buf), "Volume: %d%% (boosted)", music_volume);
        pm_custom(46); print_padded(vol_buf, 46, "center", "warning"); printf("\n\n");
        pm_custom(60); print_padded(audio_status, 60, "center", "muted"); printf("\n\n");
        
        int start_idx = selected - 10; if (start_idx < 0) start_idx = 0;
        int end_idx = start_idx + 20; if (end_idx > num_tracks) end_idx = num_tracks;
        int block_w = 60;
        for(int i = start_idx; i < end_idx; i++) {
            pm_custom(block_w);
            char buf[256];
            if (i == selected) {
                snprintf(buf, sizeof(buf), ">>>>>   %s   <<<<<", get_clean_track_display_name(i));
                if (i == current_track) print_padded(buf, block_w, "center", "success");
                else print_padded(buf, block_w, "center", "cursor");
            } else {
                snprintf(buf, sizeof(buf), "%s", get_clean_track_display_name(i));
                if (i == current_track) print_padded(buf, block_w, "center", "success");
                else print_padded(buf, block_w, "center", "info");
            }
            printf("\n");
        }
        printf("\n"); pm_custom(80); print_padded("[W/S] Navigate |[Enter] Play Selected | [-/+] Volume | [Q] Back", 80, "center", "muted"); printf("\n");
    MENU_LOOP_END(tick)
        if (c == KEY_UP || c == 'w' || c == 'W') selected = (selected - 1 + num_tracks) % num_tracks;
        else if (c == KEY_DOWN || c == 's' || c == 'S') selected = (selected + 1) % num_tracks;
        else if (c == '-' || c == '_') { music_volume = music_volume >= 10 ? music_volume - 10 : 0; set_audio_volume(); save_stats(); }
        else if (c == '+' || c == '=') { music_volume = music_volume <= 190 ? music_volume + 10 : 200; set_audio_volume(); save_stats(); }
        else if (c == KEY_ENTER) { current_track = selected; play_current_track(); save_stats(); } 
        else if (c == 27 || c == 'q' || c == 'Q') break;
    }
}

int in_game_option_menu(int allow_export, char* b64_out) {
    int num_opts = allow_export ? 6 : 5;
    int selected = 0;
    MENU_LOOP_BEGIN(tick)
        pm_custom(46); print_theme("==============================================\n", "border");
        pm_custom(46); print_theme("                 OPTIONS MENU                 \n", "title");
        pm_custom(46); print_theme("==============================================\n\n", "border");
        
        char opt_texts[6][256];
        if (allow_export) {
            strcpy(opt_texts[0], "Export Save Code");
            snprintf(opt_texts[1], sizeof(opt_texts[1]), "Input Mode: %s", control_mode == 1 ? "WASD/Arrows" : "String Command");
            snprintf(opt_texts[2], sizeof(opt_texts[2]), "Theme: %s", current_theme == 0 ? "Cyberpunk" : (current_theme == 1 ? "Fantasy" : (current_theme == 2 ? "Horror" : (current_theme == 3 ? "Steampunk" : "Biopunk"))));
            snprintf(opt_texts[3], sizeof(opt_texts[3]), "Music: %s", get_clean_track_display_name(current_track));
            strcpy(opt_texts[4], "Back to Game");
            strcpy(opt_texts[5], "Quit Game");
        } else {
            snprintf(opt_texts[0], sizeof(opt_texts[0]), "Input Mode: %s", control_mode == 1 ? "WASD/Arrows" : "String Command");
            snprintf(opt_texts[1], sizeof(opt_texts[1]), "Theme: %s", current_theme == 0 ? "Cyberpunk" : (current_theme == 1 ? "Fantasy" : (current_theme == 2 ? "Horror" : (current_theme == 3 ? "Steampunk" : "Biopunk"))));
            snprintf(opt_texts[2], sizeof(opt_texts[2]), "Music: %s", get_clean_track_display_name(current_track));
            strcpy(opt_texts[3], "Back to Game");
            strcpy(opt_texts[4], "Quit Game");
        }

        int block_w = 60;
        for(int i=0; i<num_opts; i++) {
            pm_custom(block_w);
            char buf[512];
            if (i == selected) { 
                snprintf(buf, sizeof(buf), ">>>>>   %s   <<<<<", opt_texts[i]);
                print_padded(buf, block_w, "center", "cursor"); 
            }
            else { 
                snprintf(buf, sizeof(buf), "%s", opt_texts[i]);
                print_padded(buf, block_w, "center", "info"); 
            }
            printf("\n");
        }
    MENU_LOOP_END(tick)
        
        if (c == KEY_UP || c == 'w' || c == 'W') selected = (selected - 1 + num_opts) % num_opts;
        else if (c == KEY_DOWN || c == 's' || c == 'S') selected = (selected + 1) % num_opts;
        else if (c >= '1' && c <= '0' + num_opts) { selected = c - '1'; c = KEY_ENTER; }
        
        if (c == KEY_ENTER) {
            int action = allow_export ? selected : selected + 1;
            if (allow_export && action == 0) {
                printf("\033[H\033[2J"); pm_custom(30); printf("\033[1;32m--- GAME SAVED TO SECURE STRING ---\033[0m\n\n");
                pm_custom(50); printf("Copy this short code to 'Import' and play later:\n\n");
                pm_custom(strlen(b64_out)); printf("%s\n\n", b64_out);
                pm_custom(30); printf("Press Enter to return...");
                while(read_key() != KEY_ENTER && !sigint_flag) {}
                if (sigint_flag) { restore_terminal(); exit(0); }
            } else if (action == 1) { control_mode = 1 - control_mode; save_stats(); } 
            else if (action == 2) { current_theme = (current_theme + 1) % 5; save_stats(); } 
            else if (action == 3) { music_player_menu(); } 
            else if (action == 4) { return 0; } 
            else if (action == 5) { return 1; }
        } else if (c == 27 || c == 'q' || c == 'Q') { return 0; }
    }
    return 0;
}

/* -------------------------------------------------------------------------- */
/* SINGLE PLAYER GAMEPLAY LOOP                                                */
/* -------------------------------------------------------------------------- */

void game_loop(int size, Room rooms[], int num_rooms, int solution[MAX_SIZE][MAX_SIZE], int room_map[MAX_SIZE][MAX_SIZE], int labels[MAX_SIZE][MAX_SIZE], int player_grid[MAX_SIZE][MAX_SIZE], bool locked_cells[MAX_SIZE][MAX_SIZE], const char* mode, const char* diff, int hearts, int initial_elapsed) {
    time_t start_time = time(NULL); unsigned long long last_draw_ms = 0; bool force_redraw = true; int hints_used = 0;
    char sys_msg[128] = ""; char input_buf[128] = ""; int input_len = 0; int cursor_r = 0, cursor_c = 0;
    int cat_tick = 0;
    int correct_streak = 0;
    int idle_stage = 0;
    time_t last_action_time = time(NULL);
    bool conflicts[MAX_SIZE][MAX_SIZE] = {0};

    while (1) {
        unsigned long long now = now_ms();
        time_t current_time = time(NULL); int elapsed = (int)difftime(current_time, start_time) + initial_elapsed;
        if (current_time - last_action_time >= 10 && idle_stage == 0) {
            set_pet_message(correct_streak, 1, tr4("Stuck? Re-check the clue corner and one room at a time.", "卡住了？先重看角落提示，再逐個房間推。", "卡住了？先重看角落提示，再逐个房间推。", "詰まった？左上のヒントから順に見直そう。"));
            idle_stage = 1;
            force_redraw = true;
        } else if (current_time - last_action_time >= 20 && idle_stage == 1) {
            set_pet_message(correct_streak, 2, tr4("Try the smallest possible room product first.", "先從乘積最小的房間開始。", "先从乘积最小的房间开始。", "まず積が小さい部屋から試そう。"));
            idle_stage = 2;
            force_redraw = true;
        }
        if (check_win(size, player_grid, rooms, num_rooms)) {
            int score = compute_score(size, elapsed, hints_used, hearts, mode, diff);
            total_wins++; if (score > best_score) best_score = score;
            int unlocked = update_achievements(size, elapsed, hints_used, hearts, score, mode, diff); save_stats();
            play_victory_track();
            printf("\033[H\033[2J"); celebrate_win(size, elapsed, score, unlocked, hints_used, hearts, mode, diff);
            pm_custom(35); printf("Press Enter to return to menu...");
            while(read_key() != KEY_ENTER && !sigint_flag) {}
            restore_background_music();
            if(sigint_flag) { restore_terminal(); exit(0); }
            return;
        }

        if ((now - last_draw_ms >= 33ULL) || force_redraw) {
            update_term_size();
            int board_rows_est = (size * 2) + 6;
            int grid_width_est = 5 + 6 * size;
            bool too_small = (term_w < grid_width_est + 2) || (term_h < board_rows_est + 2);
            int compact_level = 0;
            if (!too_small) {
                if (term_h < board_rows_est + 12) compact_level = 1;
                if (term_h < board_rows_est + 7) compact_level = 2;
            } else {
                compact_level = 2;
            }
            set_frame_row_estimate(too_small ? 9 : (board_rows_est + (compact_level == 0 ? 12 : (compact_level == 1 ? 8 : 5))));

            begin_soft_frame();
            pm_custom(22); print_theme("--- Inshi-no-heya ---\n", "title");
            char t_str[32]; format_duration(elapsed, t_str);
            char h_str_used[16]; snprintf(h_str_used, sizeof(h_str_used), "%d", hints_used);

            if (compact_level == 0) {
                pm_custom(15); print_theme("Time:", "header"); printf(" "); print_theme(t_str, "success"); printf("\n");
                pm_custom(15); print_theme("Hints used:", "header"); printf(" "); print_theme(h_str_used, "warning"); printf("\n");
                if (hearts >= 0) {
                    char heart_disp[32] = "";
                    for(int i=0; i<hearts; i++) strcat(heart_disp, "<3 ");
                    pm_custom(15); print_theme("Hearts:", "danger"); printf(" \033[1;31m%s\033[0m\n", heart_disp);
                }
                printf("\n");
                if (control_mode == 1) { pm_custom(78); print_theme("Commands: WASD/Arrows | 1-9 fill | 0 clear | [H]int | [C]heck | [O]ption | [Q]uit\n\n", "info"); }
                else { pm_custom(78); print_theme("Commands: 'row col val' | 'hint' | 'check' | 'clear' | 'idk' | 'option' | 'exit'\n\n", "info"); }
            } else if (compact_level == 1) {
                char compact_line[160];
                if (hearts >= 0) snprintf(compact_line, sizeof(compact_line), "Time %s | Hints %s | Hearts %d", t_str, h_str_used, hearts);
                else snprintf(compact_line, sizeof(compact_line), "Time %s | Hints %s", t_str, h_str_used);
                pm_custom(term_w - 4); print_padded(compact_line, term_w - 4, "center", "info"); printf("\n");
                if (control_mode == 1) { pm_custom(term_w - 4); print_padded("[WASD/Arrows] Move  [1-9] Fill  [0] Clear  [H][C][O][Q]", term_w - 4, "center", "muted"); printf("\n\n"); }
                else { pm_custom(term_w - 4); print_padded("Commands: row col val | hint | check | clear | option | exit", term_w - 4, "center", "muted"); printf("\n\n"); }
            } else {
                char compact_line[160];
                if (hearts >= 0) snprintf(compact_line, sizeof(compact_line), "T:%s  H:%s  HP:%d", t_str, h_str_used, hearts);
                else snprintf(compact_line, sizeof(compact_line), "T:%s  H:%s", t_str, h_str_used);
                pm_custom(term_w - 4); print_padded(compact_line, term_w - 4, "center", "muted"); printf("\n");
            }

            if (too_small) {
                char warn1[128];
                char warn2[128];
                snprintf(warn1, sizeof(warn1), "Terminal too small for %dx%d board.", size, size);
                snprintf(warn2, sizeof(warn2), "Need at least %dx%d (now %dx%d).", grid_width_est + 2, board_rows_est + 2, term_w, term_h);
                printf("\n");
                pm_custom(term_w - 4); print_padded(warn1, term_w - 4, "center", "warning"); printf("\n");
                pm_custom(term_w - 4); print_padded(warn2, term_w - 4, "center", "muted"); printf("\n");
                pm_custom(term_w - 4); print_padded("Resize terminal to continue rendering safely.", term_w - 4, "center", "info"); printf("\n");
            } else {
                ui_anim_tick = cat_tick;
                draw_grid(size, room_map, labels, player_grid, locked_cells, conflicts, control_mode == 1 ? cursor_r : -1, control_mode == 1 ? cursor_c : -1);

                if (compact_level <= 1 && sys_msg[0] != '\0') {
                    printf("\n");
                    draw_dialogue_box(78, "System", sys_msg);
                } else if (compact_level == 0 && current_pet != 0) {
                    printf("\n");
                    draw_pet_dialogue_box(78);
                } else if (compact_level == 0) printf("\n\n");
                if (control_mode == 0) { pm_custom(30); print_theme("Enter Command: ", "info"); printf("%s_", input_buf); }
                if (compact_level == 0) {
                    int pet_tick = (int)(now / 800ULL);
                    printf("\033[s");
                    draw_companion(pet_tick);
                    printf("\033[u");
                }
            }
            end_soft_frame();
            fflush(stdout); last_draw_ms = now; force_redraw = false;
        }

        if (sigint_flag) {
            char b64_out[16384]; export_game(size, num_rooms, rooms, solution, player_grid, locked_cells, hearts, elapsed, 0, 1, NULL, b64_out);
            restore_terminal(); printf("\n\033[1;31m--- GAME INTERRUPTED (Ctrl+C) ---\033[0m\n\nCopy this code to 'Import' later:\n\n%s\n\n", b64_out); exit(0);
        }

        if (kbhit_portable()) {
            bool processed_any = false;
            while (kbhit_portable()) {
                int c = read_key(); 
                if (c == -1) { restore_terminal(); exit(1); }
                sys_msg[0] = '\0';
                if (sigint_flag) break;
                bool movement_key = (c == KEY_UP || c == KEY_DOWN || c == KEY_LEFT || c == KEY_RIGHT || c == 'w' || c == 'W' || c == 'a' || c == 'A' || c == 's' || c == 'S' || c == 'd' || c == 'D');
                if (control_mode == 0 && movement_key) { control_mode = 1; save_stats(); strcpy(sys_msg, "Switched to WASD/Arrows mode."); }
                if (control_mode == 1 && handle_music_hotkeys(c)) { processed_any = true; continue; }
                
                if (control_mode == 1) { 
                    if (c == KEY_UP || c == 'w' || c == 'W') { cursor_r = cursor_r > 0 ? cursor_r - 1 : 0; last_action_time = current_time; idle_stage = 0; }
                    else if (c == KEY_DOWN || c == 's' || c == 'S') { cursor_r = cursor_r < size - 1 ? cursor_r + 1 : size - 1; last_action_time = current_time; idle_stage = 0; }
                    else if (c == KEY_LEFT || c == 'a' || c == 'A') { cursor_c = cursor_c > 0 ? cursor_c - 1 : 0; last_action_time = current_time; idle_stage = 0; }
                    else if (c == KEY_RIGHT || c == 'd' || c == 'D') { cursor_c = cursor_c < size - 1 ? cursor_c + 1 : size - 1; last_action_time = current_time; idle_stage = 0; }
                    else if (c >= '1' && c <= '0' + size) {
                        if (!locked_cells[cursor_r][cursor_c]) {
                            int val = c - '0';
                                if (hearts > 0 && val != solution[cursor_r][cursor_c]) {
                                    hearts--; snprintf(sys_msg, sizeof(sys_msg), "Wrong! -1 Heart. Hearts left: %d", hearts);
                                    queue_pet_dialogue(tr4("Oops. That room answer was not it.", "哎呀，這格答案不對。", "哎呀，这格答案不对。", "おっと、その答えは違うみたい。"));
                                    correct_streak = 0;
                                    last_action_time = current_time;
                                    idle_stage = 0;
                                    if (hearts == 0) { 
                                    printf("\033[H\033[2J"); pm_custom(30); printf("\033[1;31m--- GAME OVER ---\033[0m\n"); 
                                    pm_custom(30); printf("You ran out of hearts!\n"); 
                                    pm_custom(30); printf("Press Enter..."); 
                                    while(read_key() != KEY_ENTER && !sigint_flag) {}
                                    if(sigint_flag) { restore_terminal(); exit(0); }
                                    return;
                                }
                            } else {
                                player_grid[cursor_r][cursor_c] = val;
                                last_action_time = current_time;
                                idle_stage = 0;
                                if (val == solution[cursor_r][cursor_c]) {
                                    correct_streak++;
                                    if (correct_streak == 3) { queue_pet_dialogue(tr4("Combo x3. Perfect rhythm.", "連擊 x3！節奏完美。", "连击 x3！节奏完美。", "コンボ x3！完璧なリズム。")); correct_streak = 0; }
                                } else {
                                    correct_streak = 0;
                                }
                            }
                        } else strcpy(sys_msg, "That cell is a locked hint.");
                    }
                    else if (c == '*') { for(int r=0; r<size; r++) for(int cc=0; cc<size; cc++) if(!locked_cells[r][cc]) player_grid[r][cc] = 0; strcpy(sys_msg, "Board cleared."); correct_streak = 0; last_action_time = current_time; idle_stage = 0; }
                    else if (c == '0' || c == ' ' || c == KEY_BACKSPACE) { if (!locked_cells[cursor_r][cursor_c]) player_grid[cursor_r][cursor_c] = 0; last_action_time = current_time; idle_stage = 0; }
                    else if (c == 'o' || c == 'O') {
                        char b64_out[16384]; export_game(size, num_rooms, rooms, solution, player_grid, locked_cells, hearts, elapsed, 0, 1, NULL, b64_out);
                        time_t pause_start = time(NULL); int res = in_game_option_menu(1, b64_out); if (res == 1) { return; }
                        start_time += (time(NULL) - pause_start); force_redraw = true;
                    }
                    else if (c == 'h' || c == 'H') {
                        hints_used++;
                        apply_progressive_hint(size, solution, player_grid, locked_cells, rooms, num_rooms, room_map, hints_used, conflicts, sys_msg, sizeof(sys_msg));
                        queue_pet_dialogue(tr4("I can still help. Follow the next clue.", "我還能幫你，先跟下一個提示。", "我还能帮你，先跟下一个提示。", "まだ手伝えるよ。次のヒントに進もう。"));
                        last_action_time = current_time;
                        idle_stage = 0;
                    }
                    else if (c == 'c' || c == 'C') { compute_conflicts(size, player_grid, rooms, num_rooms, room_map, conflicts); strcpy(sys_msg, "Conflicts highlighted in red."); last_action_time = current_time; idle_stage = 0; }
                    else if (c == 'i' || c == 'I') { 
                        printf("\033[H\033[2J"); bool el[MAX_SIZE][MAX_SIZE]={0}; draw_grid(size, room_map, labels, solution, el, NULL, -1, -1); 
                        printf("\n"); pm_custom(20); printf("Try harder!\n"); 
                        pm_custom(20); printf("Press Enter...");
                        while(read_key() != KEY_ENTER && !sigint_flag) {}
                        if(sigint_flag) { restore_terminal(); exit(0); }
                        return;
                    }
                    else if (c == 'q' || c == 'Q') { return; }
                    processed_any = true;
                } else { 
                    if (c == KEY_ENTER) {
                        if (strcasecmp(input_buf, "exit") == 0 || strcasecmp(input_buf, "q") == 0) { return; }
                        else if (strcasecmp(input_buf, "option") == 0 || strcasecmp(input_buf, "o") == 0 || strcasecmp(input_buf, "export") == 0) {
                            char b64_out[16384]; export_game(size, num_rooms, rooms, solution, player_grid, locked_cells, hearts, elapsed, 0, 1, NULL, b64_out);
                            time_t pause_start = time(NULL); int res = in_game_option_menu(1, b64_out); if (res == 1) { return; }
                            start_time += (time(NULL) - pause_start); input_buf[0] = '\0'; input_len = 0; force_redraw = true;
                        }
                        else if (strcasecmp(input_buf, "*") == 0 || strcasecmp(input_buf, "clear") == 0) { for(int r=0; r<size; r++) for(int cc=0; cc<size; cc++) if(!locked_cells[r][cc]) player_grid[r][cc] = 0; }
                        else if (strcasecmp(input_buf, "idk") == 0) { 
                            printf("\033[H\033[2J"); bool el[MAX_SIZE][MAX_SIZE]={0}; draw_grid(size, room_map, labels, solution, el, NULL, -1, -1); 
                            printf("\n"); pm_custom(30); printf("Press Enter...");
                            while(read_key() != KEY_ENTER && !sigint_flag) {}
                            if(sigint_flag) { restore_terminal(); exit(0); }
                            return;
                        }
                        else if (strcasecmp(input_buf, "hint") == 0) {
                            hints_used++;
                            apply_progressive_hint(size, solution, player_grid, locked_cells, rooms, num_rooms, room_map, hints_used, conflicts, sys_msg, sizeof(sys_msg));
                            queue_pet_dialogue(tr4("One clue at a time. You have this.", "一步一提示，你做得到。", "一步一提示，你做得到。", "ヒントを一つずつ。きっとできる。"));
                            last_action_time = current_time;
                            idle_stage = 0;
                        }
                        else if (strcasecmp(input_buf, "check") == 0) {
                            compute_conflicts(size, player_grid, rooms, num_rooms, room_map, conflicts);
                            strcpy(sys_msg, "Conflicts highlighted in red.");
                            last_action_time = current_time;
                            idle_stage = 0;
                        }
                        else {
                            int r, c_v, v;
                            if (sscanf(input_buf, "%d %d %d", &r, &c_v, &v) == 3 && r >= 1 && r <= size && c_v >= 1 && c_v <= size && v >= 0 && v <= size) {
                                if (!locked_cells[r-1][c_v-1]) {
                                    if (hearts > 0 && v != 0 && v != solution[r-1][c_v-1]) { hearts--; if (hearts == 0) { 
                                        printf("\033[H\033[2J"); pm_custom(30); printf("\033[1;31m--- GAME OVER ---\033[0m\n"); 
                                        pm_custom(30); printf("Press Enter..."); 
                                        while(read_key() != KEY_ENTER && !sigint_flag) {}
                                        if(sigint_flag) { restore_terminal(); exit(0); }
                                        return;
                                    } } else {
                                        player_grid[r-1][c_v-1] = v;
                                        last_action_time = current_time;
                                        idle_stage = 0;
                                        if (v == solution[r-1][c_v-1]) {
                                            correct_streak++;
                                            if (correct_streak == 3) { queue_pet_dialogue(tr4("Combo x3. Perfect rhythm.", "連擊 x3！節奏完美。", "连击 x3！节奏完美。", "コンボ x3！完璧なリズム。")); correct_streak = 0; }
                                        } else {
                                            correct_streak = 0;
                                        }
                                    }
                                } else strcpy(sys_msg, "Locked hint.");
                            } else strcpy(sys_msg, "Invalid input.");
                        }
                        input_buf[0] = '\0'; input_len = 0; processed_any = true;
                    } 
                    else if (c == KEY_BACKSPACE) { if (input_len > 0) input_buf[--input_len] = '\0'; processed_any = true; }
                    else if (isprint(c) && input_len < (int)sizeof(input_buf) - 1) { input_buf[input_len++] = c; input_buf[input_len] = '\0'; processed_any = true; }
                }
            }
            if (processed_any) force_redraw = true;
        }
        cat_tick++;
        sleep_ms(30);
    }
}

/* -------------------------------------------------------------------------- */
/* MULTIPLAYER GAMEPLAY LOOPS                                                 */
/* -------------------------------------------------------------------------- */

void play_time_attack_end_animation(bool p_win, bool g_win, int p_score, int g_score, int max_score) {
    int cur_p = 0, cur_g = 0;
    const int anim_frames = 25;
    for (int frame = 1; frame <= anim_frames; frame++) {
        if (sigint_flag) break;
        cur_p = (p_score * frame) / anim_frames;
        cur_g = (g_score * frame) / anim_frames;

        char p_line[96];
        char g_line[96];
        snprintf(p_line, sizeof(p_line), "Player Score: %d / %d", cur_p, max_score);
        snprintf(g_line, sizeof(g_line), "Ghost Score:  %d / %d", cur_g, max_score);

        bool win = false;
        const char* headline = "TIME ATTACK OVER";
        const char* line1 = "";
        const char* line2 = "";
        const char* line3 = "Press Enter to return";
        if (p_win) {
            win = true;
            headline = "PLAYER VICTORY";
            line1 = "You cleared the board first.";
        } else if (g_win) {
            win = false;
            headline = "AI VICTORY";
            line1 = "The ghost cleared the board first.";
        } else if (p_score > g_score) {
            win = true;
            headline = "PLAYER VICTORY";
            line1 = "Time ran out, but your board was ahead.";
        } else if (g_score > p_score) {
            win = false;
            headline = "AI VICTORY";
            line1 = "Time ran out and the ghost stayed ahead.";
        } else {
            win = true;
            headline = "DRAW";
            line1 = "Time ran out with equal scores.";
        }
        line2 = p_line;
        line3 = g_line;
        render_fullscreen_result_frame(win, headline, line1, line2, line3, frame);
        sleep_ms(100);
    }
    if (sigint_flag) return;

    const char* headline = "TIME ATTACK OVER";
    const char* line1 = "";
    if (p_win) {
        headline = "PLAYER VICTORY";
        line1 = "You cleared the board first.";
    } else if (g_win) {
        headline = "AI VICTORY";
        line1 = "The ghost cleared the board first.";
    } else if (p_score > g_score) {
        headline = "PLAYER VICTORY";
        line1 = "Time ran out, but your board was ahead.";
    } else if (g_score > p_score) {
        headline = "AI VICTORY";
        line1 = "Time ran out and the ghost stayed ahead.";
    } else {
        headline = "DRAW";
        line1 = "Time ran out with equal scores.";
    }
    {
        char p_value[64];
        char g_value[64];
        char result_value[96];
        const char* keys[] = {"Result", "Player Score", "Ghost Score", "Board Cells"};
        const char* values[] = {result_value, p_value, g_value, "Time attack board"};
        snprintf(p_value, sizeof(p_value), "%d / %d", p_score, max_score);
        snprintf(g_value, sizeof(g_value), "%d / %d", g_score, max_score);
        snprintf(result_value, sizeof(result_value), "%s", line1);
        printf("\033[H\033[2J");
        draw_summary_table("TIME ATTACK SUMMARY", headline, keys, values, 4, 82);
    }
}

void play_sprint_end_animation(int p_idx, int g_idx, int elapsed) {
    bool win = (p_idx == 5);
    char t_str[32];
    char subtitle[128];
    char time_line[128];
    char board_line[128];
    format_duration(elapsed, t_str);
    snprintf(subtitle, sizeof(subtitle), "%s", win ? "You reached the finish first." : "The ghost reached the finish first.");
    snprintf(time_line, sizeof(time_line), "Total Time: %s", t_str);
    snprintf(board_line, sizeof(board_line), "Player boards: %d / 5    Ghost boards: %d / 5", p_idx, g_idx);
    play_sprint_finish_animation(win, p_idx, g_idx,
        win ? "PUZZLE SPRINT CLEAR" : "GHOST WIN",
        subtitle, time_line, board_line);

    {
        char result_value[96];
        char time_value[64];
        char board_value[96];
        const char* keys[] = {"Result", "Total Time", "Boards"};
        const char* values[] = {result_value, time_value, board_value};
        snprintf(result_value, sizeof(result_value), "%s", win ? "You completed all 5 boards first." : "The ghost completed all 5 boards first.");
        snprintf(time_value, sizeof(time_value), "%s", t_str);
        snprintf(board_value, sizeof(board_value), "Player %d / 5 | Ghost %d / 5", p_idx, g_idx);
        printf("\033[H\033[2J");
        draw_summary_table("SPRINT SUMMARY", win ? "Puzzle sprint result" : "Puzzle sprint result", keys, values, 3, 82);
    }
}

void game_loop_time_attack(int size, Room rooms[], int num_rooms, int solution[MAX_SIZE][MAX_SIZE], int room_map[MAX_SIZE][MAX_SIZE], int labels[MAX_SIZE][MAX_SIZE], int player_grid[MAX_SIZE][MAX_SIZE], bool locked_cells[MAX_SIZE][MAX_SIZE], int ai_difficulty, int ghost_grid_in[MAX_SIZE][MAX_SIZE], int initial_elapsed) {
    time_t start_time = time(NULL); unsigned long long last_draw_ms = 0; bool force_redraw = true;
    int ghost_grid[MAX_SIZE][MAX_SIZE]; bool ghost_locked[MAX_SIZE][MAX_SIZE]; int cat_tick = 0;
    bool player_conflicts[MAX_SIZE][MAX_SIZE] = {0};
    
    if (ghost_grid_in != NULL) memcpy(ghost_grid, ghost_grid_in, sizeof(ghost_grid));
    else memcpy(ghost_grid, player_grid, sizeof(ghost_grid)); 
    memcpy(ghost_locked, locked_cells, sizeof(ghost_locked));
    
    int ai_ticks = 0; int ai_threshold = (ai_difficulty == 1) ? 167 : (ai_difficulty == 2) ? 100 : (ai_difficulty == 3) ? 67 : 17; 
    char sys_msg[128] = ""; char input_buf[128] = ""; int input_len = 0; int cursor_r = 0, cursor_c = 0;
    int correct_streak = 0; int idle_stage = 0; time_t last_action_time = time(NULL);

    while (1) {
        unsigned long long now = now_ms();
        time_t current_time = time(NULL); int elapsed = (int)difftime(current_time, start_time) + initial_elapsed;
        int time_left = 180 - elapsed;
        if (current_time - last_action_time >= 10 && idle_stage == 0) {
            set_pet_message(correct_streak, 1, "Stay focused. You can still catch up.");
            idle_stage = 1;
            force_redraw = true;
        } else if (current_time - last_action_time >= 20 && idle_stage == 1) {
            set_pet_message(correct_streak, 2, "Tiny steps still count.");
            idle_stage = 2;
            force_redraw = true;
        }
        bool p_win = check_win(size, player_grid, rooms, num_rooms); bool g_win = check_win(size, ghost_grid, rooms, num_rooms);

        if (p_win || g_win || time_left <= 0) {
            int p_score = 0, g_score = 0;
            for(int r=0; r<size; r++) for(int c=0; c<size; c++) {
                if (player_grid[r][c] == solution[r][c]) p_score++;
                if (ghost_grid[r][c] == solution[r][c]) g_score++;
            }
            if (p_win || (!g_win && p_score >= g_score)) play_victory_track();
            play_time_attack_end_animation(p_win, g_win, p_score, g_score, size*size);
            printf("\n\n"); pm_custom(30); printf("Press Enter to return...");
            wait_for_enter_or_sigint(); restore_background_music(); if(sigint_flag) { restore_terminal(); exit(0); } return;
        }

        ai_ticks++;
        if (ai_difficulty > 0 && ai_ticks >= ai_threshold) {
            ai_ticks = 0;
            if (ghost_take_turn(size, ghost_grid, solution, rooms, num_rooms, room_map, ai_difficulty)) force_redraw = true;
        }

        if ((now - last_draw_ms >= 90ULL) || force_redraw) {
            update_term_size();
            int grid_width = 5 + 6 * size; int total_w = grid_width * 2 + 7;
            int board_rows_est = (size * 2) + 7;
            bool too_small = (term_w < total_w + 2) || (term_h < board_rows_est + 2);
            int compact_level = too_small ? 1 : (term_h < board_rows_est + 10 ? 1 : 0);
            set_frame_row_estimate(too_small ? 9 : (board_rows_est + (compact_level == 0 ? 11 : 7)));

            begin_soft_frame(); pm_custom(total_w); print_theme("--- Time Attack vs Ghost AI ---\n", "title");
            char t_str[32]; format_duration(time_left, t_str);
            {
                char time_buf[64];
                snprintf(time_buf, sizeof(time_buf), "Time Left: %s", t_str);
                pm_custom(total_w); print_padded(time_buf, total_w, "center", "warning"); printf(compact_level == 0 ? "\n\n" : "\n");
            }

            pm_board_custom(total_w, 0);
            print_padded("PLAYER", grid_width, "center", "value"); printf("       "); print_padded("GHOST AI", grid_width, "center", "danger"); printf(compact_level == 0 ? "\n\n" : "\n");
            
            if (compact_level == 0) {
                if (control_mode == 1) { pm_custom(total_w); print_padded("Commands: WASD/Arrows | 1-9 fill | 0 clear | [C]heck | [O]ption | [Q]uit", total_w, "center", "info"); printf("\n\n"); }
                else { pm_custom(total_w); print_padded("Commands: 'row col val' | 'check' | 'clear' | 'option' | 'exit'", total_w, "center", "info"); printf("\n\n"); }
            }

            if (too_small) {
                char warn1[160];
                char warn2[160];
                snprintf(warn1, sizeof(warn1), "Terminal too small for duel board %dx%d.", size, size);
                snprintf(warn2, sizeof(warn2), "Need at least %dx%d (now %dx%d).", total_w + 2, board_rows_est + 2, term_w, term_h);
                printf("\n");
                pm_custom(term_w - 4); print_padded(warn1, term_w - 4, "center", "warning"); printf("\n");
                pm_custom(term_w - 4); print_padded(warn2, term_w - 4, "center", "muted"); printf("\n");
                pm_custom(term_w - 4); print_padded("Resize terminal to continue rendering safely.", term_w - 4, "center", "info"); printf("\n");
            } else {
                ui_anim_tick = cat_tick;
                draw_dual_grid(size, room_map, labels, player_grid, locked_cells, player_conflicts, control_mode == 1 ? cursor_r : -1, control_mode == 1 ? cursor_c : -1, room_map, labels, ghost_grid, ghost_locked, NULL, -1, -1);
                if (compact_level == 0 && sys_msg[0] != '\0') { printf("\n"); pm_custom(strlen(sys_msg)); print_theme(sys_msg, "warning"); printf("\n"); }
                else if (compact_level == 0 && current_pet != 0 && term_h >= board_rows_est + 14) { printf("\n"); draw_pet_dialogue_box(78); printf("\n"); }
                else if (compact_level == 0) printf("\n\n");
                if (control_mode == 0) { pm_custom(30); print_theme("Enter Command: ", "info"); printf("%s_", input_buf); }
                if (compact_level == 0 && current_pet != 0 && term_h >= board_rows_est + 14) {
                    int pet_tick = (int)(now / 800ULL);
                    printf("\033[s");
                    draw_companion(pet_tick);
                    printf("\033[u");
                }
            }
            end_soft_frame();
            fflush(stdout); last_draw_ms = now; force_redraw = false;
        }

        if (sigint_flag) {
            char b64_out[16384]; export_game(size, num_rooms, rooms, solution, player_grid, locked_cells, -1, elapsed, 1, ai_difficulty, ghost_grid, b64_out);
            restore_terminal(); printf("\n\033[1;31m--- GAME INTERRUPTED (Ctrl+C) ---\033[0m\n\nCopy this code to 'Import' later:\n\n%s\n\n", b64_out); exit(0);
        }

        if (kbhit_portable()) {
            bool processed_any = false;
            while (kbhit_portable()) {
                int c = read_key(); 
                if (c == -1) { restore_terminal(); exit(1); }
                sys_msg[0] = '\0';
                if (sigint_flag) break;
                bool movement_key = (c == KEY_UP || c == KEY_DOWN || c == KEY_LEFT || c == KEY_RIGHT || c == 'w' || c == 'W' || c == 'a' || c == 'A' || c == 's' || c == 'S' || c == 'd' || c == 'D');
                if (control_mode == 0 && movement_key) { control_mode = 1; save_stats(); strcpy(sys_msg, "Switched to WASD/Arrows mode."); }
                if (control_mode == 1 && handle_music_hotkeys(c)) { processed_any = true; continue; }
                
                if (control_mode == 1) { 
                    if (c == KEY_UP || c == 'w' || c == 'W') { cursor_r = cursor_r > 0 ? cursor_r - 1 : 0; last_action_time = current_time; idle_stage = 0; }
                    else if (c == KEY_DOWN || c == 's' || c == 'S') { cursor_r = cursor_r < size - 1 ? cursor_r + 1 : size - 1; last_action_time = current_time; idle_stage = 0; }
                    else if (c == KEY_LEFT || c == 'a' || c == 'A') { cursor_c = cursor_c > 0 ? cursor_c - 1 : 0; last_action_time = current_time; idle_stage = 0; }
                    else if (c == KEY_RIGHT || c == 'd' || c == 'D') { cursor_c = cursor_c < size - 1 ? cursor_c + 1 : size - 1; last_action_time = current_time; idle_stage = 0; }
                    else if (c >= '1' && c <= '0' + size) { if (!locked_cells[cursor_r][cursor_c]) { player_grid[cursor_r][cursor_c] = c - '0'; last_action_time = current_time; idle_stage = 0; if (player_grid[cursor_r][cursor_c] == solution[cursor_r][cursor_c]) { correct_streak++; set_pet_message(correct_streak, idle_stage, "Nice move."); if (correct_streak >= 3) correct_streak = 0; } else { correct_streak = 0; } } else strcpy(sys_msg, "Locked hint cell."); }
                    else if (c == '*') { for(int r=0; r<size; r++) for(int cc=0; cc<size; cc++) if(!locked_cells[r][cc]) player_grid[r][cc] = 0; correct_streak = 0; last_action_time = current_time; idle_stage = 0; }
                    else if (c == '0' || c == ' ' || c == KEY_BACKSPACE) { if (!locked_cells[cursor_r][cursor_c]) player_grid[cursor_r][cursor_c] = 0; last_action_time = current_time; idle_stage = 0; correct_streak = 0; }
                    else if (c == 'c' || c == 'C') { compute_conflicts(size, player_grid, rooms, num_rooms, room_map, player_conflicts); strcpy(sys_msg, "Conflicts highlighted in red."); last_action_time = current_time; idle_stage = 0; }
                    else if (c == 'o' || c == 'O') { 
                        char b64_out[16384]; export_game(size, num_rooms, rooms, solution, player_grid, locked_cells, -1, elapsed, 1, ai_difficulty, ghost_grid, b64_out);
                        time_t pause_start = time(NULL); int res = in_game_option_menu(1, b64_out); if (res == 1) { return; } start_time += (time(NULL) - pause_start); force_redraw = true; 
                    }
                    else if (c == 'q' || c == 'Q') { return; }
                    processed_any = true;
                } else { 
                    if (c == KEY_ENTER) {
                        if (strcasecmp(input_buf, "exit") == 0 || strcasecmp(input_buf, "q") == 0) { return; }
                        else if (strcasecmp(input_buf, "*") == 0 || strcasecmp(input_buf, "clear") == 0) { for(int r=0; r<size; r++) for(int cc=0; cc<size; cc++) if(!locked_cells[r][cc]) player_grid[r][cc] = 0; correct_streak = 0; last_action_time = current_time; idle_stage = 0; }
                        else if (strcasecmp(input_buf, "check") == 0) { compute_conflicts(size, player_grid, rooms, num_rooms, room_map, player_conflicts); strcpy(sys_msg, "Conflicts highlighted in red."); last_action_time = current_time; idle_stage = 0; }
                        else if (strcasecmp(input_buf, "option") == 0 || strcasecmp(input_buf, "o") == 0) { 
                            char b64_out[16384]; export_game(size, num_rooms, rooms, solution, player_grid, locked_cells, -1, elapsed, 1, ai_difficulty, ghost_grid, b64_out);
                            time_t pause_start = time(NULL); int res = in_game_option_menu(1, b64_out); if (res == 1) { return; } start_time += (time(NULL) - pause_start); input_buf[0] = '\0'; input_len = 0; force_redraw = true; 
                        }
                        else {
                            int r, c_v, v;
                            if (sscanf(input_buf, "%d %d %d", &r, &c_v, &v) == 3 && r >= 1 && r <= size && c_v >= 1 && c_v <= size && v >= 0 && v <= size) {
                                if (!locked_cells[r-1][c_v-1]) {
                                    player_grid[r-1][c_v-1] = v;
                                    last_action_time = current_time;
                                    idle_stage = 0;
                                    if (v == solution[r-1][c_v-1] && v != 0) {
                                        correct_streak++;
                                        set_pet_message(correct_streak, idle_stage, "Nice move.");
                                        if (correct_streak >= 3) correct_streak = 0;
                                    } else {
                                        correct_streak = 0;
                                    }
                                } else strcpy(sys_msg, "Locked hint cell.");
                            } else strcpy(sys_msg, "Invalid. Format: row col val");
                        }
                        input_buf[0] = '\0'; input_len = 0; processed_any = true;
                    } else if (c == KEY_BACKSPACE) { if (input_len > 0) input_buf[--input_len] = '\0'; processed_any = true; }
                    else if (isprint(c) && input_len < (int)sizeof(input_buf) - 1) { input_buf[input_len++] = c; input_buf[input_len] = '\0'; processed_any = true; }
                }
            }
            if (processed_any) force_redraw = true;
        }
        cat_tick++;
        sleep_ms(30);
    }
}

void game_loop_sprint(int size, int ai_difficulty) {
    Room sprint_rooms[5][MAX_SIZE * MAX_SIZE]; int sprint_num_rooms[5];
    int sprint_solutions[5][MAX_SIZE][MAX_SIZE]; int sprint_room_maps[5][MAX_SIZE][MAX_SIZE]; int sprint_labels[5][MAX_SIZE][MAX_SIZE];
    int player_grids[5][MAX_SIZE][MAX_SIZE]; bool player_locked[5][MAX_SIZE][MAX_SIZE];
    int ghost_grids[5][MAX_SIZE][MAX_SIZE]; bool ghost_locked[5][MAX_SIZE][MAX_SIZE];
    int cat_tick = 0;

    for(int i=0; i<5; i++) {
        generate_puzzle(size, sprint_rooms[i], &sprint_num_rooms[i], sprint_solutions[i], sprint_room_maps[i]); build_labels(size, sprint_rooms[i], sprint_num_rooms[i], sprint_labels[i]);
        create_prefilled_board(size, sprint_solutions[i], player_grids[i], player_locked[i], "median");
        memcpy(ghost_grids[i], player_grids[i], sizeof(ghost_grids[i])); memcpy(ghost_locked[i], player_locked[i], sizeof(ghost_locked[i]));
    }

    int p_idx = 0; int g_idx = 0; time_t start_time = time(NULL); unsigned long long last_draw_ms = 0; bool force_redraw = true;
    int ai_ticks = 0; int ai_threshold = (ai_difficulty == 1) ? 167 : (ai_difficulty == 2) ? 100 : (ai_difficulty == 3) ? 67 : 17; 
    char sys_msg[128] = ""; char input_buf[128] = ""; int input_len = 0; int cursor_r = 0, cursor_c = 0;
    int correct_streak = 0; int idle_stage = 0; time_t last_action_time = time(NULL);
    bool player_conflicts[MAX_SIZE][MAX_SIZE] = {0};

    while (1) {
        unsigned long long now = now_ms();
        time_t current_time = time(NULL); int elapsed = (int)difftime(current_time, start_time);
        if (current_time - last_action_time >= 10 && idle_stage == 0) {
            set_pet_message(correct_streak, 1, "Keep going. One board at a time.");
            idle_stage = 1;
            force_redraw = true;
        } else if (current_time - last_action_time >= 20 && idle_stage == 1) {
            set_pet_message(correct_streak, 2, "Tiny steps still count.");
            idle_stage = 2;
            force_redraw = true;
        }
        if (check_win(size, player_grids[p_idx], sprint_rooms[p_idx], sprint_num_rooms[p_idx])) { p_idx++; force_redraw = true; }
        if (check_win(size, ghost_grids[g_idx], sprint_rooms[g_idx], sprint_num_rooms[g_idx])) { g_idx++; force_redraw = true; }

        if (p_idx == 5 || g_idx == 5) {
            if (p_idx == 5) play_victory_track();
            play_sprint_end_animation(p_idx, g_idx, elapsed);
            printf("\n\n"); pm_custom(30); printf("Press Enter to return...");
            wait_for_enter_or_sigint(); restore_background_music(); if(sigint_flag) { restore_terminal(); exit(0); } return;
        }

        ai_ticks++;
        if (ai_difficulty > 0 && ai_ticks >= ai_threshold && g_idx < 5) {
            ai_ticks = 0;
            if (ghost_take_turn(size, ghost_grids[g_idx], sprint_solutions[g_idx], sprint_rooms[g_idx], sprint_num_rooms[g_idx], sprint_room_maps[g_idx], ai_difficulty)) force_redraw = true;
        }

        if ((now - last_draw_ms >= 90ULL) || force_redraw) {
            update_term_size();
            int grid_width = 5 + 6 * size; int total_w = grid_width * 2 + 7;
            int board_rows_est = (size * 2) + 7;
            bool too_small = (term_w < total_w + 2) || (term_h < board_rows_est + 2);
            int compact_level = too_small ? 1 : (term_h < board_rows_est + 10 ? 1 : 0);
            set_frame_row_estimate(too_small ? 9 : (board_rows_est + (compact_level == 0 ? 11 : 7)));

            begin_soft_frame(); pm_custom(total_w); print_theme("--- Puzzle Sprint vs Ghost AI ---\n", "title");
            char t_str[32]; format_duration(elapsed, t_str);
            {
                char time_buf[64];
                snprintf(time_buf, sizeof(time_buf), "Elapsed Time: %s", t_str);
                pm_custom(total_w); print_padded(time_buf, total_w, "center", "warning"); printf(compact_level == 0 ? "\n\n" : "\n");
            }
            char pt_str[64]; snprintf(pt_str, sizeof(pt_str), "PLAYER (Board %d / 5)", p_idx + 1); char gt_str[64]; snprintf(gt_str, sizeof(gt_str), "GHOST AI (Board %d / 5)", g_idx + 1);
            
            pm_board_custom(total_w, 0); print_padded(pt_str, grid_width, "center", "value"); printf("       "); print_padded(gt_str, grid_width, "center", "danger"); printf(compact_level == 0 ? "\n\n" : "\n");
            
            if (compact_level == 0) {
                if (control_mode == 1) { pm_custom(total_w); print_padded("Commands: WASD/Arrows | 1-9 fill | 0 clear | [C]heck | [O]ption | [Q]uit", total_w, "center", "info"); printf("\n\n"); }
                else { pm_custom(total_w); print_padded("Commands: 'row col val' | 'check' | 'clear' | 'option' | 'exit'", total_w, "center", "info"); printf("\n\n"); }
            }

            if (too_small) {
                char warn1[160];
                char warn2[160];
                snprintf(warn1, sizeof(warn1), "Terminal too small for sprint duel board %dx%d.", size, size);
                snprintf(warn2, sizeof(warn2), "Need at least %dx%d (now %dx%d).", total_w + 2, board_rows_est + 2, term_w, term_h);
                printf("\n");
                pm_custom(term_w - 4); print_padded(warn1, term_w - 4, "center", "warning"); printf("\n");
                pm_custom(term_w - 4); print_padded(warn2, term_w - 4, "center", "muted"); printf("\n");
                pm_custom(term_w - 4); print_padded("Resize terminal to continue rendering safely.", term_w - 4, "center", "info"); printf("\n");
            } else {
                ui_anim_tick = cat_tick;
                draw_dual_grid(size, sprint_room_maps[p_idx], sprint_labels[p_idx], player_grids[p_idx], player_locked[p_idx], player_conflicts, control_mode == 1 ? cursor_r : -1, control_mode == 1 ? cursor_c : -1, sprint_room_maps[g_idx], sprint_labels[g_idx], ghost_grids[g_idx], ghost_locked[g_idx], NULL, -1, -1);
                if (compact_level == 0 && sys_msg[0] != '\0') { printf("\n"); pm_custom(strlen(sys_msg)); print_theme(sys_msg, "warning"); printf("\n"); }
                else if (compact_level == 0 && current_pet != 0 && term_h >= board_rows_est + 14) { printf("\n"); draw_pet_dialogue_box(78); printf("\n"); }
                else if (compact_level == 0) printf("\n\n");
                if (control_mode == 0) { pm_custom(30); print_theme("Enter Command: ", "info"); printf("%s_", input_buf); }
                if (compact_level == 0 && current_pet != 0 && term_h >= board_rows_est + 14) {
                    int pet_tick = (int)(now / 800ULL);
                    printf("\033[s");
                    draw_companion(pet_tick);
                    printf("\033[u");
                }
            }
            end_soft_frame();
            fflush(stdout); last_draw_ms = now; force_redraw = false;
        }

        if (sigint_flag) {
            char b64_out[16384]; export_game(size, sprint_num_rooms[p_idx], sprint_rooms[p_idx], sprint_solutions[p_idx], player_grids[p_idx], player_locked[p_idx], -1, elapsed, 1, ai_difficulty, ghost_grids[p_idx], b64_out);
            restore_terminal(); printf("\n\033[1;31m--- GAME INTERRUPTED (Ctrl+C) ---\033[0m\n\nCopy this code to 'Import' later:\n\n%s\n\n", b64_out); exit(0);
        }

        if (kbhit_portable()) {
            bool processed_any = false;
            while (kbhit_portable()) {
                int c = read_key(); 
                if (c == -1) { restore_terminal(); exit(1); }
                sys_msg[0] = '\0';
                if (sigint_flag) break;
                if (control_mode == 1 && handle_music_hotkeys(c)) { processed_any = true; continue; }
                
                if (control_mode == 1) { 
                    if (c == KEY_UP || c == 'w' || c == 'W') { cursor_r = cursor_r > 0 ? cursor_r - 1 : 0; last_action_time = current_time; idle_stage = 0; }
                    else if (c == KEY_DOWN || c == 's' || c == 'S') { cursor_r = cursor_r < size - 1 ? cursor_r + 1 : size - 1; last_action_time = current_time; idle_stage = 0; }
                    else if (c == KEY_LEFT || c == 'a' || c == 'A') { cursor_c = cursor_c > 0 ? cursor_c - 1 : 0; last_action_time = current_time; idle_stage = 0; }
                    else if (c == KEY_RIGHT || c == 'd' || c == 'D') { cursor_c = cursor_c < size - 1 ? cursor_c + 1 : size - 1; last_action_time = current_time; idle_stage = 0; }
                    else if (c >= '1' && c <= '0' + size) { if (!player_locked[p_idx][cursor_r][cursor_c]) { player_grids[p_idx][cursor_r][cursor_c] = c - '0'; last_action_time = current_time; idle_stage = 0; if (player_grids[p_idx][cursor_r][cursor_c] == sprint_solutions[p_idx][cursor_r][cursor_c] && player_grids[p_idx][cursor_r][cursor_c] != 0) { correct_streak++; set_pet_message(correct_streak, idle_stage, "Nice move."); if (correct_streak >= 3) correct_streak = 0; } else { correct_streak = 0; } } else strcpy(sys_msg, "Locked hint cell."); }
                    else if (c == '*') { for(int r=0; r<size; r++) for(int cc=0; cc<size; cc++) if(!player_locked[p_idx][r][cc]) player_grids[p_idx][r][cc] = 0; correct_streak = 0; last_action_time = current_time; idle_stage = 0; }
                    else if (c == '0' || c == ' ' || c == KEY_BACKSPACE) { if (!player_locked[p_idx][cursor_r][cursor_c]) player_grids[p_idx][cursor_r][cursor_c] = 0; last_action_time = current_time; idle_stage = 0; correct_streak = 0; }
                    else if (c == 'c' || c == 'C') { compute_conflicts(size, player_grids[p_idx], sprint_rooms[p_idx], sprint_num_rooms[p_idx], sprint_room_maps[p_idx], player_conflicts); strcpy(sys_msg, "Conflicts highlighted in red."); last_action_time = current_time; idle_stage = 0; }
                    else if (c == 'o' || c == 'O') { 
                        char b64_out[16384]; export_game(size, sprint_num_rooms[p_idx], sprint_rooms[p_idx], sprint_solutions[p_idx], player_grids[p_idx], player_locked[p_idx], -1, elapsed, 1, ai_difficulty, ghost_grids[p_idx], b64_out);
                        time_t pause_start = time(NULL); int res = in_game_option_menu(1, b64_out); if (res == 1) { return; } start_time += (time(NULL) - pause_start); force_redraw = true; 
                    }
                    else if (c == 'q' || c == 'Q') { return; }
                    processed_any = true;
                } else { 
                    if (c == KEY_ENTER) {
                        if (strcasecmp(input_buf, "exit") == 0 || strcasecmp(input_buf, "q") == 0) { return; }
                        else if (strcasecmp(input_buf, "*") == 0 || strcasecmp(input_buf, "clear") == 0) { for(int r=0; r<size; r++) for(int cc=0; cc<size; cc++) if(!player_locked[p_idx][r][cc]) player_grids[p_idx][r][cc] = 0; correct_streak = 0; last_action_time = current_time; idle_stage = 0; }
                        else if (strcasecmp(input_buf, "check") == 0) { compute_conflicts(size, player_grids[p_idx], sprint_rooms[p_idx], sprint_num_rooms[p_idx], sprint_room_maps[p_idx], player_conflicts); strcpy(sys_msg, "Conflicts highlighted in red."); last_action_time = current_time; idle_stage = 0; }
                        else if (strcasecmp(input_buf, "option") == 0 || strcasecmp(input_buf, "o") == 0) { 
                            char b64_out[16384]; export_game(size, sprint_num_rooms[p_idx], sprint_rooms[p_idx], sprint_solutions[p_idx], player_grids[p_idx], player_locked[p_idx], -1, elapsed, 1, ai_difficulty, ghost_grids[p_idx], b64_out);
                            time_t pause_start = time(NULL); int res = in_game_option_menu(1, b64_out); if (res == 1) { return; } start_time += (time(NULL) - pause_start); input_buf[0] = '\0'; input_len = 0; force_redraw = true; 
                        }
                        else {
                            int r, c_v, v;
                            if (sscanf(input_buf, "%d %d %d", &r, &c_v, &v) == 3 && r >= 1 && r <= size && c_v >= 1 && c_v <= size && v >= 0 && v <= size) {
                                if (!player_locked[p_idx][r-1][c_v-1]) {
                                    player_grids[p_idx][r-1][c_v-1] = v;
                                    last_action_time = current_time;
                                    idle_stage = 0;
                                    if (v == sprint_solutions[p_idx][r-1][c_v-1] && v != 0) {
                                        correct_streak++;
                                        set_pet_message(correct_streak, idle_stage, "Nice move.");
                                        if (correct_streak >= 3) correct_streak = 0;
                                    } else {
                                        correct_streak = 0;
                                    }
                                } else strcpy(sys_msg, "Locked hint cell.");
                            } else strcpy(sys_msg, "Invalid. Format: row col val");
                        }
                        input_buf[0] = '\0'; input_len = 0; processed_any = true;
                    } else if (c == KEY_BACKSPACE) { if (input_len > 0) input_buf[--input_len] = '\0'; processed_any = true; }
                    else if (isprint(c) && input_len < (int)sizeof(input_buf) - 1) { input_buf[input_len++] = c; input_buf[input_len] = '\0'; processed_any = true; }
                }
            }
            if (processed_any) force_redraw = true;
        }
        cat_tick++;
        sleep_ms(30);
    }
}

/* -------------------------------------------------------------------------- */
/* MENUS & RUNNERS                                                            */
/* -------------------------------------------------------------------------- */

void play_single_player_menu() {
    int sel = 0, start_idx = 0, back_idx = 0;
    int type = 1, size_fixed = 0, size_rand = 2, diff = 2;
    const char* types[] = {
        tr4("Fixed Room", "固定房", "固定房", "固定ルーム"),
        tr4("Random Room", "隨機房", "随机房", "ランダムルーム")
    };
    const char* sizes_fixed[] = {"3x3", "5x5"};
    const char* sizes_rand[] = {"3x3", "4x4", "5x5", "6x6", "7x7", "8x8", "9x9"};
    const char* diffs[] = {
        tr4("Nightmare", "夢魘", "梦魇", "ナイトメア"),
        tr4("Hard", "困難", "困难", "難しい"),
        tr4("Median", "中等", "中等", "普通"),
        tr4("Easy", "簡單", "简单", "やさしい")
    };
    
    MENU_LOOP_BEGIN(tick)
        print_banner_centered();
        int item_idx = 0; int block_w = 60;
        char buf[256]; char f[256];
        
        pm_custom(block_w);
        snprintf(buf, sizeof(buf), "%s: %s", tr4("Room Type", "房間類型", "房间类型", "ルームタイプ"), types[type]);
        if (sel == item_idx) { snprintf(f, sizeof(f), ">>>>>   %s: < %s >   <<<<<", tr4("Room Type", "房間類型", "房间类型", "ルームタイプ"), types[type]); print_padded(f, block_w, "center", "cursor"); }
        else { print_padded(buf, block_w, "center", "info"); }
        printf("\n"); item_idx++;
        
        pm_custom(block_w);
        snprintf(buf, sizeof(buf), "%s: %s", tr4("Board Size", "棋盤大小", "棋盘大小", "盤面サイズ"), type == 0 ? sizes_fixed[size_fixed] : sizes_rand[size_rand]);
        if (sel == item_idx) { snprintf(f, sizeof(f), ">>>>>   %s: < %s >   <<<<<", tr4("Board Size", "棋盤大小", "棋盘大小", "盤面サイズ"), type == 0 ? sizes_fixed[size_fixed] : sizes_rand[size_rand]); print_padded(f, block_w, "center", "cursor"); }
        else { print_padded(buf, block_w, "center", "info"); }
        printf("\n"); item_idx++;
        
        if (type == 1) {
            pm_custom(block_w);
            snprintf(buf, sizeof(buf), "%s: %s", tr4("Difficulty", "難度", "难度", "難易度"), diffs[diff]);
            if (sel == item_idx) { snprintf(f, sizeof(f), ">>>>>   %s: < %s >   <<<<<", tr4("Difficulty", "難度", "难度", "難易度"), diffs[diff]); print_padded(f, block_w, "center", "cursor"); }
            else { print_padded(buf, block_w, "center", "info"); }
            printf("\n"); item_idx++;
        }
        
        pm_custom(block_w);
        if (sel == item_idx) { print_padded(tr4(">>>>>   START GAME   <<<<<", ">>>>>   開始遊戲   <<<<<", ">>>>>   开始游戏   <<<<<", ">>>>>   ゲーム開始   <<<<<"), block_w, "center", "cursor"); } else { print_padded(tr4("START GAME", "開始遊戲", "开始游戏", "ゲーム開始"), block_w, "center", "info"); }
        printf("\n"); start_idx = item_idx++;
        
        pm_custom(block_w);
        if (sel == item_idx) { print_padded(tr4(">>>>>   BACK   <<<<<", ">>>>>   返回   <<<<<", ">>>>>   返回   <<<<<", ">>>>>   戻る   <<<<<"), block_w, "center", "cursor"); } else { print_padded(tr4("BACK", "返回", "返回", "戻る"), block_w, "center", "info"); }
        printf("\n"); back_idx = item_idx++;
        
    MENU_LOOP_END(tick)
        
        int num_items = type == 1 ? 5 : 4; 
        if (c == KEY_UP || c == 'w' || c == 'W') sel = (sel - 1 + num_items) % num_items;
        else if (c == KEY_DOWN || c == 's' || c == 'S') sel = (sel + 1) % num_items;
        else if (c == KEY_LEFT || c == 'a' || c == 'A') {
            if (sel == 0) type = 1 - type;
            else if (sel == 1 && type == 0) size_fixed = size_fixed > 0 ? size_fixed - 1 : 1;
            else if (sel == 1 && type == 1) size_rand = size_rand > 0 ? size_rand - 1 : 6;
            else if (sel == 2 && type == 1) diff = diff > 0 ? diff - 1 : 3;
            if (sel >= num_items) sel = num_items - 1;
        }
        else if (c == KEY_RIGHT || c == 'd' || c == 'D') {
            if (sel == 0) type = 1 - type;
            else if (sel == 1 && type == 0) size_fixed = (size_fixed + 1) % 2;
            else if (sel == 1 && type == 1) size_rand = (size_rand + 1) % 7;
            else if (sel == 2 && type == 1) diff = (diff + 1) % 4;
            if (sel >= num_items) sel = num_items - 1;
        }
        else if (c == KEY_ENTER) {
            if (sel == 0) {
                type = 1 - type;
                if (sel >= num_items) sel = num_items - 1;
            } else if (sel == 1 && type == 0) {
                size_fixed = (size_fixed + 1) % 2;
            } else if (sel == 1 && type == 1) {
                size_rand = (size_rand + 1) % 7;
            } else if (sel == 2 && type == 1) {
                diff = (diff + 1) % 4;
            } else if (sel == start_idx) {
                if (type == 0) {
                    int size, num_rooms; Room* rooms; int solution[MAX_SIZE][MAX_SIZE];
                    load_fixed_puzzle(size_fixed + 1, &size, &rooms, &num_rooms, solution);
                    int room_map[MAX_SIZE][MAX_SIZE], labels[MAX_SIZE][MAX_SIZE], player_grid[MAX_SIZE][MAX_SIZE] = {0}; bool locked_cells[MAX_SIZE][MAX_SIZE] = {0};
                    build_room_map(size, rooms, num_rooms, room_map); build_labels(size, rooms, num_rooms, labels);
                    play_join_animation();
                    game_loop(size, rooms, num_rooms, solution, room_map, labels, player_grid, locked_cells, "fixed", "fixed", -1, 0);
                    sel = 0; 
                } else {
                    int size = size_rand + 3;
                    const char* diff_str = diff == 0 ? "nightmare" : (diff == 1 ? "hard" : (diff == 2 ? "median" : "easy"));
                    int hearts = diff == 0 ? 3 : -1;
                    Room rooms[MAX_SIZE * MAX_SIZE]; int num_rooms; int grid[MAX_SIZE][MAX_SIZE], room_map[MAX_SIZE][MAX_SIZE], labels[MAX_SIZE][MAX_SIZE];
                    int player_grid[MAX_SIZE][MAX_SIZE]; bool locked_cells[MAX_SIZE][MAX_SIZE];
                    generate_puzzle(size, rooms, &num_rooms, grid, room_map); build_labels(size, rooms, num_rooms, labels); create_prefilled_board(size, grid, player_grid, locked_cells, diff_str);
                    play_join_animation();
                    game_loop(size, rooms, num_rooms, grid, room_map, labels, player_grid, locked_cells, "random", diff_str, hearts, 0);
                    sel = 0;
                }
            } else if (sel == back_idx) {
                break;
            }
        }
    }
}

void play_multi_player_menu() {
    int sel = 0, start_idx = 0, back_idx = 0;
    int mode = 0, size_idx = 2, diff = 1;
    const char* modes[] = {
        tr4("Time Attack", "計時對戰", "计时对战", "タイムアタック"),
        tr4("Puzzle Sprint", "拼圖衝刺", "拼图冲刺", "パズルスプリント"),
        tr4("Daily Challenge", "每日挑戰", "每日挑战", "デイリーチャレンジ"),
        tr4("Hardcore Run", "硬核闖關", "硬核闯关", "ハードコアラン")
    };
    const char* sizes[] = {"3x3", "4x4", "5x5", "6x6", "7x7", "8x8", "9x9"};
    const char* diffs[] = {
        tr4("Easy", "簡單", "简单", "やさしい"),
        tr4("Normal", "普通", "普通", "ノーマル"),
        tr4("Hard", "困難", "困难", "難しい"),
        tr4("Nightmare", "夢魘", "梦魇", "ナイトメア")
    };
    
    MENU_LOOP_BEGIN(tick)
        print_banner_centered();
        int item_idx = 0; int block_w = 60;
        char buf[256]; char f[256];
        
        pm_custom(block_w);
        snprintf(buf, sizeof(buf), "%s: %s", tr4("Game Mode", "遊戲模式", "游戏模式", "ゲームモード"), modes[mode]);
        if (sel == item_idx) { snprintf(f, sizeof(f), ">>>>>   %s: < %s >   <<<<<", tr4("Game Mode", "遊戲模式", "游戏模式", "ゲームモード"), modes[mode]); print_padded(f, block_w, "center", "cursor"); }
        else { print_padded(buf, block_w, "center", "info"); }
        printf("\n"); item_idx++;
        
        pm_custom(block_w);
        snprintf(buf, sizeof(buf), "%s: %s", tr4("Board Size", "棋盤大小", "棋盘大小", "盤面サイズ"), sizes[size_idx]);
        if (sel == item_idx) { snprintf(f, sizeof(f), ">>>>>   %s: < %s >   <<<<<", tr4("Board Size", "棋盤大小", "棋盘大小", "盤面サイズ"), sizes[size_idx]); print_padded(f, block_w, "center", "cursor"); }
        else { print_padded(buf, block_w, "center", "info"); }
        printf("\n"); item_idx++;
        
        pm_custom(block_w);
        snprintf(buf, sizeof(buf), "%s: %s", tr4("Ghost AI Diff", "幽靈AI難度", "幽灵AI难度", "ゴーストAI難易度"), diffs[diff]);
        if (sel == item_idx) { snprintf(f, sizeof(f), ">>>>>   %s: < %s >   <<<<<", tr4("Ghost AI Diff", "幽靈AI難度", "幽灵AI难度", "ゴーストAI難易度"), diffs[diff]); print_padded(f, block_w, "center", "cursor"); }
        else { print_padded(buf, block_w, "center", "info"); }
        printf("\n"); item_idx++;
        
        pm_custom(block_w);
        if (sel == item_idx) { print_padded(tr4(">>>>>   START GAME   <<<<<", ">>>>>   開始遊戲   <<<<<", ">>>>>   开始游戏   <<<<<", ">>>>>   ゲーム開始   <<<<<"), block_w, "center", "cursor"); } else { print_padded(tr4("START GAME", "開始遊戲", "开始游戏", "ゲーム開始"), block_w, "center", "info"); }
        printf("\n"); start_idx = item_idx++;
        
        pm_custom(block_w);
        if (sel == item_idx) { print_padded(tr4(">>>>>   BACK   <<<<<", ">>>>>   返回   <<<<<", ">>>>>   返回   <<<<<", ">>>>>   戻る   <<<<<"), block_w, "center", "cursor"); } else { print_padded(tr4("BACK", "返回", "返回", "戻る"), block_w, "center", "info"); }
        printf("\n"); back_idx = item_idx++;
        
    MENU_LOOP_END(tick)
        
        int num_items = 5;
        if (c == KEY_UP || c == 'w' || c == 'W') sel = (sel - 1 + num_items) % num_items;
        else if (c == KEY_DOWN || c == 's' || c == 'S') sel = (sel + 1) % num_items;
        else if (c == KEY_LEFT || c == 'a' || c == 'A') {
        if (sel == 0) mode = (mode + 1) % 4;
        else if (sel == 1) size_idx = size_idx > 0 ? size_idx - 1 : 6;
        else if (sel == 2) diff = diff > 0 ? diff - 1 : 3;
        }
        else if (c == KEY_RIGHT || c == 'd' || c == 'D') {
            if (sel == 0) mode = (mode + 1) % 4;
            else if (sel == 1) size_idx = (size_idx + 1) % 7;
            else if (sel == 2) diff = (diff + 1) % 4;
        }
        else if (c == KEY_ENTER) {
            if (sel == 0) {
                mode = (mode + 1) % 4;
            } else if (sel == 1) {
                size_idx = (size_idx + 1) % 7;
            } else if (sel == 2) {
                diff = (diff + 1) % 4;
            } else if (sel == start_idx) {
                int size = size_idx + 3;
                int ai_diff = diff + 1;
                play_join_animation();
                if (mode == 0) {
                    Room rooms[MAX_SIZE * MAX_SIZE]; int num_rooms; int grid[MAX_SIZE][MAX_SIZE], room_map[MAX_SIZE][MAX_SIZE], labels[MAX_SIZE][MAX_SIZE];
                    int player_grid[MAX_SIZE][MAX_SIZE]; bool locked_cells[MAX_SIZE][MAX_SIZE];
                    generate_puzzle(size, rooms, &num_rooms, grid, room_map); build_labels(size, rooms, num_rooms, labels); create_prefilled_board(size, grid, player_grid, locked_cells, "median");
                    game_loop_time_attack(size, rooms, num_rooms, grid, room_map, labels, player_grid, locked_cells, ai_diff, NULL, 0);
                } else if (mode == 1) {
                    game_loop_sprint(size, ai_diff);
                } else if (mode == 2) {
                    unsigned restore_seed = (unsigned)time(NULL) ^ (unsigned)clock();
                    time_t now = time(NULL);
                    struct tm* tm_now = localtime(&now);
                    unsigned daily_seed = 0;
                    if (tm_now) daily_seed = (unsigned)((tm_now->tm_year + 1900) * 10000 + (tm_now->tm_mon + 1) * 100 + tm_now->tm_mday);
                    srand(daily_seed);
                    Room rooms[MAX_SIZE * MAX_SIZE]; int num_rooms; int grid[MAX_SIZE][MAX_SIZE], room_map[MAX_SIZE][MAX_SIZE], labels[MAX_SIZE][MAX_SIZE];
                    int player_grid[MAX_SIZE][MAX_SIZE]; bool locked_cells[MAX_SIZE][MAX_SIZE];
                    generate_puzzle(size, rooms, &num_rooms, grid, room_map); build_labels(size, rooms, num_rooms, labels); create_prefilled_board(size, grid, player_grid, locked_cells, "median");
                    game_loop_time_attack(size, rooms, num_rooms, grid, room_map, labels, player_grid, locked_cells, ai_diff, NULL, 0);
                    srand(restore_seed);
                } else {
                    int hardcore_size = size < 5 ? 5 : size;
                    Room rooms[MAX_SIZE * MAX_SIZE]; int num_rooms; int grid[MAX_SIZE][MAX_SIZE], room_map[MAX_SIZE][MAX_SIZE], labels[MAX_SIZE][MAX_SIZE];
                    int player_grid[MAX_SIZE][MAX_SIZE]; bool locked_cells[MAX_SIZE][MAX_SIZE];
                    generate_puzzle(hardcore_size, rooms, &num_rooms, grid, room_map); build_labels(hardcore_size, rooms, num_rooms, labels); create_prefilled_board(hardcore_size, grid, player_grid, locked_cells, "hard");
                    game_loop(hardcore_size, rooms, num_rooms, grid, room_map, labels, player_grid, locked_cells, "random", "hard", 1, 0);
                }
                sel = 0;
            } else if (sel == back_idx) {
                break;
            }
        }
    }
}

void build_custom_game_interactive() {
    begin_text_entry_screen("--- Custom Room Builder ---");
    pm_custom(30); printf("Build a custom challenge code\n\n");

    int size = 0;
    if (!prompt_int_value("Enter Board Size (3-9): ", 3, MAX_SIZE, &size)) {
        end_text_entry_screen();
        return;
    }

    Room rooms[MAX_SIZE * MAX_SIZE];
    int num_rooms;
    int grid[MAX_SIZE][MAX_SIZE], room_map[MAX_SIZE][MAX_SIZE];
    int player_grid[MAX_SIZE][MAX_SIZE];
    bool locked_cells[MAX_SIZE][MAX_SIZE];
    generate_puzzle(size, rooms, &num_rooms, grid, room_map);
    for (int r = 0; r < size; r++) for (int c = 0; c < size; c++) {
        player_grid[r][c] = 0;
        locked_cells[r][c] = false;
    }

    int hints = 0;
    char hint_prompt[80];
    snprintf(hint_prompt, sizeof(hint_prompt), "How many pre-filled hint cells? (0 to %d): ", size * size);
    if (!prompt_int_value(hint_prompt, 0, size * size, &hints)) {
        end_text_entry_screen();
        return;
    }
    Point pos[MAX_SIZE * MAX_SIZE];
    int idx = 0;
    for (int r = 0; r < size; r++) for (int c = 0; c < size; c++) pos[idx++] = (Point){r, c};
    for (int i = size * size - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Point tmp = pos[i];
        pos[i] = pos[j];
        pos[j] = tmp;
    }
    for (int i = 0; i < hints; i++) {
        player_grid[pos[i].r][pos[i].c] = grid[pos[i].r][pos[i].c];
        locked_cells[pos[i].r][pos[i].c] = true;
    }

    int hearts = 0;
    if (!prompt_int_value("Set Hearts (-1 for infinite, 1-9 for strict mode): ", -1, 9, &hearts)) {
        end_text_entry_screen();
        return;
    }
    if (hearts == 0) hearts = -1;

    char code[16384];
    export_game(size, num_rooms, rooms, grid, player_grid, locked_cells, hearts, 0, 0, 1, NULL, code);

    clear_screen();
    printf("\n");
    pm_custom(45); printf("\033[1;32mYour custom challenge is ready!\033[0m\n\n");
    pm_custom(65); printf("Puzzle summary: %dx%d board | %d rooms | %d hints | hearts %s\n\n",
        size, size, num_rooms, hints, hearts == -1 ? "infinite" : "limited");
    pm_custom(65); printf("Share this code with friends, or replay the exact setup later.\n\n");
    pm_custom(strlen(code)); printf("%s\n\n", code);
    pm_custom(35); printf("Press Enter to return to main menu...");
    fflush(stdout);

    char wait_buf[8];
    fgets(wait_buf, sizeof(wait_buf), stdin);
    end_text_entry_screen();
}

void play_custom_room() {
    const char* opts[] = {"Import Game from Code", "Build Custom Game", "Back"};
    int c = run_menu_centered(print_banner_centered, "Import / Build Custom Room", opts, 3);
    if (c == 0) {
        begin_text_entry_screen("--- Import Custom Game ---");
        pm_custom(30); printf("Enter the short game code: "); char code[32768]; 
        if (!fgets(code, sizeof(code), stdin)) { end_text_entry_screen(); return; }
        end_text_entry_screen();
        
        int size, num_rooms, hearts, elapsed, mp_mode, ai_diff; Room rooms[MAX_SIZE * MAX_SIZE]; 
        int solution[MAX_SIZE][MAX_SIZE], player_grid[MAX_SIZE][MAX_SIZE], ghost_grid[MAX_SIZE][MAX_SIZE]; bool locked_cells[MAX_SIZE][MAX_SIZE];
        
        if (import_game(code, &size, &num_rooms, rooms, solution, player_grid, locked_cells, &hearts, &elapsed, &mp_mode, &ai_diff, ghost_grid)) {
            int room_map[MAX_SIZE][MAX_SIZE], labels[MAX_SIZE][MAX_SIZE]; build_room_map(size, rooms, num_rooms, room_map); build_labels(size, rooms, num_rooms, labels);
            
            play_join_animation();
            
            if (mp_mode == 1) {
                game_loop_time_attack(size, rooms, num_rooms, solution, room_map, labels, player_grid, locked_cells, ai_diff, ghost_grid, elapsed);
            } else {
                game_loop(size, rooms, num_rooms, solution, room_map, labels, player_grid, locked_cells, "custom", "custom", hearts, elapsed);
            }
        } else { 
            printf("\n"); pm_custom(35); printf("\033[1;31mInvalid or corrupted code string!\033[0m\n");
            pm_custom(35); printf("Press Enter..."); 
            while(read_key() != KEY_ENTER && !sigint_flag) {}
            if(sigint_flag) { restore_terminal(); exit(0); }
        }
    } else if (c == 1) {
        build_custom_game_interactive();
    }
}

void print_profile_summary() {
    MENU_LOOP_BEGIN(tick)
        print_banner_centered();
        int w = 72;
        pm_custom(w); print_theme("========================================================================\n", "border");
        pm_custom(w); print_theme("                         PROFILE & SETTINGS\n", "title");
        pm_custom(w); print_theme("========================================================================\n\n", "border");
        pm_custom(w); print_theme("Record\n", "accent");
        pm_custom(w); print_theme("  Total wins: ", "header"); printf("\033[32m%d\033[0m", total_wins);
        print_theme("   Best score: ", "header"); printf("\033[32m%d\033[0m\n\n", best_score);
        pm_custom(w); print_theme("Companion\n", "accent");
    pm_custom(w); print_theme("  Active pet: ", "header"); print_theme(get_pet_name(), "value"); print_theme("   [P] Cycle Companion (None / Chiikawa / Hachiware / Usagi / Momonga)\n", "info");
        pm_custom(w); print_theme("  Team: ", "header");
    print_theme(current_pet == 0 ? "{None}" : "None", current_pet == 0 ? "cursor" : "info"); printf("  ");
    print_theme(current_pet == 1 ? "{Chiikawa}" : "Chiikawa", current_pet == 1 ? "cursor" : "info"); printf("  ");
    print_theme(current_pet == 2 ? "{Hachiware}" : "Hachiware", current_pet == 2 ? "cursor" : "info"); printf("  ");
    print_theme(current_pet == 3 ? "{Usagi}" : "Usagi", current_pet == 3 ? "cursor" : "info"); printf("  ");
    print_theme(current_pet == 4 ? "{Momonga}" : "Momonga", current_pet == 4 ? "cursor" : "info"); printf("\n\n");
        printf("\033[s"); draw_companion(tick / 2); printf("\033[u");
        pm_custom(w); print_theme("Achievements\n", "accent"); int masks[] = {ACH_FIRST_WIN, ACH_NO_HINT, ACH_SPEED, ACH_HARD, ACH_BOSS, ACH_IRON_HEART, ACH_MEGA_GRID, ACH_SCORE_MASTER, ACH_NIGHTMARE_SPEED, ACH_STEADY_HAND, ACH_HINT_COMMANDER, ACH_COMEBACK_SPIRIT};
        for(int i=0; i<12; i++) { 
            pm_custom(w);
            if (unlocked_achievements & masks[i]) printf("\033[32m[x]\033[0m %s\n", get_ach_text(masks[i])); 
            else printf("\033[2m[ ]\033[0m %s\n", get_ach_text(masks[i])); 
        }
        printf("\n");
        pm_custom(w); print_theme("Settings\n", "accent");
        pm_custom(w); print_theme("  [T]", "info"); printf(" Toggle Input Mode (Current: %s)\n", control_mode == 1 ? "\033[32mNew (WASD/Arrows)\033[0m" : "\033[33mOld (String Input)\033[0m");
        pm_custom(w); print_theme("  [C]", "info"); const char* t_name = current_theme == 0 ? "\033[36mCyberpunk & Sci-Fi\033[0m" : (current_theme == 1 ? "\033[32mFantasy & RPG\033[0m" : (current_theme == 2 ? "\033[31mHorror & Survival\033[0m" : (current_theme == 3 ? "\033[33mSteampunk & Invention\033[0m" : "\033[35mBiopunk & Organic Horror\033[0m")));
        printf(" Cycle Game Theme  (Current: %s)\n", t_name);
    pm_custom(w); print_theme("  [M]", "info"); printf(" Open Music Player (Playing: \033[36m%s\033[0m)\n", get_clean_track_display_name(current_track));
        pm_custom(w); print_theme("  [Q]/[Enter]", "info"); printf(" Back to menu\n"); fflush(stdout);
    MENU_LOOP_END(tick)
        if (c == 't' || c == 'T') { control_mode = 1 - control_mode; save_stats(); } else if (c == 'c' || c == 'C') { current_theme = (current_theme + 1) % 5; save_stats(); } 
        else if (c == 'p' || c == 'P') { current_pet = (current_pet + 1) % 5; save_stats(); }
        else if (c == 'm' || c == 'M') { music_player_menu(); } else if (c == '\n' || c == '\r' || c == KEY_ENTER || c == 'q' || c == 'Q' || c == 27) { break; }
    }
}

void print_profile_summary_v2() {
    int selected = 0;
    while (1) {
        update_term_size();
        begin_soft_frame();
        int w = 78;
        int panel_w = 38;
        int masks[] = {ACH_FIRST_WIN, ACH_NO_HINT, ACH_SPEED, ACH_HARD, ACH_BOSS, ACH_IRON_HEART, ACH_MEGA_GRID, ACH_SCORE_MASTER, ACH_NIGHTMARE_SPEED, ACH_STEADY_HAND, ACH_HINT_COMMANDER, ACH_COMEBACK_SPIRIT};
        int unlocked_count = 0;
        for (int i = 0; i < 12; i++) if (unlocked_achievements & masks[i]) unlocked_count++;
        const char* theme_name = current_theme == 0 ? tr4("Cyberpunk & Sci-Fi", "賽博龐克與科幻", "赛博朋克与科幻", "サイバーパンク&SF") :
                                  (current_theme == 1 ? tr4("Fantasy & RPG", "奇幻與RPG", "奇幻与RPG", "ファンタジー&RPG") :
                                  (current_theme == 2 ? tr4("Horror & Survival", "恐怖與生存", "恐怖与生存", "ホラー&サバイバル") :
                                  (current_theme == 3 ? tr4("Steampunk & Invention", "蒸汽龐克與發明", "蒸汽朋克与发明", "スチームパンク&発明") :
                                                        tr4("Biopunk & Organic Horror", "生物龐克與有機恐怖", "生物朋克与有机恐怖", "バイオパンク&オーガニックホラー"))));
        char row0[128], row1[128], row2[128], row3[128], row4[128], row5[128], row6[64];
        snprintf(row0, sizeof(row0), "Companion : %s (pick None to disable)", get_pet_name());
        snprintf(row1, sizeof(row1), "%s", tr4("Achievements : View", "成就 : 檢視", "成就 : 查看", "実績 : 表示"));
        snprintf(row2, sizeof(row2), "%s: %s", tr4("Input Mode", "輸入模式", "输入模式", "入力モード"), control_mode == 1 ? tr4("New (WASD/Arrows)", "新模式 (WASD/方向鍵)", "新模式 (WASD/方向键)", "新操作 (WASD/矢印)") : tr4("Old (String Input)", "舊模式 (字串輸入)", "旧模式 (字符串输入)", "旧操作 (文字入力)"));
        snprintf(row3, sizeof(row3), "%s : %s", tr4("Theme", "主題", "主题", "テーマ"), theme_name);
        snprintf(row4, sizeof(row4), "%s : %s", tr4("Music", "音樂", "音乐", "音楽"), get_clean_track_display_name(current_track));
        snprintf(row5, sizeof(row5), "%s : %s", tr4("Language", "語言", "语言", "言語"), get_language_name());
        snprintf(row6, sizeof(row6), "%s", tr4("[ Back ]", "[ 返回 ]", "[ 返回 ]", "[ 戻る ]"));
        const char* rows[] = {row0, row1, row2, row3, row4, row5, row6};

        pm_custom(w); print_theme(tr4("Profile & Settings\n", "個人檔案與設定\n", "个人资料与设置\n", "プロフィールと設定\n"), "title");
        pm_custom(w); print_theme("=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n", "border");
        pm_custom(w); print_padded(tr4("[W/S] Move   [A/D or Enter] Change   [Q] Back", "[W/S] 移動   [A/D或Enter] 更改   [Q] 返回", "[W/S] 移动   [A/D或Enter] 更改   [Q] 返回", "[W/S] 移動   [A/D/Enter] 変更   [Q] 戻る"), w, "center", "info"); printf("\n");
        pm_custom(w); print_theme("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n\n", "border");

        pm_custom(w); print_theme(tr4("Snapshot\n", "快照\n", "快照\n", "スナップショット\n"), "accent");
        {
            char snapshot[160];
            snprintf(snapshot, sizeof(snapshot), "%s %d   |   %s %d   |   %s %d / 12",
                     tr4("Wins", "勝場", "胜场", "勝利数"),
                     total_wins,
                     tr4("Best Score", "最佳分數", "最佳分数", "最高スコア"),
                     best_score,
                     tr4("Achievements", "成就", "成就", "実績"),
                     unlocked_count);
            pm_custom(w); print_padded(snapshot, w, "center", "header"); printf("\n\n");
        }

        for (int i = 0; i < 7; i++) {
            pm_custom(panel_w);
            if (i == selected) {
                char buf[160];
                snprintf(buf, sizeof(buf), "%s", rows[i]);
                print_padded(buf, panel_w, "center", "cursor");
            } else {
                print_padded(rows[i], panel_w, "center", i == 6 ? "muted" : "info");
            }
            printf("\n");
        }

        printf("\n");
        pm_custom(w); print_theme(selected == 1 ? tr4("Achievements Preview\n", "成就預覽\n", "成就预览\n", "実績プレビュー\n") : tr4("Companion Preview\n", "夥伴預覽\n", "伙伴预览\n", "相棒プレビュー\n"), "accent"); printf("\n");
        if (selected == 1) {
            if (unlocked_count == 0) {
                pm_custom(w); print_padded(tr4("No achievements yet. Your first win will unlock one.", "尚未解鎖成就，首次勝利後會解鎖。", "尚未解锁成就，首次胜利后会解锁。", "まだ実績はありません。初勝利で解除されます。"), w, "center", "muted"); printf("\n");
            } else {
                for (int i = 0; i < 12; i++) {
                    char line[256];
                    snprintf(line, sizeof(line), "%s %s",
                             (unlocked_achievements & masks[i]) ? "[x]" : "[ ]",
                             get_ach_text(masks[i]));
                    pm_custom(w);
                    print_padded(line, w, "center", (unlocked_achievements & masks[i]) ? "info" : "muted");
                    printf("\n");
                }
            }
        } else {
            draw_companion_preview_centered(24);
            pm_custom(w);
            print_padded(current_pet == 0 ? "[None]  Chiikawa  Hachiware  Usagi  Momonga" :
                         current_pet == 1 ? " None  [Chiikawa]  Hachiware  Usagi  Momonga" :
                         current_pet == 2 ? " None   Chiikawa  [Hachiware]  Usagi  Momonga" :
                         current_pet == 3 ? " None   Chiikawa   Hachiware  [Usagi]  Momonga" :
                                            " None   Chiikawa   Hachiware   Usagi  [Momonga]",
                         w, "center", "header");
        }
        printf("\n\n");

        pm_custom(w); print_theme(tr4("Latest Milestones\n", "最新里程碑\n", "最新里程碑\n", "最新マイルストーン\n"), "accent"); printf("\n");
        if (unlocked_count == 0) {
            pm_custom(w); print_padded(tr4("No achievements yet. Your first win will unlock one.", "尚未解鎖成就，首次勝利後會解鎖。", "尚未解锁成就，首次胜利后会解锁。", "まだ実績はありません。初勝利で解除されます。"), w, "center", "muted"); printf("\n");
        } else {
            int shown = 0;
            for (int i = 0; i < 12; i++) {
                if (!(unlocked_achievements & masks[i])) continue;
                char line[256];
                snprintf(line, sizeof(line), "+ %s", get_ach_text(masks[i]));
                pm_custom(w); print_padded(line, w, "center", "info"); printf("\n");
                shown++;
                if (shown >= 2) break;
            }
        }
        printf("\n");
        pm_custom(w); print_padded(tr4("[W/S] Move   [A/D or Enter] Change   [Q] Back", "[W/S] 移動   [A/D或Enter] 更改   [Q] 返回", "[W/S] 移动   [A/D或Enter] 更改   [Q] 返回", "[W/S] 移動   [A/D/Enter] 変更   [Q] 戻る"), w, "center", "clue"); printf("\n");
        fflush(stdout);

        int c = read_key();
        if (c == -1) { restore_terminal(); exit(1); }
        if (sigint_flag) { restore_terminal(); exit(0); }
        if (c == KEY_UP || c == 'w' || c == 'W') selected = (selected - 1 + 7) % 7;
        else if (c == KEY_DOWN || c == 's' || c == 'S') selected = (selected + 1) % 7;
        else if (c == KEY_LEFT || c == 'a' || c == 'A') {
            if (selected == 0) { current_pet = (current_pet + 4) % 5; save_stats(); }
            else if (selected == 2) { control_mode = 1 - control_mode; save_stats(); }
            else if (selected == 3) { current_theme = (current_theme + 4) % 5; save_stats(); }
            else if (selected == 4) { current_track = (current_track - 1 + num_tracks) % num_tracks; play_current_track(); save_stats(); }
            else if (selected == 5) { current_language = (current_language + 3) % 4; save_stats(); }
        } else if (c == KEY_RIGHT || c == 'd' || c == 'D' || c == KEY_ENTER) {
            if (selected == 0) { current_pet = (current_pet + 1) % 5; save_stats(); }
            else if (selected == 2) { control_mode = 1 - control_mode; save_stats(); }
            else if (selected == 3) { current_theme = (current_theme + 1) % 5; save_stats(); }
            else if (selected == 4) { music_player_menu(); }
            else if (selected == 5) { current_language = (current_language + 1) % 4; save_stats(); }
            else if (selected == 6) { break; }
        } else if (c == 'q' || c == 'Q' || c == 27) {
            break;
        }
    }
}

bool handle_music_hotkeys(int c) {
    if (c == '-' || c == '_') { music_volume = music_volume >= 10 ? music_volume - 10 : 0; set_audio_volume(); save_stats(); return true; }
    if (c == '+' || c == '=') { music_volume = music_volume <= 190 ? music_volume + 10 : 200; set_audio_volume(); save_stats(); return true; }
    if (c == 'n' || c == 'N') { current_track = (current_track + 1) % num_tracks; play_current_track(); save_stats(); return true; }
    if (c == 'p' || c == 'P') { current_track = (current_track - 1 + num_tracks) % num_tracks; play_current_track(); save_stats(); return true; }
    return false;
}

int main() {
    init_terminal(); 
    init_game_base_dir();
    setvbuf(stdin, NULL, _IONBF, 0); 
    srand((unsigned)time(NULL)); 
    
    signal(SIGINT, handle_sigint); 
#ifndef _WIN32
    signal(SIGTERM, handle_sigint); 
    signal(SIGHUP, handle_sigint);
#endif
    
    atexit(cleanup_all); 
    load_stats();
    
    play_current_track();

    // Enable Raw Mode and GUI buffer ONCE here
    set_terminal_mode(1); printf("\033[?1049h\033[?25l");

    while (1) {
        const char* main_opts[] = {
            "Single Player", 
            "Arcade Mode", 
            "Profile & Settings", 
            "Import / Build custom game", 
            "Help", 
            "Quit"
        };
        
        int choice = run_menu_centered(print_banner_centered, NULL, main_opts, 6);
        
        if (choice == -1 || choice == 5) { 
            play_exit_animation();
            printf("\033[?1049l\033[?25h");
            set_terminal_mode(0); // Safely restore state on exit
            clear_screen(); 
            break; 
        }
        else if (choice == 0) {
            play_single_player_menu();
        }
        else if (choice == 1) {
            play_arcade_mode_menu();
        }
        else if (choice == 2) {
            print_profile_summary_v2();
        }
        else if (choice == 3) {
            play_custom_room();
        }
        else if (choice == 4) {
            print_help_screen();
        }
    }
    
    restore_terminal();
    return 0;
}

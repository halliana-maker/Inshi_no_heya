/*
 * Inshi_no_heya
 *
 * Main entry point for the game. The implementation is split into numbered
 * parts below so the original 4000+ line file is easier to maintain while
 * preserving the same compilation order and file-scope state.
 */
#include "Inshi_no_heya_part1.c"
#include "Inshi_no_heya_part2.c"
#include "Inshi_no_heya_part3.c"
#include "Inshi_no_heya_part4.c"

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

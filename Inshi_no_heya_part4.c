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
    (void)!fgets(wait_buf, sizeof(wait_buf), stdin);
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

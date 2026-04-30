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
                snprintf(buf, sizeof(buf), ">>>>>   %.240s   <<<<<", opt_texts[i]);
                print_padded(buf, block_w, "center", "cursor"); 
            }
            else { 
                print_padded(opt_texts[i], block_w, "center", "info"); 
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

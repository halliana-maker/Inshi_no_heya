void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    int clear_rc = system("clear");
    (void)clear_rc;
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
                (void)!fscanf(f, "wins=%d\nscore=%d\nachievements=%d\nmode=%d\ntheme=%d\nvol=%d\ntrack=%d\n",
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
    pm_board_custom(board_w, 5); printf("     "); for(int i=0; i<size; i++) { char buf[16]; snprintf(buf, sizeof(buf), "%d", i+1); print_padded(buf, 6, "center", "header"); } printf("\033[K\n");
    pm_board_custom(board_w, 5); char border_top[256] = "    "; for(int i=0; i<size; i++) strcat(border_top, "______"); strcat(border_top, "_"); print_theme(border_top, "border"); printf("\033[K\n");

    for(int r=0; r<size; r++) {
        pm_board_custom(board_w, 5); print_theme("    |", "border");
        for(int c=0; c<size; c++) {
            bool has_right = (c == size - 1 || room_map[r][c] != room_map[r][c + 1]); char lbl[10] = "";
            if (labels[r][c] != 0) snprintf(lbl, sizeof(lbl), "%d", labels[r][c]);
            if (lbl[0] != '\0') print_padded(lbl, 5, "ljust", "clue"); else printf("     ");
            print_theme(has_right ? "|" : " ", "border");
        } printf("\033[K\n");

        pm_board_custom(board_w, 5); char r2h[16]; snprintf(r2h, sizeof(r2h), " %2d |", r + 1); print_theme(r2h, "header");
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
    printf("     "); for(int i=0; i<size; i++) { char buf[16]; snprintf(buf, sizeof(buf), "%d", i+1); print_padded(buf, 6, "center", "header"); }
    printf("   |   "); 
    printf("     "); for(int i=0; i<size; i++) { char buf[16]; snprintf(buf, sizeof(buf), "%d", i+1); print_padded(buf, 6, "center", "header"); }
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

        pm_board_custom(total_w, 5); char r2h[16]; snprintf(r2h, sizeof(r2h), " %2d |", r + 1); print_theme(r2h, "header");
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

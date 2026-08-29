#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>
#include <raymath.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

#define BASE_SCREEN_W 540
#define BASE_SCREEN_H 360

static float SCREEN_W = 540.0f;
static float SCREEN_H = 360.0f;

#define ALPHABET_SIZE 26

#define WORD_SIZE 5
#define GUESSES 6

#define NUM_LINES 5757

#define FONT_SIZE 25
/*
when enter pressed, check for existence of word.
if exists, then do stuff
*/

typedef struct {
    char *data;
    size_t size;
} string;

typedef struct {
    float dur;
    float elapsed;
    bool is_active;
    bool was_active;
} timer;

typedef struct {
    Vector2 start_pos;
    Vector2 end_pos;
} line;

typedef struct {
    string word;
    Vector2 word_size;
    Rectangle boxes[WORD_SIZE];
    Color colors[WORD_SIZE];
    float box_trans[WORD_SIZE];
    bool box_anim[WORD_SIZE]; // for each box to draw independently
    bool guessed; // flag to trigger animation
    bool anim_done;
} row;

typedef struct {
    char c[2];
    bool used;
} letter; // for the used letters

typedef struct {
    letter letters[ALPHABET_SIZE];
    line name_highlight;
    char name[8]; // just to display the thing above the letters.
    Vector2 pos;
} available_letters;

typedef struct {
    line borders[4];
} screen_deco; // screen decoration

static row b[GUESSES] = {0}; // board
static screen_deco sd = {0}; // everything around / decoration
static available_letters al = {0};

static Vector2 scale = { 1.0f, 1.0f };

static bool is_anim_playing = false;
static bool game_ended = false;
static bool game_exited = false;

static int anim_idx=0;
static timer anim_box_time = {0};

#define BASE_SPACING (Vector2){ 1.0f, 1.0f }
#define BASE_LINE_THICKNESS (Vector2){ 1.0f, 1.0f }
#define BASE_BOX_SIZE (Vector2){ 35.0f, 35.0f }

#define BASE_AVAILABLE_LETTERS_POS (Vector2){ 405.0f, 100.0f }

static Vector2 spacing = { 1.0f, 1.0f };
static Vector2 line_thickness = { 1.0f, 1.0f };
static Vector2 box_size = { 35.0f, 35.0f };

// these r set durin update.
static Vector2 grid_size = { 0.0f, 0.0f };

static int guess_count = 1;

static const char *get_key(int key) { // stupid thing i need to make cuz glfw is stupid
    switch (key) {
    // Letters
    case KEY_A: return "a";
    case KEY_B: return "b";
    case KEY_C: return "c";
    case KEY_D: return "d";
    case KEY_E: return "e";
    case KEY_F: return "f";
    case KEY_G: return "g";
    case KEY_H: return "h";
    case KEY_I: return "i";
    case KEY_J: return "j";
    case KEY_K: return "k";
    case KEY_L: return "l";
    case KEY_M: return "m";
    case KEY_N: return "n";
    case KEY_O: return "o";
    case KEY_P: return "p";
    case KEY_Q: return "q";
    case KEY_R: return "r";
    case KEY_S: return "s";
    case KEY_T: return "t";
    case KEY_U: return "u";
    case KEY_V: return "v";
    case KEY_W: return "w";
    case KEY_X: return "x";
    case KEY_Y: return "y";
    case KEY_Z: return "z";
    case KEY_SPACE:     return "space";
    case KEY_ENTER:     return "enter";
    case KEY_TAB:       return "tab";
    case KEY_BACKSPACE: return "backspace";
    case KEY_ESCAPE:    return "escape";
    case KEY_DELETE:    return "delete";
    case KEY_INSERT:    return "insert";
    case KEY_UP:    return "up";
    case KEY_DOWN:  return "down";
    case KEY_LEFT:  return "left";
    case KEY_RIGHT: return "right";
    case KEY_HOME:     return "home";
    case KEY_END:      return "end";
    case KEY_PAGE_UP:  return "page_up";
    case KEY_PAGE_DOWN:return "page_down";
    case KEY_LEFT_SHIFT:   return "left_shift";
    case KEY_RIGHT_SHIFT:  return "right_shift";
    case KEY_LEFT_CONTROL: return "left_control";
    case KEY_RIGHT_CONTROL:return "right_control";
    case KEY_LEFT_ALT:     return "left_alt";
    case KEY_RIGHT_ALT:    return "right_alt";
    case KEY_CAPS_LOCK:    return "caps_lock";
    case KEY_F1:  return "f1";
    case KEY_F2:  return "f2";
    case KEY_F3:  return "f3";
    case KEY_F4:  return "f4";
    case KEY_F5:  return "f5";
    case KEY_F6:  return "f6";
    case KEY_F7:  return "f7";
    case KEY_F8:  return "f8";
    case KEY_F9:  return "f9";
    case KEY_F10: return "f10";
    case KEY_F11: return "f11";
    case KEY_F12: return "f12";
    case KEY_MINUS:         return "-";
    case KEY_EQUAL:         return "=";
    case KEY_LEFT_BRACKET:  return "[";
    case KEY_RIGHT_BRACKET: return "]";
    case KEY_BACKSLASH:     return "\\";
    case KEY_SEMICOLON:     return ";";
    case KEY_APOSTROPHE:    return "'";
    case KEY_COMMA:         return ",";
    case KEY_PERIOD:        return ".";
    case KEY_SLASH:         return "/";
    case KEY_GRAVE:         return "`";

    default:
        return NULL;
    }
}

static Vector2 vec_add_x(const Vector2 vec, float f) {
    return (Vector2){
        vec.x + f,
        vec.y
    };
}

static Vector2 vec_add_y(const Vector2 vec, float f) {
    return (Vector2){
        vec.x,
        vec.y + f
    };
}

static Vector2 vec_sub_x(const Vector2 vec, float f) {
    return (Vector2){
        vec.x - f,
        vec.y
    };
}

static Vector2 vec_sub_y(const Vector2 vec, float f) {
    return (Vector2){
        vec.x,
        vec.y - f
    };
}

static Vector2 rect_center_pos(const Vector2 *sqr_size, const Vector2 *pos) {
    return (Vector2){
        pos->x - (sqr_size->x / 2),
        pos->y - (sqr_size->y / 2)
    };
}

static Vector2 text_mid_cent_rect(const Rectangle *rect, const Vector2 *text_pos) {
    return (Vector2){
        rect->x + rect->width / 2.0f - text_pos->x / 2.0f,
        rect->y + rect->height / 2.0f - text_pos->y / 2.0f
    };
}

static float scaled_font_size() {
    return FONT_SIZE * fminf(scale.x, scale.y);
}

static line create_line(const Vector2 sp, const Vector2 ep) {
    return (line){
        sp,
        ep
    };
}

// helpers for the timer utility.
void start_timer(timer *t, float dur) {
    t->dur = dur;
    t->elapsed = 0.0f;
    t->is_active = true;
}

void update_timer(timer *t) {
    if (t->elapsed >= t->dur) {
        t->is_active = false;
        t->was_active = true;
        return;
    }
    t->elapsed += GetFrameTime();
}

bool is_timer_active(const timer *t) {
    return t->is_active;
}

bool was_timer_active(const timer *t) {
    return t->was_active;
}

void reset_timer(timer *t) {
    t->is_active = false;
    t->elapsed = 0.0f;
    t->was_active = false;
}

// game stuff below
void init_available_letters() {
    for (int i=0; i<ALPHABET_SIZE; ++i) {
        al.letters[i].c[1] = '\0';
    }
    al.letters[0].c[0] = 'a';
    al.letters[1].c[0] = 'b';
    al.letters[2].c[0] = 'c';
    al.letters[3].c[0] = 'd';
    al.letters[4].c[0] = 'e';
    al.letters[5].c[0] = 'f';
    al.letters[6].c[0] = 'g';
    al.letters[7].c[0] = 'h';
    al.letters[8].c[0] = 'i';
    al.letters[9].c[0] = 'j';
    al.letters[10].c[0] = 'k';
    al.letters[11].c[0] = 'l';
    al.letters[12].c[0] = 'm';
    al.letters[13].c[0] = 'n';
    al.letters[14].c[0] = 'o';
    al.letters[15].c[0] = 'p';
    al.letters[16].c[0] = 'q';
    al.letters[17].c[0] = 'r';
    al.letters[18].c[0] = 's';
    al.letters[19].c[0] = 't';
    al.letters[20].c[0] = 'u';
    al.letters[21].c[0] = 'v';
    al.letters[22].c[0] = 'w';
    al.letters[23].c[0] = 'x';
    al.letters[24].c[0] = 'y';
    al.letters[25].c[0] = 'z';

    char letter_name[7] = "Letters";
    memcpy(al.name, letter_name, 7);
    al.name[7] = '\0';

    al.pos = BASE_AVAILABLE_LETTERS_POS;
}

void init_board() {
    for (int i=0; i<GUESSES; ++i) {
        for (int j=0; j<WORD_SIZE; ++j) {
            b[i].colors[j] = BLACK;
            b[i].box_trans[j] = 1.0f;
            b[i].box_anim[j] = false;
        }
    }
}

void init_screen_deco() {}

// this function picks a random word, and also stores all the words at runtime.
char* pick_random_word(string *word_list) {
    int line = rand() % NUM_LINES;
    FILE *file = fopen("words/words.txt", "r");

    char buf[50];
    int line_count = 0;
    while (fgets(buf, 50, file) && line_count < line-1) {
        line_count++;
    }
    buf[WORD_SIZE] = '\0';
    char *word = malloc(WORD_SIZE+1);
    memcpy(word, buf, WORD_SIZE);
    word[WORD_SIZE] = '\0';
    
    fseek(file, 0, SEEK_END);
    word_list->size = ftell(file);
    fseek(file, 0, SEEK_SET);
    word_list->data = malloc(word_list->size);
    printf("%s\n", word);

    fread(word_list->data, 1, word_list->size, file);
    fclose(file);
    return word;
}

bool check_guess(const char *guess, const string word_list) {
    if (strlen(guess) != WORD_SIZE) return false;
    for (size_t i=0; i<word_list.size; i+=WORD_SIZE+2) {
        char buf[WORD_SIZE+1];
        memset(buf, 0, sizeof(char));
        memcpy(buf, word_list.data + i, WORD_SIZE);
        if (strcmp(guess, buf) == 0) return true;
    }
    return false;
}

void add_guess_to_board(string *guess, int guess_idx) {
    b[guess_idx].word = (string){
        .data = malloc(WORD_SIZE+1),
        .size = WORD_SIZE
    };
    memcpy(b[guess_idx].word.data, guess->data, WORD_SIZE);
    b[guess_idx].word_size = MeasureTextEx(GetFontDefault(), guess->data, FONT_SIZE, 1);
}

bool check_availability(const char *word, string *guess, const string word_list) {
    if (IsKeyPressed(KEY_ENTER) && check_guess(guess->data, word_list)) {
        if (strcmp(guess->data, word) == 0) {
            // then they won
            int guess_idx = guess_count-1;
            add_guess_to_board(guess, guess_idx);
            for (int i=0; i<WORD_SIZE; ++i) {
                b[guess_idx].colors[i] = GREEN;
            }
            b[guess_idx].guessed = true;            
            guess_count++;
            guess->size = 0;
            printf("You win!\n");
            return true;
        }
        else if (strcmp(guess->data, word) != 0 && guess_count <= GUESSES) {
            int guess_idx = guess_count - 1;
            add_guess_to_board(guess, guess_idx);
            char remaining[WORD_SIZE + 1];
            strcpy(remaining, word);
            for (size_t i=0; i<WORD_SIZE; ++i) {
                if (guess->data[i] == word[i]) {
                    b[guess_idx].colors[i] = GREEN;
                    remaining[i] = '\0';
                }
            }

            for (size_t i=0; i<WORD_SIZE; ++i) {
                if (ColorIsEqual(b[guess_idx].colors[i], GREEN))
                continue;

                for (size_t j=0; j<WORD_SIZE; ++j) {
                    if (remaining[j] != '\0' && guess->data[i] == remaining[j]) {
                        b[guess_idx].colors[i] = YELLOW;
                        remaining[j] = '\0';
                        break;
                    }
                }
            }

            b[guess_idx].guessed = true;
            guess_count++;

            memset(guess->data, 0, WORD_SIZE + 1);
            guess->size = 0;
        }
        else if (guess_count == GUESSES-1) {
            printf("You lose!\n");
            guess_count++;
            return true;
        }
    }
    else if (IsKeyPressed(KEY_ENTER) && !check_guess(guess->data, word_list)) {
        printf("Invalid\n");
        memset(guess->data, 0, sizeof(char));
        guess->size = 0;
    }
    return false;
}

void process_input(string *guess) {
    if (game_ended) return;
    int key = GetKeyPressed();
    const char *c = get_key(key);
    if (!c) return;
    if (guess->size > 0 && strcmp(c, "backspace") == 0) {
        guess->data[guess->size-1] = '\0';
        guess->size--;
        printf("%s\n", guess->data);
        return;
    }
    if (guess->size >= WORD_SIZE) return;
    strncat(guess->data, c, 1);
    guess->size++;
}

void end_game(const char *word) {
    DrawText(TextFormat("Game finished, the word was... %s\nPress Q to exit, Press N to start a new game.", word), 30, 10, FONT_SIZE, BLACK);
}

void draw_input(string *guess) {
    const float font_size = scaled_font_size();
    for (size_t i=0; i<guess->size; ++i) {
        char c = guess->data[i];
        const char str_c[2] = { c, '\0' };
        const Vector2 char_size = MeasureTextEx(GetFontDefault(), str_c, font_size, 1);
        DrawLine((box_size.x - (5.0f * scale.x))*(i+1) - (char_size.x/2), (SCREEN_H/2) + (char_size.y / 2), (box_size.x - (5.0f * scale.x))*(i+1) + (char_size.x/2), (SCREEN_H/2) + (char_size.y / 2), BLACK);
        DrawText(str_c, (box_size.x - (5.0f * scale.x))*(i+1) - (char_size.x/2), (SCREEN_H/2) - (char_size.y / 2), font_size, BLACK);
    }
    
}

void draw_guess_text(int outer_idx, int inner_idx) {
    char c = b[outer_idx].word.data[inner_idx];
    const char str_c[2] = { c, '\0' };
    const float font_size = scaled_font_size();
    const Vector2 char_size = MeasureTextEx(GetFontDefault(), str_c, font_size, spacing.x);
    const Vector2 text_pos = text_mid_cent_rect(&(b[outer_idx].boxes[inner_idx]), (&char_size));
    DrawText(str_c, text_pos.x, text_pos.y, font_size, b[outer_idx].colors[inner_idx]);
}

void draw_board() {
    for (int i=1; i<GUESSES+1; ++i) {
        for (int j=1; j<WORD_SIZE+1; ++j) {
            if (i >= guess_count || !b[i-1].box_anim[j-1]) DrawRectangleRec(b[i-1].boxes[j-1], BLACK); // timer to slowly fade out
            else if (b[i-1].box_anim[j-1]) {
                draw_guess_text(i-1, j-1);
            }
        }
    }
}

void draw_box_anim(row *r) {
    if (!r->guessed) return;
    if (!r->anim_done) {
        is_anim_playing = true;

        if (anim_idx >= WORD_SIZE) {
            anim_idx=0;
            r->anim_done = true;
            is_anim_playing = false;
            return;
        }

        // locals for each box
        Color box_color = BLACK;
        if (!is_timer_active(&anim_box_time) && !was_timer_active(&anim_box_time)) {
            start_timer(&anim_box_time, 0.2f);
        }
        update_timer(&anim_box_time);
        // logic
        box_color = Fade(box_color, r->box_trans[anim_idx]);
        DrawRectangleRec(r->boxes[anim_idx], box_color);
        r->box_trans[anim_idx] -= 0.2f;
        if (!is_timer_active(&anim_box_time)) {
            // if timer finished, display the character.)
            r->box_anim[anim_idx] = true;
            anim_idx++;
            reset_timer(&anim_box_time);
        }
    }
}

void draw_screen_deco() {
    float line_thick = line_thickness.y;
    for (int i=0; i<4; ++i) {
        DrawLineEx(sd.borders[i].start_pos, sd.borders[i].end_pos, line_thick, BLACK);
        line_thick = i % 2 == 0 ? line_thickness.y : line_thickness.x;
    }
}

void draw_available_letters() {
    const float font_size = scaled_font_size();

//    DrawLineEx(al.name_highlight.start_pos, al.name_highlight.end_pos, line_thickness.y, BLACK); next draw line TODO::::
    int offset=0;
    for (int i=0; i<6; ++i) {
        const Vector2 char_size = MeasureTextEx(GetFontDefault(), al.letters[offset].c, font_size, 1);

        if (al.letters[offset].used) {
            DrawText(al.letters[offset].c, al.pos.x + (i * spacing.x * 15.0f), al.pos.y, font_size, RED);
        }
        else {
            DrawText(al.letters[offset].c, al.pos.x + (i * spacing.x * 15.0f), al.pos.y, font_size, BLACK);
        }
        offset++;
    }
    
    for (int i=0; i<6; ++i) {
        const Vector2 char_size = MeasureTextEx(GetFontDefault(), al.letters[offset].c, font_size, 1);

        if (al.letters[offset].used) {
            DrawText(al.letters[offset].c, al.pos.x + (i * spacing.x * 15.0f), al.pos.y + (25.0f * scale.y), font_size, RED);
        }
        else {
            DrawText(al.letters[offset].c, al.pos.x + (i * spacing.x * 15.0f), al.pos.y + (25.0f * scale.y), font_size, BLACK);
        }
        offset++;
    }

    for (int i=0; i<6; ++i) {
        const Vector2 char_size = MeasureTextEx(GetFontDefault(), al.letters[offset].c, font_size, 1);

        if (al.letters[offset].used) {
            DrawText(al.letters[offset].c, al.pos.x + (i * spacing.x * 15.0f), al.pos.y + (50.0f * scale.y), font_size, RED);
        }
        else {
            DrawText(al.letters[offset].c, al.pos.x + (i * spacing.x * 15.0f), al.pos.y + (50.0f * scale.y), font_size, BLACK);
        }
        offset++;
    }

    for (int i=0; i<6; ++i) {
        const Vector2 char_size = MeasureTextEx(GetFontDefault(), al.letters[offset].c, font_size, 1);

        if (al.letters[offset].used) {
            DrawText(al.letters[offset].c, al.pos.x + (i * spacing.x * 15.0f), al.pos.y + (75.0f * scale.y), font_size, RED);
        }
        else {
            DrawText(al.letters[offset].c, al.pos.x + (i * spacing.x * 15.0f), al.pos.y + (75.0f * scale.y), font_size, BLACK);
        }
        offset++;
    }

    for (int i=0; i<2; ++i) {
        const Vector2 char_size = MeasureTextEx(GetFontDefault(), al.letters[offset].c, font_size, 1);

        if (al.letters[offset].used) {
            DrawText(al.letters[offset].c, al.pos.x + (i * spacing.x * 15.0f), al.pos.y + (100.0f * scale.y), font_size, RED);
        }
        else {
            DrawText(al.letters[offset].c, al.pos.x + (i * spacing.x * 15.0f), al.pos.y + (100.0f * scale.y), font_size, BLACK);
        }
        offset++;
    }
}

void check_quit() {
    game_exited = game_ended ? IsKeyPressed(KEY_Q) : WindowShouldClose();
}

void update_statics() {
    SCREEN_W = GetScreenWidth();
    SCREEN_H = GetScreenHeight();
    scale.x = SCREEN_W / (float)BASE_SCREEN_W;
    scale.y = SCREEN_H / (float)BASE_SCREEN_H;
    box_size = Vector2Multiply(BASE_BOX_SIZE, scale);
    spacing = Vector2Multiply(BASE_SPACING, scale);
    line_thickness = Vector2Multiply(BASE_LINE_THICKNESS, scale);
}

void update_board() {
    const Vector2 mid = Vector2Scale((Vector2){SCREEN_W, SCREEN_H}, 0.5f);
    const Vector2 rec_mid = rect_center_pos(&box_size, &mid);
    grid_size.x = WORD_SIZE * box_size.x + (WORD_SIZE-1) * spacing.x;
    grid_size.y = GUESSES * box_size.y + (GUESSES-1) * spacing.y;
    const Vector2 box_start = {
        (SCREEN_W - grid_size.x) / 2.0f,
        (SCREEN_H - grid_size.y) / 2.0f
    };
    for (size_t i=0; i<GUESSES; ++i) {
        for (size_t j=0; j<WORD_SIZE; ++j) {
            b[i].boxes[j] = (Rectangle){box_start.x + j * (box_size.x + spacing.x), box_start.y + i * (box_size.y + spacing.y), box_size.x, box_size.y};
        }
    }
}

void update_screen_deco() {
    const Vector2 board_tl_pos = {
        b[0].boxes[0].x,
        b[0].boxes[0].y
    };
    const Vector2 board_br_pos = {
        b[GUESSES-1].boxes[WORD_SIZE-1].x + b[GUESSES-1].boxes[WORD_SIZE-1].width,
        b[GUESSES-1].boxes[WORD_SIZE-1].y + b[GUESSES-1].boxes[WORD_SIZE-1].height
    };
    sd.borders[0] = create_line(board_tl_pos, vec_add_x(board_tl_pos, grid_size.x));
    sd.borders[1] = create_line(board_br_pos, vec_sub_y(board_br_pos, grid_size.y));
    sd.borders[2] = create_line(board_br_pos, vec_sub_x(board_br_pos, grid_size.x));
    sd.borders[3] = create_line(board_tl_pos, vec_add_y(board_tl_pos, grid_size.y));
}

void update_available_letters() {
    al.pos = Vector2Multiply(BASE_AVAILABLE_LETTERS_POS, scale);
    al.name_highlight = create_line(vec_sub_y(al.pos, spacing.y), vec_add_x(al.pos, 80.0f * spacing.x));
    
    if (guess_count < 2 || !b[guess_count-2].word.data) return;
    // check the status of them
    for (int i=0; i<ALPHABET_SIZE; ++i) {
        if (!(al.letters[i].used)) {
            // inefficient way but its fine since the size of the board is small
            for (int j=0; j<WORD_SIZE; ++j) {
                if (b[guess_count-2].word.data[j] == al.letters[i].c[0]) {
                    al.letters[i].used = true;
                }
            }
        }
    }
}

void new_game(string *word_list, char *word) {
    game_ended = false;
    memset(word, 0, sizeof(char));
    memcpy(word, pick_random_word(word_list), WORD_SIZE);

    for (size_t i=0; i<GUESSES; ++i) {
        for (size_t j=0; j<WORD_SIZE; ++j) {
            b[i].colors[j] = BLACK;
            b[i].box_trans[j] = 1.0f;
            b[i].box_anim[j] = false;
        }
        b[i].anim_done = false;
        b[i].guessed = false;
    }

    for (size_t i=0; i<ALPHABET_SIZE; ++i) {
        al.letters[i].used = false;
    }

    guess_count = 1;
    is_anim_playing = false;
    anim_idx=0;
    reset_timer(&anim_box_time);
}

int main(void) {
    srand(time(0));
    init_board();
    init_available_letters();
    InitWindow(SCREEN_W, SCREEN_H, "Dummy Wordle");
    SetTargetFPS(60);
    
    string word_list = {0};
    char *word = pick_random_word(&word_list);
    string guess = {
        .data = malloc(WORD_SIZE+1),
        .size = 0
    };
    memset(guess.data, 0, sizeof(char));
    while (!game_exited) {
        // upd
        if (game_ended && IsKeyPressed(KEY_N)) {
            new_game(&word_list, word);
        }
        update_statics();
        update_board();
        update_available_letters();
        update_screen_deco();
        check_quit();
        process_input(&guess);
        if (!game_ended && !is_anim_playing && check_availability(word, &guess, word_list) || guess_count > GUESSES) {
            game_ended = true;
        }
        BeginDrawing();
        ClearBackground(WHITE);
        if (game_ended) end_game(word);
        draw_input(&guess);
        draw_box_anim(&(b[guess_count-2]));
        draw_board();
        draw_available_letters();
        draw_screen_deco();
        EndDrawing();
    }
    free(word);
    free(guess.data);
    for (size_t i=0; i<GUESSES; ++i) {
        free(b[i].word.data);
    }
    CloseWindow();
}

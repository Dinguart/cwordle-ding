#include <stdio.h>
#include <stdlib.h>
#include <raylib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

#define SCREEN_W 540
#define SCREEN_H 360

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
    string word;
    Vector2 word_size;
    Rectangle boxes[WORD_SIZE];
    Color colors[WORD_SIZE];
    float box_trans[WORD_SIZE];
    bool box_anim[WORD_SIZE]; // for each box to draw independently
    bool guessed; // flag to trigger animation
    bool anim_done;
} row;

static row b[GUESSES] = {0};

static float scale_x = 1.0f;
static float scale_y = 1.0f;

static bool is_anim_playing = false;
static bool game_ended = false;
static bool game_exited = false;

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
void init_board() {
    for (int i=0; i<GUESSES; ++i) {
        for (int j=0; j<WORD_SIZE; ++j) {
            b[i].boxes[j] = (Rectangle){
                .x = (SCREEN_W/2) + (36 * j),
                .y = (SCREEN_H/4) + (36 * i),
                .width = 35,
                .height = 35
            };
            b[i].colors[j] = BLACK;
            b[i].box_trans[j] = 1.0f;
            b[i].box_anim[j] = false;
        }
    }
}

// this function picks a random word, and also stores all the words at runtime.
char* pick_random_word(string *word_list) {
    int line = rand() % NUM_LINES;
    FILE *file = fopen("words/words.txt", "r");

    char buf[50];
    int line_count = 0;
    while (fgets(buf, 50, file) && line_count < 502) {
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

bool check_availability(const char *word, string *guess, int *guess_count, const string word_list) {
    if (IsKeyPressed(KEY_ENTER) && check_guess(guess->data, word_list)) {
        if (strcmp(guess->data, word) == 0) {
            // then they won
            int guess_idx = (*guess_count)-1;
            add_guess_to_board(guess, guess_idx);
            for (int i=0; i<WORD_SIZE; ++i) {
                b[guess_idx].colors[i] = GREEN;
            }
            b[guess_idx].guessed = true;            
            (*guess_count)++;
            guess->size = 0;
            printf("You win!\n");
            return true;
        }
        else if (strcmp(guess->data, word) != 0 && (*guess_count) <= GUESSES) {
            int guess_idx = (*guess_count)-1;
            add_guess_to_board(guess, guess_idx);
            for (size_t i=0; i<WORD_SIZE; ++i) {
                for (size_t j=0; j<WORD_SIZE; ++j) {
                    if (i == j && guess->data[i] == word[j]) b[guess_idx].colors[i] = GREEN;
                    else if (i != j && guess->data[i] == word[j] && ColorIsEqual(b[guess_idx].colors[i], BLACK)) b[guess_idx].colors[i] = YELLOW;
                }
            }
            printf("%d\n%d\n", b[guess_idx].guessed, b[guess_idx].anim_done);
            b[guess_idx].guessed = true;
            (*guess_count)++;
            printf("Loser\n");
            memset(guess->data, 0, sizeof(char));
            guess->size = 0;
        }
        else if ((*guess_count) == GUESSES-1) {
            printf("You lose!\n");
            (*guess_count)++;
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
    DrawText(TextFormat("Game finished, the word was... %s\nPress Q to exit.", word), 30, 10, FONT_SIZE, BLACK);
}

void draw_input(string *guess) {
    for (size_t i=0; i<guess->size; ++i) {
        char c = guess->data[i];
        const char str_c[2] = { c, '\0' };
        const Vector2 char_size = MeasureTextEx(GetFontDefault(), str_c, FONT_SIZE, 1);
        DrawLine(30*(i+1) - (char_size.x/2), (SCREEN_H/2) + (char_size.y / 2), 30*(i+1) + (char_size.x/2), (SCREEN_H/2) + (char_size.y / 2), BLACK);
        DrawText(str_c, 30*(i+1) - (char_size.x/2), (SCREEN_H/2) - (char_size.y / 2), FONT_SIZE, BLACK);
    }
    
}

void draw_guess_text(int outer_idx, int inner_idx) {
    char c = b[outer_idx].word.data[inner_idx];
    const char str_c[2] = { c, '\0' };
    DrawText(str_c, ((SCREEN_W/2) + (36 * (inner_idx+1)) - (b[outer_idx].word_size.y)), ((SCREEN_H/4) + (36 * (outer_idx+1)) - (b[outer_idx].word_size.x / 2)), FONT_SIZE, b[outer_idx].colors[inner_idx]);
}

void draw_board(int *guess_count) {
    for (int i=1; i<GUESSES+1; ++i) {
        for (int j=1; j<WORD_SIZE+1; ++j) {
            if (i >= (*guess_count) || !b[i-1].box_anim[j-1]) DrawRectangleRec(b[i-1].boxes[j-1], BLACK); // timer to slowly fade out
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
        static int idx=0;
        static timer box_time = {0};

        if (idx >= WORD_SIZE) {
            idx=0;
            r->anim_done = true;
            is_anim_playing = false;
            return;
        }

        // locals for each box
        Color box_color = BLACK;
        if (!is_timer_active(&box_time) && !was_timer_active(&box_time)) {
            start_timer(&box_time, 0.2f);
        }
        update_timer(&box_time);
        printf("Working..\n");
        // logic
        box_color = Fade(box_color, r->box_trans[idx]);
        DrawRectangleRec(r->boxes[idx], box_color);
        r->box_trans[idx] -= 0.2f;
        if (!is_timer_active(&box_time)) {
            // if timer finished, display the character.)
            r->box_anim[idx] = true;
            idx++;
            reset_timer(&box_time);
        }
    }
}

void check_quit() {
    game_exited = game_ended ? IsKeyPressed(KEY_Q) : WindowShouldClose();
}

int main(void) {
    srand(time(0));
    init_board();
    InitWindow(SCREEN_W, SCREEN_H, "Dummy Wordle");
    SetTargetFPS(60);
    
    string word_list = {0};
    char *word = pick_random_word(&word_list);
    int guess_count = 1;
    string guess = {
        .data = malloc(WORD_SIZE+1),
        .size = 0
    };
    memset(guess.data, 0, sizeof(char));
    while (!game_exited) {
        // upd
        check_quit();
        process_input(&guess);
        if (!game_ended && !is_anim_playing && check_availability(word, &guess, &guess_count, word_list) || guess_count > GUESSES) {
            game_ended = true;
        }
        BeginDrawing();
        ClearBackground(WHITE);
        if (game_ended) end_game(word);
        draw_input(&guess);
        draw_box_anim(&(b[guess_count-2]));
        draw_board(&guess_count);
        EndDrawing();
    }
    free(word);
    free(guess.data);
    for (size_t i=0; i<GUESSES; ++i) {
        free(b[i].word.data);
    }
    CloseWindow();
}

#include "tangram.h"
#include "shader.h"
#include "game.h"

// ENGINE CODE

TangramResult tangram_test_pointer(void *ptr)
{
    if (ptr != NULL)
        return TANGRAM_OK;
    fprintf(stderr, "Pointer \"%s\" test FAILED! %s\n", ptr, SDL_GetError());
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Pointer \"%s\" test FAILED! %s\n", ptr, SDL_GetError());

    return TANGRAM_ERR;
}

void init_screen()
{
    size_t size = width * height * sizeof(unsigned int);
    tangram.screen = malloc(size);
    memset(tangram.screen, 0, size);
}

void init_clock(TangramClock *clock)
{
    clock->now = SDL_GetPerformanceCounter();
    clock->last = 0;
    clock->dt = 0;
}

void tick_clock(TangramClock *clock)
{
    clock->last = clock->now;
    clock->now = SDL_GetPerformanceCounter();

    clock->dt = (double)((clock->now - clock->last) / (double)SDL_GetPerformanceFrequency());
    game_state->ticks++;
}

bool key_is_down(SDL_KeyCode key)
{
    return tangram.keystate[SDL_GetScancodeFromKey(key)];
}

bool key_is_pressed(SDL_KeyCode key)
{
    if (key_is_down(key) && !tangram.pressed.keys[key])
    {
        tangram.pressed.keys[key] = true;
        return true;
    }
    return false;
}

bool key_is_down_buffered(SDL_KeyCode key)
{
    return key_is_down(key) && every_n_frames(game_state->tpu);
}

bool key_is_up(SDL_KeyCode key)
{
    return !tangram.keystate[SDL_GetScancodeFromKey(key)];
}

TangramTexture *new_texture(const char *filename)
{
    TangramTexture *t = malloc(sizeof(TangramTexture));
    t->data = stbi_load(filename, &t->w, &t->h, &t->channels, STBI_rgb_alpha);
    if (t->data)
    {
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Created texture!");
        return t;
    }
    return NULL;
}

void free_textures()
{
    stbi_image_free(tangram.textures.background);
    stbi_image_free(tangram.textures.spritesheet);
}

void draw_clear(unsigned int color)
{
    unsigned int *p = tangram.screen;
    unsigned int *end = tangram.screen + width * height;
    unsigned int c = RGB_TO_BGR(color);
    while (p < end)
        *p++ = c;
}

void draw_pixel(int x, int y, unsigned int color)
{
    if (x >= 0 && x < width && y >= 0 && y < height)
    {
        unsigned int *pixel = &tangram.screen[((height - 1 - y) * width) + x];
        if ((color & 0xFF000000) != 0xFF000000)
            *pixel = RGB_TO_BGR(color);
    }
}

void draw_line(Point from, Point to, unsigned int color)
{
    // Bresenham's line algorithm
    float dx = fabs(to.x - from.x);
    float dy = fabs(to.y - from.y);
    float sx = from.x < to.x ? 1.0f : -1.0f;
    float sy = from.y < to.y ? 1.0f : -1.0f;
    float err = dx - dy;

    for (int i = 0; i <= fmaxf(dx, dy); i++)
    {
        draw_pixel((int)(from.x + 0.5f), (int)(from.y + 0.5f), color);
        if (from.x == to.x && from.y == to.y)
            break;
        int e2 = 2.0f * err;
        if (e2 > -dy)
        {
            err -= dy;
            from.x += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            from.y += sy;
        }
    }
}

void draw_rectangle(Point from, Point to, unsigned int color, bool outline)
{
    const Point A = (Point){from.x, from.y};
    const Point B = (Point){to.x, from.y};
    const Point C = (Point){to.x, to.y};
    const Point D = (Point){from.x, to.y};
    if (outline)
    {
        draw_line(A, B, color);
        draw_line(B, C, color);
        draw_line(C, D, color);
        draw_line(D, A, color);
    }
    else
    {
        for (int rx = from.x; rx < to.x; rx++)
            for (int ry = from.y; ry < to.y; ry++)
                draw_pixel(rx, ry, color);
    }
}

void draw_texture(TangramTexture *texture, Point pos, Point uv, Point size, float scale, unsigned int blend)
{
    // the image will pretend to have 3 channels but all textures are loaded with 4
    int channels = 4;

    // Extract blend color channels
    unsigned char blend_r = (blend >> 16) & 0xFF;
    unsigned char blend_g = (blend >> 8) & 0xFF;
    unsigned char blend_b = blend & 0xFF;
    float factor = ((blend >> 24) & 0xFF) / 255.0f; // Extract alpha as a blend factor

    for (int tx = 0; tx < size.x; tx++)
        for (int ty = 0; ty < size.y; ty++)
        {
            int dtx = (int)(uv.x + (float)tx);
            int dty = (int)(uv.y + (float)ty);

            int pixel = (dty * texture->w + dtx) * channels;
            unsigned char r = texture->data[pixel];
            unsigned char g = texture->data[pixel + 1];
            unsigned char b = texture->data[pixel + 2];
            unsigned char a = texture->data[pixel + 3];

            // Apply blending to the color
            r = (unsigned char)(r * (1.0f - factor) + blend_r * factor);
            g = (unsigned char)(g * (1.0f - factor) + blend_g * factor);
            b = (unsigned char)(b * (1.0f - factor) + blend_b * factor);

            unsigned int color = 0;
            color |= (r << 16);
            color |= (g << 8);
            color |= b;

            draw_pixel(pos.x + tx, pos.y + ty, color);
    }
}

HSTREAM new_sound(const char *filename)
{
    if (filename != NULL)
    {
        return BASS_StreamCreateFile(0, filename, 0, 0, 0);
    }
}

void init_sounds()
{
    tangram.sfx[0] = new_sound("data/sfx/SEB_instal.wav");
    for (int i = 1; i < 8; i++)
    {
        char filename[24];
        sprintf(filename, "data/sfx/SEB_mino%d.wav", i);
        tangram.sfx[i] = BASS_StreamCreateFile(0, filename, 0, 0, 0);
    }
    tangram.sfx[8] = new_sound("data/sfx/SEB_disappear.wav");
    tangram.sfx[9] = new_sound("data/sfx/SEB_fall.wav");
    tangram.sfx[10] = new_sound("data/sfx/SEB_fixa.wav");
    tangram.sfx[11] = new_sound("data/sfx/SEB_prerotate.wav");
}

void play_sound(int id, float volume)
{
    HSTREAM sfx = tangram.sfx[id];
    if (BASS_ChannelIsActive(sfx) == BASS_ACTIVE_PLAYING)
        if (!BASS_ChannelStop(sfx))
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not stop sound #%d: %d", id, BASS_ErrorGetCode());
    BASS_ChannelSetAttribute(sfx, BASS_ATTRIB_VOL, volume);
    if (!BASS_ChannelPlay(sfx, 0))
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not play sound #%d: %d", id, BASS_ErrorGetCode());
}

void free_sounds()
{
    for (int i = 0; i < SOUND_AMOUNT; i++)
        BASS_StreamFree(tangram.sfx[i]);
}

// GAME CODE

void init_queue()
{
    for (int i = 0; i < QUEUE_SIZE; i++)
    {
        unsigned int index = ((unsigned int)((genrand_real3() * (INT32_MAX - 1) + SDL_GetTicks() + 0xb297afff)) % 7) + 1;
        while (index_in_queue(index))
            index = ((unsigned int)((genrand_real3() * (INT32_MAX - 1) + SDL_GetTicks() + 0x99128bea)) % 7) + 1;
        game_state->queue[i] = index;
    }
}

bool index_in_queue(int idx)
{
    for (int i = 0; i < QUEUE_SIZE; i++)
    {
        if (game_state->queue[i] == idx)
            return true;
    }
    return false;
}

unsigned int new_index(unsigned int seed)
{
    unsigned int n;
    do
    {
        n = ((unsigned int)((genrand_real3() * (INT32_MAX - 1)) + seed) % 7) + 1;
    } while (index_in_queue(n));
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "New index: %u", n);
    return n;
}

void update_queue(int idx)
{
    for (int i = 0; i < QUEUE_SIZE; i++)
        game_state->queue[i] = game_state->queue[i + 1];
    game_state->queue[QUEUE_SIZE - 1] = idx;
}

void init_game_state()
{
    game_state = malloc(sizeof(GameState));
    size_t size = BOARD_WIDTH * BOARD_HEIGHT * sizeof(unsigned char);
    game_state->board = malloc(size);
    memset(game_state->board, 0, size);

    init_queue();

    game_state->piece = new_piece(-1);
    game_state->game_over = false;
    game_state->ticks = 0;
    game_state->gravity = 1;
    game_state->ftr = fps;
    game_state->tpu = 1;
    game_state->level = 0;
    game_state->score = 0;
    game_state->das = DAS_FRAMES;
    game_state->are = game_state->ticks;
    game_state->lockticks = game_state->ticks;
}

bool every_n_frames(unsigned int frames)
{
    return game_state->ticks % frames == 0;
}

void draw_board()
{
    float X_OFFSET = floor(width * 0.5 - BOARD_WIDTH * CELL_SIZE * 0.5);
    float Y_OFFSET = floor(height * 0.5 - BOARD_HEIGHT * CELL_SIZE * 0.5);
    // Draw board pane
    draw_rectangle(
        (Point){X_OFFSET, Y_OFFSET},
        (Point){X_OFFSET + BOARD_WIDTH * CELL_SIZE, Y_OFFSET + BOARD_HEIGHT * CELL_SIZE},
        0, false);
    // Draw piece
    if (game_state->piece != NULL && !game_state->piece->locked)
    {
        Piece *p = game_state->piece;
        for (int b = 0; b < 4; b++)
        {
            unsigned int bx = p->blocks[b].x;
            unsigned int by = p->blocks[b].y;
            unsigned int color = PIECE_COLORS[p->blocks[b].piece];

            draw_texture(
                tangram.textures.spritesheet,
                (Point){X_OFFSET + bx * CELL_SIZE, Y_OFFSET + by * CELL_SIZE},
                (Point){(float)p->blocks[b].piece * CELL_SIZE, 0.0f},
                (Point){CELL_SIZE, CELL_SIZE},
                1.0f, 0xFFFFFF);
        }
    }
    // Draw queue pane
    draw_rectangle(
        (Point){X_OFFSET + BOARD_WIDTH * CELL_SIZE + 32, Y_OFFSET},
        (Point){X_OFFSET + BOARD_WIDTH * CELL_SIZE + 128, Y_OFFSET + BOARD_HEIGHT * CELL_SIZE},
        0xFFFFFF, true);
    // Draw piece queue
    for (int q = 1; q < QUEUE_SIZE; q++)
    {
        Piece *next = new_piece(game_state->queue[q]);
        for (int n = 0; n < 4; n++)
        {
            unsigned int nx = next->blocks[n].x;
            unsigned int ny = next->blocks[n].y;
            unsigned int color = PIECE_COLORS[next->blocks[n].piece];

            Point draw_position = (Point){
                X_OFFSET + (BOARD_WIDTH * CELL_SIZE) + nx * CELL_SIZE,
                Y_OFFSET + (q - 1) * BOARD_HEIGHT * 3 + ny * CELL_SIZE + CELL_SIZE};

            draw_texture(
                tangram.textures.spritesheet,
                draw_position,
                (Point){(float)next->blocks[n].piece * CELL_SIZE, 0.0f},
                (Point){CELL_SIZE, CELL_SIZE},
                1.0f, 0xFFFFFF);
        }
        free(next);
    }
    // Draw board
    for (int bx = 0; bx < BOARD_WIDTH; bx++)
    {
        for (int by = 0; by < BOARD_HEIGHT; by++)
        {
            unsigned char *piece = &game_state->board[by * BOARD_WIDTH + bx];
            if (*piece != PIECE_NONE)
            {
                draw_texture(
                    tangram.textures.spritesheet,
                    (Point){X_OFFSET + bx * CELL_SIZE, Y_OFFSET + by * CELL_SIZE},
                    (Point){*piece * CELL_SIZE, 0.0f},
                    (Point){CELL_SIZE, CELL_SIZE},
                    1.0f, 0xE0000000);
                bool top_free = game_state->board[by * BOARD_WIDTH + bx - BOARD_WIDTH] == PIECE_NONE;
                bool left_free = bx > 0 && game_state->board[by * BOARD_WIDTH + bx - 1] == PIECE_NONE;
                bool right_free = bx < BOARD_WIDTH && game_state->board[by * BOARD_WIDTH + bx + 1] == PIECE_NONE;
                bool bottom_free = by < BOARD_HEIGHT && game_state->board[by * BOARD_WIDTH + bx + BOARD_WIDTH] == PIECE_NONE;
                if (top_free)
                {
                    draw_line(
                        (Point){X_OFFSET + bx * CELL_SIZE,
                                Y_OFFSET + by * CELL_SIZE},
                        (Point){X_OFFSET + bx * CELL_SIZE + CELL_SIZE,
                                Y_OFFSET + by * CELL_SIZE},
                        0xFFFFFF);
                }
                if (left_free)
                {
                    draw_line(
                        (Point){X_OFFSET + bx * CELL_SIZE,
                                Y_OFFSET + by * CELL_SIZE},
                        (Point){X_OFFSET + bx * CELL_SIZE,
                                Y_OFFSET + by * CELL_SIZE + CELL_SIZE},
                        0xFFFFFF);
                }
                if (right_free)
                {
                    draw_line(
                        (Point){X_OFFSET + bx * CELL_SIZE + CELL_SIZE,
                                Y_OFFSET + by * CELL_SIZE},
                        (Point){X_OFFSET + bx * CELL_SIZE + CELL_SIZE,
                                Y_OFFSET + by * CELL_SIZE + CELL_SIZE},
                        0xFFFFFF);
                }
                if (bottom_free)
                {
                    draw_line(
                        (Point){X_OFFSET + bx * CELL_SIZE,
                                Y_OFFSET + by * CELL_SIZE + CELL_SIZE},
                        (Point){X_OFFSET + bx * CELL_SIZE + CELL_SIZE,
                                Y_OFFSET + by * CELL_SIZE + CELL_SIZE},
                        0xFFFFFF);
                }
            }
        }
    }
    // Draw border stroke
    draw_rectangle(
        (Point){X_OFFSET, Y_OFFSET},
        (Point){X_OFFSET + BOARD_WIDTH * CELL_SIZE, Y_OFFSET + BOARD_HEIGHT * CELL_SIZE},
        0xFFFFFF, true);
}

Block new_block(unsigned short data)
{
    Block b;
    b.unused = data >> 15;
    b.piece = (data & 0x7000) >> 12;
    b.rotation = (data & 0xC00) >> 10;
    b.y = (data & 0x1F0) >> 5;
    b.x = data & 0x1F;
    return b;
}

Piece *new_piece(int idx)
{
    Piece *p;
    p = malloc(sizeof(Piece));
    p->locked = 0;
    p->coll = 0;

    PieceIndex index = idx;
    int initial_dir = 0;

    if (index == -1)
    {
        index = new_index(SDL_GetTicks() + 0xb297afff);
        update_queue(index);
        index = game_state->queue[0];
        // game_state->dhf = 0;
        if (tangram.keystate != NULL)
        {
            if (key_is_down(SDLK_z))
            {
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Detected counterclockwise IRS!");
                initial_dir = -1;
            }
            else if (key_is_down(SDLK_x))
            {
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Detected clockwise IRS!");
                initial_dir = 1;
            }
        }
    }

    switch (index)
    {
    case PIECE_NONE:
        return NULL;
    case PIECE_I:
        p->blocks[0] = new_block(0x1003);
        p->blocks[1] = new_block(0x1004);
        p->blocks[2] = new_block(0x1005);
        p->blocks[3] = new_block(0x1006);
        break;
    case PIECE_J:
        p->blocks[0] = new_block(0x2004);
        p->blocks[1] = new_block(0x2024);
        p->blocks[2] = new_block(0x2025);
        p->blocks[3] = new_block(0x2026);
        break;
    case PIECE_L:
        p->blocks[0] = new_block(0x3006);
        p->blocks[1] = new_block(0x3024);
        p->blocks[2] = new_block(0x3025);
        p->blocks[3] = new_block(0x3026);
        break;
    case PIECE_O:
        p->blocks[0] = new_block(0x4004);
        p->blocks[1] = new_block(0x4005);
        p->blocks[2] = new_block(0x4024);
        p->blocks[3] = new_block(0x4025);
        break;
    case PIECE_S:
        p->blocks[0] = new_block(0x5005);
        p->blocks[1] = new_block(0x5006);
        p->blocks[2] = new_block(0x5024);
        p->blocks[3] = new_block(0x5025);
        break;
    case PIECE_Z:
        p->blocks[0] = new_block(0x6004);
        p->blocks[1] = new_block(0x6005);
        p->blocks[2] = new_block(0x6025);
        p->blocks[3] = new_block(0x6026);
        break;
    case PIECE_T:
        p->blocks[0] = new_block(0x7005);
        p->blocks[1] = new_block(0x7024);
        p->blocks[2] = new_block(0x7025);
        p->blocks[3] = new_block(0x7026);
        break;
    default:
        break;
    }

    if (idx == -1)
    {
        if (initial_dir != 0)
            play_sound(SOUND_IRS, 0.9f);

        play_sound(game_state->queue[1], 1.0f);

        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Spawned piece #%u", index);
        SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "The queue now is: [%d, %d, %d, %d, %d]",
                    game_state->queue[0],
                    game_state->queue[1],
                    game_state->queue[2],
                    game_state->queue[3],
                    game_state->queue[4]);
    }

    return p;
}

unsigned int piece_range(Piece *piece, PieceRange range)
{
    unsigned int a = piece->blocks[0].y;
    unsigned int b = piece->blocks[1].y;
    for (int i = 0; i < 4; i++)
    {
        int py = piece->blocks[i].y;
        if (py < a)
            a = py;
        if (py > b)
            b = py;
    }

    switch (range)
    {
    case HIGHEST_BLOCK:
        return a;
    case LOWEST_BLOCK:
        return b;
    case FULL_RANGE:
        return a - b;
    }
}

void move_piece(Piece *piece, int x, int y)
{
    for (int i = 0; i < 4; i++)
    {
        int dx = piece->blocks[i].x + x;
        int dy = piece->blocks[i].y + y;
        if (dy >= BOARD_HEIGHT || (game_state->board[dy * BOARD_WIDTH + dx] != PIECE_NONE && x == 0))
        {
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Skipping lock delay...");
            play_sound(SOUND_PIECECOLLIDE, 0.9f);
            lock_piece(piece);
            return;
        }
        if (
            dx < 0 || dx >= BOARD_WIDTH ||
            game_state->board[dy * BOARD_WIDTH + dx] != PIECE_NONE)
            return;
    }

    // Move the entire piece when all blocks pass the check
    for (int i = 0; i < 4; i++)
    {
        piece->blocks[i].x += x;
        piece->blocks[i].y += y;
    }

    unsigned int ly = piece_range(piece, LOWEST_BLOCK);
    for (int i = 0; i < 4; i++)
    {
        if (piece->blocks[i].y == ly)
        {
            unsigned int cell = game_state->board[(piece->blocks[i].y + 1) * BOARD_WIDTH + piece->blocks[i].x];
            if (cell != PIECE_NONE && !piece->coll)
            {
                piece->coll = true;
                SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Piece is locked now! Waiting for lock delay to run out...");
                play_sound(SOUND_PIECECOLLIDE, 0.9f);
                game_state->lockticks = game_state->ticks;
            }
            if (cell == PIECE_NONE && piece->coll)
                piece->coll = false;
            break;
        }
    }
}

void rotate_piece(Piece *piece, int direction)
{
    // O doesn't rotate!
    if (piece->blocks[0].piece == PIECE_O)
        return;

    int prev_r = piece->blocks[0].rotation;
    int r = piece->blocks[0].rotation + direction;
    if (r < ROT_0)
        r = ROT_270;
    else if (r > ROT_270)
        r = ROT_0;

    Block pivot;
    bool ok = true;
    unsigned int from_center = 1;

    unsigned int rx[4];
    unsigned int ry[4];
    for (int i = 0; i < 4; i++)
    {
        rx[i] = pivot.x - from_center + ROTATION_DATA[piece->blocks[i].piece - 1][r][i][0];
        ry[i] = pivot.y - from_center + ROTATION_DATA[piece->blocks[i].piece - 1][r][i][1];
    }

    if (piece->blocks[0].piece == PIECE_I)
        pivot = piece->blocks[1];
    else if (piece->blocks[0].piece == PIECE_S)
        pivot = piece->blocks[3];
    else
        pivot = piece->blocks[2];

    int offset_x = 0;
    int offset_y = 0;
    for (int i = 0; i < 4; i++)
    {
        piece->blocks[i].rotation = r;
        rx[i] = pivot.x - 1 + ROTATION_DATA[piece->blocks[i].piece - 1][piece->blocks[i].rotation][i][0];
        ry[i] = pivot.y - 1 + ROTATION_DATA[piece->blocks[i].piece - 1][piece->blocks[i].rotation][i][1];
        if (
            rx[i] >= 0 && ry[i] >= 0 && rx[i] < BOARD_WIDTH && ry[i] < BOARD_HEIGHT)
        {
            /*
            if (rx[i] != PIECE_NONE || ry[i] != PIECE_NONE)
            {
                // wall kicks
                int p = piece->blocks[i].piece == PIECE_I;
                int r = piece->blocks[i].rotation;

                for (int n = 0; n < WALLKICK_TESTS; n++)
                {
                    int dir = direction < 0;
                    int test_x = WALLKICK_DATA[p][r][dir][n][0];
                    int test_y = WALLKICK_DATA[p][r][dir][n][1];

                    ok = check_move(piece, test_x, test_y);
                    if (!ok)
                        continue;

                    offset_x = test_x;
                    offset_y = test_y;
                    SDL_LogInfo(
                        SDL_LOG_CATEGORY_APPLICATION,
                        "Wall kick test #%d passed for piece #%d when going from rotation %d to %d!",
                        n + 1, piece->blocks[i].piece, prev_r, r
                    );
                    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "The new offset is (%d, %d)", offset_x, offset_y);
                    break;
                }
            }
            */
        }
        else
        {
            ok = false;
            break;
        }
    }
    if (ok)
    {
        for (int i = 0; i < 4; i++)
        {
            piece->blocks[i].x = rx[i];
            piece->blocks[i].y = ry[i];
        }
    }
}

void lock_piece(Piece *piece)
{
    piece->locked = true;
    for (int i = 0; i < 4; i++)
    {
        int px = piece->blocks[i].x;
        int py = piece->blocks[i].y;
        game_state->board[py * BOARD_WIDTH + px] = piece->blocks[i].piece;
    }
    play_sound(SOUND_PIECELOCK, 1.0f);
    unsigned int y1 = piece_range(piece, HIGHEST_BLOCK);
    unsigned int y2 = piece_range(piece, LOWEST_BLOCK);
    int diff = y2 - y1 + 1;
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Checking lines %d to %d", y1, y2);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Checking %d inside check_lines", diff);

    for (int y = y1; y <= y2; y++)
        check_line(y);

    game_state->level++;
    game_state->are = game_state->ticks;
}

void check_line(unsigned int y)
{
    if (y <= 0)
    {
        game_state->game_over = true;
        BASS_ChannelStop(tangram.music);
        return;
    }

    for (int x = 0; x < BOARD_WIDTH; x++)
    {
        if (game_state->board[y * BOARD_WIDTH + x] == PIECE_NONE)
            return;
    }

    play_sound(SOUND_DISAPPEAR, 0.7f);
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Line %d has been cleared!!", y);
    for (int x = 0; x < BOARD_WIDTH; x++)
        game_state->board[y * BOARD_WIDTH + x] = PIECE_NONE;

    for (int dy = y; dy > 0; dy--)
        for (int dx = 0; dx < BOARD_WIDTH; dx++)
            game_state->board[dy * BOARD_WIDTH + dx] = game_state->board[dy * BOARD_WIDTH + dx - BOARD_WIDTH];

    game_state->score += 100 + game_state->level * 2;
    game_state->level++;
}

void restart_game()
{
    BASS_ChannelPlay(tangram.music, 1);
    free(game_state);
    init_game_state();
}

// ENGINE EVENTS

static int tangram_event_setup()
{
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
        return false;
    }

    tangram.window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
    if (!tangram.window)
    {
        fprintf(stderr, "Failed to create Tangram window: %s\n", SDL_GetError());
        return false;
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, 0);
    SDL_SetWindowMinimumSize(tangram.window, 640, 480);

    init_screen();
    tangram.textures.background = new_texture("data/img/background.jpg");
    tangram.textures.spritesheet = new_texture("data/img/texturemap.png");

    // GL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

    SDL_EnableScreenSaver();
    atexit(SDL_Quit);

    tangram.gl.context = SDL_GL_CreateContext(tangram.window);
    SDL_GL_MakeCurrent(tangram.window, tangram.gl.context);
    if (gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress) <= 0)
    {
        fprintf(stderr, "Failed to create GL context: %s\n", SDL_GetError());
        return false;
    }

    tangram.gl.program = glCreateProgram();

    GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_src, NULL);
    glCompileShader(vertex_shader);

    GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_src, NULL);
    glCompileShader(fragment_shader);

    glAttachShader(tangram.gl.program, vertex_shader);
    glAttachShader(tangram.gl.program, fragment_shader);

    glLinkProgram(tangram.gl.program);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);

    const Vertex vertices[] = {
        {{-1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}},
        {{1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}},
        {{-1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
        {{1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},
    };

    glGenVertexArrays(1, &tangram.gl.VAO);
    glGenBuffers(1, &tangram.gl.VBO);

    glBindVertexArray(tangram.gl.VAO);

    glBindBuffer(GL_ARRAY_BUFFER, tangram.gl.VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, pos));
    glEnableVertexAttribArray(0);
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, color));
    glEnableVertexAttribArray(1);
    // Texture coordinate attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)offsetof(Vertex, tex_coord));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);

    // Foreground
    glGenTextures(1, &tangram.gl.fg_texture_id);
    glBindTexture(GL_TEXTURE_2D, tangram.gl.fg_texture_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tangram.screen);
    glGenerateMipmap(GL_TEXTURE_2D);

    // Background
    glGenTextures(1, &tangram.gl.bg_texture_id);
    glBindTexture(GL_TEXTURE_2D, tangram.gl.bg_texture_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tangram.textures.background->w, tangram.textures.background->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tangram.textures.background->data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glUseProgram(tangram.gl.program);
    glUniform1i(glGetUniformLocation(tangram.gl.program, "fgTex"), 0);
    glUniform1i(glGetUniformLocation(tangram.gl.program, "bgTex"), 1);

    SDL_SysWMinfo wmi;
    SDL_VERSION(&wmi.version);
    SDL_GetWindowWMInfo(tangram.window, &wmi);
    HWND handle = wmi.info.win.window;

    if (!BASS_Init(-1, 48000, BASS_DEVICE_DEFAULT, handle, NULL))
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to initialize BASS: %d\n", BASS_ErrorGetCode());
        return false;
    }

    init_sounds();

    tangram.music = BASS_MusicLoad(0, "data/music/molrevenge.mod", 0, 0, BASS_SAMPLE_LOOP | BASS_MUSIC_NONINTER, 0);
    if (!tangram.music)
    {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to load music: %d\n", BASS_ErrorGetCode());
        return false;
    }

    init_genrand(SDL_GetTicks() + rand());
    init_clock(&tangram.clock);
    init_game_state();

    BASS_ChannelSetAttribute(tangram.music, BASS_ATTRIB_VOL, 0.7f);
    BASS_ChannelPlay(tangram.music, 1);

    return true;
}

static void tangram_event_update()
{
    tangram.keystate = SDL_GetKeyboardState(NULL);
    tick_clock(&tangram.clock);

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_KEYDOWN:
            SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Key pressed: %u\n", event.key.keysym.sym);
            if (key_is_down(SDLK_ESCAPE))
                tangram.running = false;
            break;
        case SDL_KEYUP:
            tangram.pressed.keys[event.key.keysym.sym] = false;
            break;
        case SDL_QUIT:
            tangram.running = false;
            break;
        default:
            break;
        }
    }

    int input_h = (int)key_is_down(SDLK_RIGHT) - (int)key_is_down(SDLK_LEFT);
    game_state->dhf = input_h != 0 ? game_state->dhf + 1 : 0;

    if (game_state->are == 0 || game_state->ticks > game_state->are + ARE_FRAMES && !game_state->piece->locked)
    {
        if (key_is_up(SDLK_DOWN) && (game_state->dhf == 1 || game_state->dhf >= game_state->das))
            move_piece(game_state->piece, input_h, 0);
        if (key_is_pressed(SDLK_z))
            rotate_piece(game_state->piece, -1);
        if (key_is_pressed(SDLK_x))
            rotate_piece(game_state->piece, 1);
        if (key_is_down_buffered(SDLK_DOWN) || (every_n_frames(game_state->ftr) && !game_state->piece->coll))
        {
            if (!game_state->piece->coll)
                move_piece(game_state->piece, 0, game_state->gravity);
            else
                lock_piece(game_state->piece);
        }

        if (!game_state->piece->locked && game_state->piece->coll && game_state->ticks > game_state->lockticks + LOCK_DELAY)
            lock_piece(game_state->piece);
    }
    else if (
        game_state->ticks > game_state->are + ARE_FRAMES &&
        game_state->piece->locked &&
        !game_state->game_over)
    {
        if (game_state->ftr > 4)
            game_state->ftr = fps - game_state->level * 0.25;
        game_state->piece = new_piece(-1);
    }

    if (key_is_pressed(SDLK_r) && game_state->game_over)
    {
        restart_game();
    }

    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "Δt = %f (%.2f/%u fps)\n", tangram.clock.dt, (float)fps / (tangram.clock.dt * (float)fps), fps);
    float t = SDL_GetTicks() * 0.001f;
    glUniform1f(glGetUniformLocation(tangram.gl.program, "iTime"), t);

    int ww, wh;
    SDL_GetWindowSizeInPixels(tangram.window, &ww, &wh);
    glUniform3f(glGetUniformLocation(tangram.gl.program, "iResolution"), (float)ww, (float)wh, 0.f);

    char new_title[48];
    sprintf(new_title, "%s | Level %d - Score: %d", title, game_state->level, game_state->score);
    SDL_SetWindowTitle(tangram.window, new_title);
}

static void tangram_event_render()
{
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_DST_ALPHA);
    int ww, wh;
    SDL_GetWindowSize(tangram.window, &ww, &wh);
    glViewport(0, 0, ww, wh);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

    draw_clear(0);
    draw_board();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tangram.gl.fg_texture_id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, tangram.screen);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tangram.gl.bg_texture_id);

    glBindVertexArray(tangram.gl.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, tangram.gl.VBO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    SDL_GL_SwapWindow(tangram.window);
}

static void tangram_event_exit()
{
    free_sounds();
    BASS_MusicFree(tangram.music);
    BASS_Free();
    glDeleteVertexArrays(1, &tangram.gl.VAO);
    glDeleteBuffers(1, &tangram.gl.VBO);
    glDeleteProgram(tangram.gl.program);
    glDeleteTextures(1, &tangram.gl.fg_texture_id);
    free_textures();
    SDL_GL_DeleteContext(tangram.gl.context);
    SDL_DestroyWindow(tangram.window);
    SDL_Quit();
}

int main(int argc, char *argv[])
{
    tangram.running = tangram_event_setup();

    while (tangram.running)
    {
        tangram_event_update();
        tangram_event_render();

        // This limits the FPS so your CPU doesn't burst out in flames.
        SDL_Delay(1000 / fps - tangram.clock.dt);
    }

    tangram_event_exit();

    return 0;
}

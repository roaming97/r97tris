#ifndef TANGRAM_HEADER
#define TANGRAM_HEADER

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#ifdef __TINYC__
#undef main
#endif
#include <glad/gl.h>
#include <bass/bass.h>
#define STBI_NO_SIMD
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "include/mt19937ar.h"

// Engine macros

#define RGB_TO_BGR(c) ((c & 0xFF) << 16) | (c & 0xFF00) | ((c & 0xFF0000) >> 16)
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define sign(x) ((x < 0) ? -1 : 1)

// Game constants

static const unsigned int width = 640;
static const unsigned int height = 480;
static const unsigned int fps = 60;
static const unsigned char *title = "Tetris";

// Game loop events

// This event only runs once, it configures and initializes Tangram. Returns `true` if setup is successful, `false` otherwise.
static int tangram_event_setup();
// This event runs every frame of the application's lifetime, it processes input, updates engine variables and components.
static void tangram_event_update();
// This event runs every frame of the application's lifetime, it draws to the screen and processes graphics.
static void tangram_event_render();
// This event only runs once at the end of the application's lifetime, it makes sure to deinitialize and free everything gracefully.
static void tangram_event_exit();

void init_screen();

typedef struct TangramClock
{
    Uint64 now;
    Uint64 last;
    double dt;
} TangramClock;

void init_clock(TangramClock *clock);
void tick_clock(TangramClock *clock);

typedef struct KeyboardMap
{
    int keys[322];
} KeyboardMap;

bool key_is_pressed(SDL_KeyCode key);
bool key_is_down_buffered(SDL_KeyCode key);
bool key_is_up(SDL_KeyCode key);

typedef struct Vertex
{
  float pos[3];
  float color[3];
  float tex_coord[2];
} Vertex;

typedef struct Point
{
	float x, y;
} Point;



typedef struct TangramTexture
{
    unsigned char *data;
    int w;
    int h;
    int channels;
} TangramTexture;

TangramTexture *new_texture(const char *filename);
void free_textures();

typedef struct TextureMap
{
    TangramTexture *background;
    TangramTexture *spritesheet;
} TextureMap;

void draw_clear(unsigned int color);
void draw_pixel(int x, int y, unsigned int color);
void draw_line(Point from, Point to, unsigned int color);
void draw_rectangle(Point from, Point to, unsigned int color, bool outline);
void draw_texture(TangramTexture *texture, Point pos, Point uv, Point size, float scale, unsigned int blend);

typedef struct TangramGL
{
    SDL_GLContext context;
    GLuint program;
    GLuint VAO, VBO;
    GLuint fg_texture_id;
    GLuint bg_texture_id;
} TangramGL;

#define SOUND_AMOUNT 12

HSTREAM new_sound(const char *filename);
void init_sounds();
void play_sound(int id, float volume);
void free_sounds();

/// The main Tangram instance.
///
/// This manager holds the window, GL context, application states, etc.
struct TangramManager
{
    SDL_Window *window;
    TangramGL gl;
    TangramClock clock;
    const unsigned char *keystate;
    KeyboardMap pressed;
    unsigned int *screen;
    TextureMap textures;
    HMUSIC music;
    HSTREAM sfx[12];
    bool running;
} tangram;

enum TangramResult
{
    TANGRAM_OK,
    TANGRAM_ERR,
};
// Defines a Tangram result, similar to Rust's `Result` in a C enum.
typedef enum TangramResult TangramResult;

TangramResult tangram_test_pointer(void *ptr);

#endif

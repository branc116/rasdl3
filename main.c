#include "SDL3/SDL_camera.h"
#include "SDL3/SDL_events.h"
#define SDL_MAIN_USE_CALLBACKS 1  /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_test_font.h>
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"

unsigned long long init_time;

#define LOG_COMMON(severity, fmt, ...) do { \
	long long unsigned timens = __builtin_ia32_rdtsc() - init_time; \
	long long unsigned total_timeus = timens / 1000; \
	long long unsigned total_timems = total_timeus / 1000; \
	int timems = total_timems % 1000; \
	long long unsigned total_times = total_timems / 1000; \
	int times = total_times % 60; \
	long long unsigned total_timem = total_times / 60; \
	int timem = total_timem % 60; \
	long long unsigned total_timeh = total_timem / 60; \
	int timeh = total_timem % 24; \
	int total_timed = total_timeh / 24; \
	fprintf(stderr, "[" severity "][%d:%2d:%2d:%2d:%3d][T%d][%s:%d]" fmt "\n", total_timed, timeh, timem, times, timems, get_thread_id(), __FILE__, __LINE__, \
                    ##__VA_ARGS__); \
} while (0)
#define LOGE(fmt, ...)      LOG_COMMON("ERROR", "%s "fmt, SDL_GetError(),  ##__VA_ARGS__) 
#define LOGI(fmt, ...)      LOG_COMMON(" INFO", fmt, ##__VA_ARGS__) 

#define TRYOK(expr) do { \
  ok = expr; \
  if (ok == false) {  \
    LOGE("Failed to '" #expr "'"); \
    exit(1); \
  } \
  else LOGI("'" #expr "' OK");  \
} while(0)

#define TRYOKE(expr) do { \
  ok = expr; \
  if (ok == false) {  \
    LOGE("Failed to '" #expr "'"); \
    exit(1); \
  } \
} while(0)

int get_thread_id() { return 0; }

/* We will use this renderer to draw into this window every frame. */
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_bool ok;

#define N 32
static SDL_FPoint track_up[N];
static SDL_FPoint track_down[N];
static int w = 800, h = 600;
static float acc = 0;
static float speed = 0;
static float x = 0;
static float y = 0;
static float angle = 0;
static float dangle = 0;
bool on_road;
int finished = 0;
float time;

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {
  init_time = __builtin_ia32_rdtsc();
  SDL_Surface* surface;
  int vsync;
  SDL_GLContext gl_ctx;

  TRYOK(SDL_Init(SDL_INIT_VIDEO));
  TRYOK(SDL_SetAppMetadata("SDL3 Racing", "1.0.0", "com.branc116.sdl3racing"));
  TRYOK(window = SDL_CreateWindow("Racing", 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE));
  TRYOK(surface = SDL_GetWindowSurface(window));
  TRYOK(SDL_GetWindowSurfaceVSync(window, &vsync));
  LOGI("Vsync state = %d", vsync);
  TRYOK(SDL_GetWindowSurfaceVSync(window, &vsync));
  TRYOK(SDL_SetWindowOpacity(window, 1.0f));
  TRYOK(gl_ctx = SDL_GL_CreateContext(window));
  SDL_SystemTheme theme = SDL_GetSystemTheme();
  LOGI("Theme is %d", theme);
  TRYOK(renderer = SDL_GetRenderer(window));
  TRYOKE(SDL_GetCurrentRenderOutputSize(renderer, &w, &h));
  for (int i = 0; i < N; ++i) {
    float t = (float)i / (N - 1);
    track_down[i] = (SDL_FPoint) { .x = w*t, .y = 50.f*SDL_sinf(t * 20.f) + (float)w / 2.f };
    track_up[i].x = track_down[i].x;
    track_up[i].y = track_down[i].y - 100;
  }
  x = track_up[0].x;
  y = (track_up[0].y + track_down[0].y) * 0.5;
  for (int i = 0; i < N; ++i) {
    float t = (float)i / (N - 1);
  }
  return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, const SDL_Event *event) {
  if (event->type == SDL_EVENT_QUIT) {
    return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
  } else if (event->type == SDL_EVENT_KEY_UP) {
    if (event->key.key == SDLK_W) acc = 0;
    if (event->key.key == SDLK_S) acc = 0;
    if (event->key.key == SDLK_A) dangle = 0;
    if (event->key.key == SDLK_D) dangle = 0;
    if (event->key.key == SDLK_R) {
      x = track_up[0].x;
      y = (track_up[0].y + track_down[0].y) * 0.5;
      for (int i = 0; i < N; ++i) {
        float t = (float)i / (N - 1);
      }
      finished = 0;
      time = 0;
      acc = 0;
      speed = 0;
      angle = 0;
      dangle = 0;
    }
  } else if (event->type == SDL_EVENT_KEY_DOWN) {
    if (event->key.key == SDLK_W) acc = 1;
    if (event->key.key == SDLK_S) acc = -1;
    if (event->key.key == SDLK_A) dangle = -1;
    if (event->key.key == SDLK_D) dangle = 1;
  }
  return SDL_APP_CONTINUE;  /* carry on with the program! */
}

void render_rect_rot(float x, float y, float w, float h, float angle) {
  float hh = h / 2;
  float hw = w / 2;
  float x1, x2, y1, y2;
  float c = SDL_cosf(angle), s = SDL_sinf(angle);
  {
    float ulx = c * -hw + s * -hh; 
    float uly = -s * -hw + c * -hh;
    float urx = c * hw + s * -hh;
    float ury = -s * hw + c * -hh;
    SDL_RenderLine(renderer, x + ulx, y + uly, x + urx, y + ury);
  }
  {
    float ulx = c * -hw + s * hh; 
    float uly = -s * -hw + c * hh;
    float urx = c * hw + s * hh;
    float ury = -s * hw + c * hh;
    SDL_RenderLine(renderer, x + ulx, y + uly, x + urx, y + ury);
  }
  {
    float ulx = c  * -hw + s * -hh; 
    float uly = -s * -hw + c * -hh;
    float urx = c  * -hw + s * hh;
    float ury = -s * -hw + c * hh;
    SDL_RenderLine(renderer, x + ulx, y + uly, x + urx, y + ury);
  }
  {
    float ulx = c  * hw + s * -hh; 
    float uly = -s * hw + c * -hh;
    float urx = c  * hw + s * hh;
    float ury = -s * hw + c * hh;
    SDL_RenderLine(renderer, x + ulx, y + uly, x + urx, y + ury);
  }
}

void draw_car(float x, float y, float angle) {
  if (on_road) TRYOKE(SDL_SetRenderDrawColorFloat(renderer, 1, 0.1, 0.1, 1));
  else TRYOKE(SDL_SetRenderDrawColorFloat(renderer, 0.1, 1.0, 0.1, 1));
  render_rect_rot(x, y, 50, 25, angle);
}

void draw_track(void) {
  TRYOKE(SDL_SetRenderDrawColorFloat(renderer, 0.1, 1, 0.1, 1));
  TRYOKE(SDL_RenderLines(renderer, track_up, N));
  TRYOKE(SDL_RenderLines(renderer, track_down, N));
}

static void get_closest(float x, float y, int* i) {
  int closest = 0;
  float mx = track_down[0].x;
  float my = (track_down[0].y + track_up[0].y) / 2;
  float cDist = powf(mx - x, 2) + powf(my - y, 2);
  for (int j = 0; j < N; ++j) {
    mx = track_down[j].x;
    my = (track_down[j].y + track_up[j].y) / 2;
    float dist = powf(mx - x, 2) + powf(my - y, 2);
    if (cDist > dist) {
      cDist = dist;
      closest = j;
    }
  }
  *i = closest;
}

static bool is_on_road(float x, float y) {
  int closest = 0;
  get_closest(x, y, &closest);
  return track_down[closest].y > y && track_up[closest].y < y;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate) {
  int closest = 0;
  get_closest(x, y, &closest);
  if (closest < N - 1) {
    float dt = 1 / 60.f;
    time += dt;
    angle += dt * -dangle * 0.2;
    on_road = is_on_road(x, y);

    float s = SDL_sinf(angle), c = SDL_cosf(angle);
    speed = speed * (on_road ? 0.99 : 0.5) + 20.f * dt * acc;
    speed *= dangle != 0 ? 0.9999 : 1;
    x += dt * speed * c;
    y += dt * speed * -s;


    TRYOKE(SDL_SetRenderDrawColorFloat(renderer, 0, 0, 0, 1));
    TRYOKE(SDL_RenderClear(renderer));
    draw_track();
    draw_car(x, y, angle);

    char buff[64];
    sprintf(buff, "Time: %.3fs", time);
    SDLTest_DrawString(renderer, 0, 0, buff);
  } else {
    finished += 1;
    char buff[64];
    sprintf(buff, "Time: %.3fs", time);
    for (int i = 0; i < finished; ++i) {
      int y = (i * 10) % h;
      int x = 140 *((i * 10) / h);
      SDLTest_DrawString(renderer, x, y, buff);
    }
  }
  TRYOKE(SDL_RenderPresent(renderer));
  return SDL_APP_CONTINUE;  /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate)
{
    /* SDL will clean up the window/renderer for us. */
}



/*
 *
 *

 Not on wayland display..
  SDL_MessageBoxData msg = {
    .window = window,
    .title = "Hello",
    .message = "This is a message",
    .buttons = &(SDL_MessageBoxButtonData) {
      .text = "Ok",
      .buttonID = 1
    },
    .numbuttons = 1,
    .colorScheme = &(SDL_MessageBoxColorScheme) {
      .colors = { 50, 50, 50, 50 }
    }
  };
  int butt;
  TRYOK(SDL_ShowMessageBox(&msg, &butt));
*/

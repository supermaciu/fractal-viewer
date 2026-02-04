#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080

#define SCALE 1

static int iter = 1;

double interpolate(int scaled, int from1, int from2, double to1, double to2) {
    return (double) scaled/(from2-from1)*(to2-to1)+to1;
}

double magnitude(double a, double b) {
    return SDL_sqrt(a*a + b*b);
}

bool isMandelbrot(double a, double b) {
    double fa = 0;
    double fb = 0;
    int i;
    for (i = 0; i < 200; i++) {
        double temp_fa = fa*fa - fb*fb + a;
        double temp_fb = 2*fa*fb + b;
        fa = temp_fa;
        fb = temp_fb;
        if (magnitude(fa, fb) > 2) break;
    }

    return (i > iter);
}

void draw() {
    // clear canvas
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    // draw axis
    SDL_SetRenderDrawColor(renderer, 50, 50, 255, SDL_ALPHA_OPAQUE);
    SDL_RenderLine(renderer, WINDOW_WIDTH/2, 0, WINDOW_WIDTH/2, WINDOW_HEIGHT);
    SDL_RenderLine(renderer, 0, WINDOW_HEIGHT/2, WINDOW_WIDTH, WINDOW_HEIGHT/2);

    // draw fractal
    uint8_t colored;
    double a, b;
    for (int y = 0; y < WINDOW_HEIGHT; y++) {
        for (int x = 0; x < WINDOW_WIDTH; x++) {
            a = interpolate(x, 0, WINDOW_WIDTH, -2, 2);
            b = -interpolate(y, 0, WINDOW_HEIGHT, -1.25, 1.25);
            if (isMandelbrot(a, b)) {
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
                SDL_RenderPoint(renderer, x, y);
            }
        }
    }

    SDL_RenderPresent(renderer);
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_CreateWindowAndRenderer("Fractal Viewer", WINDOW_WIDTH*SCALE, WINDOW_HEIGHT*SCALE, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
        SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetRenderLogicalPresentation(renderer, WINDOW_WIDTH*SCALE, WINDOW_HEIGHT*SCALE, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SDL_SetRenderScale(renderer, SCALE, SCALE);

    draw();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }
    if (event->type == SDL_EVENT_KEY_DOWN) {
        if (event->key.key == SDLK_UP) {
            iter++;
        } else if (event->key.key == SDLK_DOWN) {
            iter = (iter > 1) ? iter-1 : iter;
        }
        draw();
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
}
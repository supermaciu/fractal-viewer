#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 420

#define SCALE 2

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
    for (i = 0; i < 1000; i++) {
        double temp_fa = fa*fa - fb*fb + a;
        double temp_fb = 2*fa*fb + b;
        fa = temp_fa;
        fb = temp_fb;
        if (magnitude(fa, fb) > 2) break;
    }

    return (i > 800);
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

    // draw fractal
    uint8_t colored;
    double a, b;
    for (int y = 0; y < WINDOW_HEIGHT; y++) {
        for (int x = 0; x < WINDOW_WIDTH; x++) {
            a = interpolate(x, 0, WINDOW_WIDTH, -2, 2);
            b = -interpolate(y, 0, WINDOW_HEIGHT, -2, 2);
            colored = isMandelbrot(a, b) ? 0 : 255;
            SDL_SetRenderDrawColor(renderer, colored, colored, colored, SDL_ALPHA_OPAQUE);
            SDL_RenderPoint(renderer, x, y);
        }
    }

    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
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
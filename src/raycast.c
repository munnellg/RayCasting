#include <math.h>
#include <stdio.h>
#include <SDL2/SDL.h>

#define PROGRAM_NAME   "Maze"

#define MAP_WIDTH  5
#define MAP_HEIGHT 5

#define TILE_SIZE   64
#define WALL_HEIGHT 64

#define FIELD_OF_VIEW         60
#define FRAMES_PER_SECOND     60
#define DEFAULT_SCREEN_WIDTH  640
#define DEFAULT_SCREEN_HEIGHT 480

#define RADIAN(x) ((x) * M_PI/180.f)

#define MAX(x,y) (((x)>(y))? (x) : (y))
#define MIN(x,y) (((x)<(y))? (x) : (y))
#define CLAMP(lo, x, hi) MIN(MAX(lo, x), hi)

struct actor {
    float x, y, theta, height, turn_speed, move_speed;
};

struct state {
    SDL_Window   *window;
    SDL_Renderer *renderer;
    SDL_Texture  *texture;

    int quit;
    int fullscreen;
    int screen_width, screen_height;

    struct actor player;    
};

static int map[] = {
    1, 1, 1, 1, 1,
    1, 0, 0, 0, 1,
    1, 0, 1, 0, 1,
    1, 0, 0, 0, 1,
    1, 1, 1, 1, 1
};

static void
fill_column ( Uint32 *buf, int bufw, Uint32 colour, int x, int y, int len ) {
    for ( int i = y; i < y+len; i++ ) { buf[x + i*bufw] = colour; }
}

static void
turn ( struct actor *actor, int dir ) {
    actor->theta += actor->turn_speed * dir;
}

static void
strafe ( struct actor *actor, int dir ) {
    float dx = cos(RADIAN(actor->theta + 90)) * actor->move_speed * dir;
    float dy = sin(RADIAN(actor->theta + 90)) * actor->move_speed * dir;
    actor->x += dx; actor->y += dy;
}

static void
move ( struct actor *actor, int dir ) {
    float dx = cos(RADIAN(actor->theta)) * actor->move_speed * dir;
    float dy = sin(RADIAN(actor->theta)) * actor->move_speed * dir;
    actor->x += dx; actor->y += dy;
}

static int
initialize ( struct state *s ) {
    // zero everything
    memset( s, 0, sizeof(struct state) );   
    
    // set up the window parameters before initializing SDL
    s->screen_width  = DEFAULT_SCREEN_WIDTH;
    s->screen_height = DEFAULT_SCREEN_HEIGHT;
    s->fullscreen    = 0;

    // position the player
    s->player.x          = 1.5f;
    s->player.y          = 1.5f;
    s->player.theta      = 0.0f;
    s->player.move_speed = 0.000015;
    s->player.turn_speed = 0.0007f;
    s->player.height     = 32;

    // Initialize SDL so we can use it
    if ( SDL_Init(SDL_INIT_EVERYTHING) < 0 ) {
        printf("SDL_Init : %s\n", SDL_GetError());
        return 0;
    }
    
    // create a new window and renderer according to settings
    int status = SDL_CreateWindowAndRenderer(
            s->screen_width, s->screen_height,              // resolution
            s->fullscreen? SDL_WINDOW_FULLSCREEN_DESKTOP:0, // toggle fullscreen
            &s->window, &s->renderer                        // init structs
    );

    // make sure we were able to create a window and renderer
    if ( status < 0 ) {
        printf("SDL_CreateWindowAndRenderer : %s\n", SDL_GetError());
        return 0;
    }
    
    // configure how the renderer handles screen dimensions
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(s->renderer, s->screen_width, s->screen_height);
    SDL_SetWindowTitle(s->window, PROGRAM_NAME);
    // SDL_SetRelativeMouseMode(SDL_TRUE);

    // create the texture that we will use for rendering
    s->texture = SDL_CreateTexture(
        s->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
        s->screen_width, s->screen_height
    );

    return 1;
}

static void
handle_events ( struct state *s ) {
    SDL_Event e;

    const Uint8 *keyboard = SDL_GetKeyboardState(NULL);

    while ( SDL_PollEvent(&e) ) {
        switch ( e.type ) {
            case SDL_QUIT:
                s->quit = 1;
                break;
            case SDL_KEYDOWN:
                if (e.key.keysym.sym == SDLK_q) { s->quit = 1; }
                break;      
        }
    }
    
    move(&s->player, keyboard[SDL_SCANCODE_W] - keyboard[SDL_SCANCODE_S]);    
    strafe(&s->player, keyboard[SDL_SCANCODE_D] - keyboard[SDL_SCANCODE_A]);
    turn(&s->player, keyboard[SDL_SCANCODE_RIGHT]-keyboard[SDL_SCANCODE_LEFT]);
}

static void
render ( struct state *s ) {
    Uint32 pixels[s->screen_width*s->screen_height];

    // draw the ceiling and floor
    int horizon = s->screen_width * s->screen_height * 0.5f;
    memset( pixels, 0xCC, sizeof(Uint32) * horizon );
    memset( pixels + horizon, 0x55, sizeof(Uint32) * horizon );
    
    float a = s->player.theta - FIELD_OF_VIEW/2;
    float da = (float) FIELD_OF_VIEW/s->screen_width;

    // cast rays
    for ( int i=0; i < s->screen_width; i++, a += da ) {        
        float len_ray = (float)TILE_SIZE/128;
        float dx = cos(RADIAN(a)) * len_ray;
        float dy = sin(RADIAN(a)) * len_ray;
        float x = s->player.x * TILE_SIZE, y = s->player.y * TILE_SIZE;
        float dist = 0;

        for (;;) {
            int gridx = x/TILE_SIZE, gridy = y/TILE_SIZE;

            if ( gridx < 0 || gridx >= MAP_WIDTH || 
                    gridy < 0 || gridy >= MAP_HEIGHT ) {
                dist = -1;
                break;
            }

            if ( map[ gridx + gridy*MAP_WIDTH ] != 0 ) {
                break;
            }

            x += dx; y += dy;            
            dist += len_ray;
        }

        // only render if ray hit something
        if ( dist >= 0 ) { 
            int height = s->screen_height/MAX(1.0f, dist/TILE_SIZE);
            int wall_top = (s->screen_height - height)/2;
            fill_column( pixels, s->screen_width, 0xFF, i, wall_top, height );
        };
    }

    // copy pixel buffer to texture and update the display
    SDL_UpdateTexture(s->texture, NULL, pixels, s->screen_width*sizeof(Uint32));
    SDL_RenderClear(s->renderer);
    SDL_RenderCopy(s->renderer, s->texture, NULL, NULL);
    SDL_RenderPresent(s->renderer);
}

static void
terminate ( struct state *s ) {
    if ( s->texture )  { SDL_DestroyTexture(s->texture); }
    if ( s->renderer ) { SDL_DestroyRenderer(s->renderer); }
    if ( s->window )   { SDL_DestroyWindow(s->window); }
    SDL_Quit();
}

int
main ( int argc, char *argv[] ) {
    struct state s;
    
    if (!initialize(&s)) {      
        terminate(&s);
        return 1;
    }

    Uint32 start = SDL_GetTicks();
    while (!s.quit) {
        handle_events(&s);
        if ( (SDL_GetTicks() - start) > 1000/FRAMES_PER_SECOND     ) {
            render(&s);
            start = SDL_GetTicks();
        }
    }

    terminate(&s);
    
    return 0;
}

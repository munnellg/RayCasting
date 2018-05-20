#include <math.h>
#include <stdio.h>
#include <SDL2/SDL.h>

#define PROGRAM_NAME   "Maze"

#define MAP_WIDTH  24
#define MAP_HEIGHT 24

#define FIELD_OF_VIEW         60
#define FRAMES_PER_SECOND     60
#define MS_PER_FRAME          (1000/FRAMES_PER_SECOND)
#define DEFAULT_SCREEN_WIDTH  640
#define DEFAULT_SCREEN_HEIGHT 360
#define MAX_RAY               32

#define RADIAN(x) ((x) * M_PI/180.f)

#define MAX(x,y) (((x)>(y))? (x) : (y))
#define MIN(x,y) (((x)<(y))? (x) : (y))

struct actor {
    float x, y, theta, turn_speed, move_speed;
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
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,2,2,2,2,2,0,0,0,0,3,0,3,0,3,0,0,0,1,
    1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,3,0,0,0,3,0,0,0,1,
    1,0,0,0,0,0,2,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,2,2,0,2,2,0,0,0,0,3,0,3,0,3,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,4,0,4,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,4,0,0,0,0,5,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,4,0,4,0,0,0,0,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,4,0,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,4,4,4,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
};

static void
fill_column ( Uint32 *buf, int bufw, Uint32 colour, int x, int y, int len ) {
    for ( int i = y; i < y+len; i++ ) { buf[x + i*bufw] = colour; }
}

static int
empty_cell ( int x, int y ) {
    // assume that any cell that is out of bounds is empty
    if ( x < 0 || x >= MAP_WIDTH || y < 0 || y >= MAP_HEIGHT ) return 1;
    return map[ x + MAP_WIDTH*y ] == 0;
}

static void
turn ( struct actor *actor, int dir ) {
    actor->theta += actor->turn_speed * dir;
}

static void
strafe ( struct actor *actor, int dir ) {
    float dx = cos(RADIAN(actor->theta + 90)) * actor->move_speed * dir;
    float dy = sin(RADIAN(actor->theta + 90)) * actor->move_speed * dir;
    if ( empty_cell( actor->x+dx, actor->y ) ) { actor->x += dx; }
    if ( empty_cell( actor->x, actor->y+dy ) ) { actor->y += dy; }
}

static void
move ( struct actor *actor, int dir ) {
    float dx = cos(RADIAN(actor->theta)) * actor->move_speed * dir;
    float dy = sin(RADIAN(actor->theta)) * actor->move_speed * dir;
    if ( empty_cell( actor->x+dx, actor->y ) ) { actor->x += dx; }
    if ( empty_cell( actor->x, actor->y+dy ) ) { actor->y += dy; }
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
    s->player.move_speed = 0.15;
    s->player.turn_speed = 2.0f;

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
    turn(&s->player, keyboard[SDL_SCANCODE_RIGHT]-keyboard[SDL_SCANCODE_LEFT]);
    strafe(&s->player, keyboard[SDL_SCANCODE_D] - keyboard[SDL_SCANCODE_A]);
}

static void
render ( struct state *s ) {
    Uint32 pixels[s->screen_width*s->screen_height];

    // draw the ceiling and floor in shades of grey
    int horizon = (s->screen_width * s->screen_height) >> 1;
    memset( pixels, 0xCC, sizeof(Uint32) * horizon );
    memset( pixels + horizon, 0x55, sizeof(Uint32) * horizon );
    
    // compute start angle for FOV and the angle increment per screen column
    float a = s->player.theta - FIELD_OF_VIEW/2;
    float da = (float) FIELD_OF_VIEW/s->screen_width;
    float dist2plane = (s->screen_width/2.0f)/tan(RADIAN(FIELD_OF_VIEW/2.0f));
    
    // cast rays
    for ( int i=0; i < s->screen_width; i++, a += da ) {        
        // figure out direction and displacement of ray in x and y directions
        float vx = cos(RADIAN(a));
        float vy = sin(RADIAN(a));
                
        // map cells
        int ix = s->player.x;
        int iy = s->player.y;
       
        // figure out ray x and y deltas
        float dx = fabs(1.0/vx);
        float dy = fabs(1.0/vy);

        // figure out integer steps for map cells
        int stepix = (vx < 0)? -1 : 1;
        int stepiy = (vy < 0)? -1 : 1;

        // initialize ray distance to displacement from next cell edge
        float distx = (vx < 0)? (s->player.x-ix)*dx : (ix+1.0-s->player.x)*dx;
        float disty = (vy < 0)? (s->player.y-iy)*dy : (iy+1.0-s->player.y)*dy;

        int side;

        // extend ray until it hits something or until it exceeds max view dist
        while ( empty_cell( ix, iy ) ) {
            if ( distx < disty ) {
                distx += dx;
                ix += stepix;                
                side = 0;
            } else {
                disty += dy;
                iy += stepiy;
                side = 1;
            }
        }

        float dist;
        if ( side ) {
            dist = (iy - s->player.y + (1 - stepiy)/2)/vy;
        } else {
            dist = (ix - s->player.x + (1 - stepix)/2)/vx;
        }

        dist = dist * cos(RADIAN((-FIELD_OF_VIEW/2.0f)+(i*da)));
        
        int h = dist2plane/dist;        
        int top = (s->screen_height - h) >> 1;
        if ( top < 0 ) { top = 0; }
        if ( top + h > s->screen_height ) { h = s->screen_height - top; }

        Uint32 colour;                        
        switch (map[ix + iy * MAP_WIDTH]) {
            case 1:  colour = 0x2b4570; break;
            case 2:  colour = 0x5d737e; break;
            case 3:  colour = 0x58a4b0; break;
            case 4:  colour = 0x373f51; break;
            case 5:  colour = 0xe49273; break;
            default: colour = 0x0000FF; break;
        }

        fill_column( pixels, s->screen_width, colour >> side, i, top, h );
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
    
    while (!s.quit) {
        Uint32 start = SDL_GetTicks();
        handle_events(&s);
        render(&s);
        Sint32 delay = MS_PER_FRAME - (SDL_GetTicks() - start);
        SDL_Delay(MAX(delay, 0));
    }

    terminate(&s);
    
    return 0;
}

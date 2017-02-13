#ifdef _WIN32
#include <Windows.h>
#endif

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#define _USE_MATH_DEFINES
#include <math.h>
#undef size_t
#include <SDL.h>
#undef main
#include "../Include/SDLauxiliary.h"

#include "../Include/TestModel.h"
#include "../Include/Raytracer.h"
#include "../Include/ModelLoader.h"
#include "../Include/bitmap_image.hpp"


//VS14 FIX
#ifdef _WIN32
FILE _iob[] = { *stdin, *stdout, *stderr };
extern "C" FILE * __cdecl __iob_func(void) {
    return _iob;
}
#endif

/*
using namespace std;
using glm::vec3;
using glm::mat3;*/


using namespace std;
using glm::vec3;
using glm::vec2;
using glm::ivec2;
using glm::mat3;

/* ----------------------------------------------------------------------------*/
/* GLOBAL VARIABLES                                                            */

const int SCREEN_WIDTH = 500;
const int SCREEN_HEIGHT = 500;
SDL_Surface* screen;
int t;
glm::vec3 campos(0.0, 0.0, -2.0);

/* ----------------------------------------------------------------------------*/
/* FUNCTIONS                                                                   */

void Update();
void Draw(model::Scene& scene);
glm::vec2 project2D(glm::vec3 point);
void Interpolate(glm::ivec2 a, glm::ivec2 b, vector<glm::ivec2>& result);

int main( int argc, char* argv[] )
{
	screen = InitializeSDL( SCREEN_WIDTH, SCREEN_HEIGHT );
	t = SDL_GetTicks();	// Set start value for timer.

    // Load model
    std::vector<Triangle> model = std::vector<Triangle>();
    LoadTestModel(model);

    model::Scene scene;
    scene.addTriangles(model);


	while( NoQuitMessageSDL() )
	{
		Update();
		Draw(scene);
	}

	SDL_SaveBMP( screen, "screenshot.bmp" );
	return 0;
}

void Update()
{
    Uint8* keystate = SDL_GetKeyState(0);
    if (keystate[SDLK_UP]) {
        // Move camera forward
        campos.z += 0.005;
    }
    if (keystate[SDLK_DOWN]) {
        // Move camera backward
        campos.z -= 0.005;
    }
    if (keystate[SDLK_LEFT]) {
        // Move camera to the left
        campos.x -= 0.005;
    }
    if (keystate[SDLK_RIGHT]) {
        // Move 
        campos.x += 0.005;
    }

	// Compute frame time:
	int t2 = SDL_GetTicks();
	float dt = float(t2-t);
	t = t2;
	cout << "Render time: " << dt << " ms." << endl;
}

void Draw(model::Scene& scene)
{
    

    


    // Project position to 2D:
   /* float xScr = (pos.x - campos.x) / (pos.z - campos.z);
    float yScr = (pos.y - campos.y) / (pos.z - campos.z);

    // Scale and bias
    xScr += 1;
    yScr += 1;
    xScr *= 0.5;
    yScr *= 0.5;
    xScr *= 500;
    yScr *= 500;*/


	if( SDL_MUSTLOCK(screen) )
		SDL_LockSurface(screen);

    // Clear scren
    SDL_FillRect(screen, 0, 0x00000000);



	/*for( int y=0; y<SCREEN_HEIGHT; ++y )
	{
		for( int x=0; x<SCREEN_WIDTH; ++x )
		{
			vec3 color( 1.0, 1.0, 0.0 );
			PutPixelSDL( screen, x, y, color );
		}
	}*/
    for (auto& t : scene.getTrianglesRef()) {


        // Project 2D points
        vec2 projpos0 = project2D(t.v0);
        vec2 projpos1 = project2D(t.v1);
        vec2 projpos2 = project2D(t.v2);
        std::vector<ivec2> lineA( (int)glm::length(projpos1-projpos0) ), lineB((int)glm::length(projpos2 - projpos0)), lineC((int)glm::length(projpos2 - projpos1));
        Interpolate(projpos0, projpos1, lineA);
        Interpolate(projpos0, projpos2, lineB);
        Interpolate(projpos1, projpos2, lineC);

        for (vec2 v : lineA) {
            PutPixelSDL(screen, v.x, v.y, glm::vec3(1.0, 1.0, 1.0));
        }
        for (vec2 v : lineB) {
            PutPixelSDL(screen, v.x, v.y, glm::vec3(1.0, 1.0, 1.0));
        }
        for (vec2 v : lineC) {
            PutPixelSDL(screen, v.x, v.y, glm::vec3(1.0, 1.0, 1.0));
        }
        //PutPixelSDL(screen, projpos0.x, projpos0.y, glm::vec3(1.0,1.0,1.0));
        //PutPixelSDL(screen, projpos1.x, projpos1.y, glm::vec3(1.0, 1.0, 1.0));
        //PutPixelSDL(screen, projpos2.x, projpos2.y, glm::vec3(1.0, 1.0, 1.0));
    }

	if( SDL_MUSTLOCK(screen) )
		SDL_UnlockSurface(screen);

	SDL_UpdateRect( screen, 0, 0, 0, 0 );
}

glm::vec2 project2D(glm::vec3 point) {
    // Project position to 2D:
    glm::vec2 screen_space_pos = glm::vec2(
        (point.x - campos.x) / (point.z - campos.z),
        (point.y - campos.y) / (point.z - campos.z));

    // Scale and bias
    screen_space_pos.x += 1;
    screen_space_pos.y += 1;
    screen_space_pos *= 0.5;
    screen_space_pos.x *= SCREEN_WIDTH;
    screen_space_pos.y *= SCREEN_HEIGHT;
    return screen_space_pos;
}

void Interpolate(glm::ivec2 a, glm::ivec2 b, vector<glm::ivec2>& result) {
    int N = result.size();
    vec2 step = vec2(b - a) / float(max(N - 1, 1));
    vec2 current(a);
    for (int i = 0; i<N; ++i) {
        result[i] = current;
        current += step;
    }
}
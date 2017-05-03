#ifdef _WIN32
#include <Windows.h>
#endif

#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>
#undef size_t
#include <SDL.h>
#include <SDL_video.h>
#include "../Include/SDLauxiliary.h"
#include <glm/gtx/rotate_vector.hpp>
#undef main
#include "../Include/RenderUtil.h"

#include "../Include/TestModel.h"

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

typedef size_t uint;

using namespace std;
using glm::vec3;
using glm::vec2;
using glm::ivec2;
using glm::mat3;

/* ----------------------------------------------------------------------------*/
/* GLOBAL VARIABLES                                                            */

SDL_Surface* screen;
int t;
glm::vec3 campos(0.0, 0.0, -3.0);
glm::vec3 camposs = campos;
glm::vec3 camdir(0.0f, 0.0f, 1.0f);

glm::vec3 sunlight_pos(-26.256, -33.5917, 19.1776);
glm::vec3 sunlight_dir(0.64513, 0.692729, -0.322388);

glm::vec3 plight_pos(0.0f, -1.0f, 0.0f);
bool running = true;

float yaw = 0.0f, pitch = 0.0f;
float yaws = 0.0f, pitchs = 0.0f;
bool mouse_look_enabled = false;
bool p_pressed = false;

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
    //scene.addTriangles(model);p
    
    // LOAD TRAINSTATION MODEL AND SET MATERIAL ID
    model::Model m("trainstation.obj");
    m.setActiveMaterial(MaterialType::CARGO_METAL);

    scene.addModel(&m);

    if( scene.getTrianglesRef().size() == 0){
        printf("There are no triangles!");
        while (1);
    }

    for (auto& triangle : scene.getTrianglesRef()) {
        RENDER::addTriangle(triangle);
    }

    RENDER::initialize(); //Triangles must be loaded to RENDER before this is called; "void RENDER::addTriangle(Triangle& triangle)"
    
    // Generate a few buffers to store results in
    FrameBuffer* frame_buffer  = new FrameBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, RENDER::getContext());
    FrameBuffer* ssao_buffer   = new FrameBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, RENDER::getContext());
    FrameBuffer* shadow_buffer = new FrameBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, RENDER::getContext());
   // FrameBuffer* lighting_buffer = new FrameBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, RENDER::getContext());

    // Generate SSAO Kernel
    int sample_count = 96;
    RENDER::buildSSAOSampleKernel(sample_count);

    // Generate a buffer to store lighting in
    const int LIGHTMAP_WIDTH = 2048;
    const int LIGHTMAP_HEIGHT = 2048;
    FrameBuffer* lightmap_buffer = new FrameBuffer(LIGHTMAP_WIDTH, LIGHTMAP_HEIGHT, RENDER::getContext());

    // Render shadowmap
   /* lightmap_buffer->clear();
    RENDER::renderFrame(lightmap_buffer, sunlight_pos, sunlight_dir);
    lightmap_buffer->saveBMP("lightmap.bmp");*/
    
    bool pressed = false;
	while( NoQuitMessageSDL() && running )
	{
		Update(); 

        clock_t b = clock();

        // Render main frame
        //RENDER::renderFrame(lightmap_buffer, camposs, camdir);
        RENDER::renderFrame(frame_buffer, camposs, camdir);

        // TEMP: Press Space to render light buffer
        Uint8* keystate = SDL_GetKeyState(0);
        if (keystate[SDLK_SPACE] && !pressed) {
            
            // Clear lightmap
            lightmap_buffer->clear();

            // Store position
            sunlight_pos = camposs;
            sunlight_dir = camdir;

            // Render lighting
            RENDER::renderFrame(lightmap_buffer, sunlight_pos, sunlight_dir);
            lightmap_buffer->saveBMP("lightmap.bmp");
            pressed = true;

            std::cout << "lightpos: " << sunlight_pos.x << " " << sunlight_pos.y << " " << sunlight_pos.z << std::endl;
            std::cout << "lightdir: " << sunlight_dir.x << " " << sunlight_dir.y << " " << sunlight_dir.z << std::endl;
        } else {
            pressed = false;
        }

        // Render Shadow buffer
        RENDER::calculateShadows(frame_buffer, lightmap_buffer, shadow_buffer, sunlight_pos, sunlight_dir, camposs);

        // Calculate lighting
       // for (int i = 0; i < 10; i++) {
            RENDER::calculatePointLight(frame_buffer, shadow_buffer, camposs, camdir, plight_pos, glm::vec3(1.0f, 0.5f, 0.5f), 5.0f, 0.5f, 128.0f);
            //RENDER::calculatePointLight(frame_buffer, shadow_buffer, camposs, camdir, camposs, glm::vec3(0.5f, 1.0f, 0.5f), 20.0f, 0.5f, 128.0f);
           /* RENDER::calculatePointLight(frame_buffer, shadow_buffer, camposs, glm::vec3(-10.f, -10.0f, 1.0f), glm::vec3(1.0f, 0.5f, 0.5f), 20.0f, 0.5f, 128.0f);
            RENDER::calculatePointLight(frame_buffer, shadow_buffer, camposs, glm::vec3(10.f, -10.0f, 1.0f), glm::vec3(1.0f, 0.5f, 0.5f), 20.0f, 0.5f, 128.0f);
            RENDER::calculatePointLight(frame_buffer, shadow_buffer, camposs, glm::vec3(-10.f, 10.0f, 1.0f), glm::vec3(1.0f, 0.5f, 0.5f), 20.0f, 0.5f, 128.0f);*/
    //    }

        // Render SSAO buffer
        RENDER::calculateSSAO(frame_buffer, ssao_buffer, SCREEN_WIDTH, SCREEN_HEIGHT, camposs, camdir);
       

        // Accumulate results on GPU
        RENDER::accumulateBuffers(frame_buffer, ssao_buffer, shadow_buffer, SCREEN_WIDTH, SCREEN_HEIGHT);

        // TEMP: Copy back lighting buffer
        frame_buffer->transferGPUtoCPU_Colour();
        /*ssao_buffer->transferGPUtoCPU_Colour();
        shadow_buffer->transferGPUtoCPU_Colour();*/

        clock_t e = clock();

        double elapsed_secs = 1000 * double(e - b) / CLOCKS_PER_SEC;
        if(VERBOSE) std::cout << "FULL FRAME RASTER: " << elapsed_secs << "ms" << std::endl;
        
        
        if (SDL_MUSTLOCK(screen)) SDL_LockSurface(screen);

#pragma omp parallel for
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            for (int y = 0; y < SCREEN_HEIGHT; y++) {
                //PixelColour& p = ssao_buffer->getCPUBuffer_colour()[x + y * SCREEN_WIDTH];//frame_buffer[x + y * SCREEN_WIDTH];
                PixelColour& p2 = frame_buffer->getCPUBuffer_colour()[x + y * SCREEN_WIDTH];
                //PixelColour& p3 = shadow_buffer->getCPUBuffer_colour()[x + y * SCREEN_WIDTH];
                
                float r, g, b;
                r = (float)p2.r;
                g = (float)p2.g;
                b = (float)p2.b;

                // TEMP: multiply by shadow factor
           /*     r *= (float)p3.r/255.0f;
                g *= (float)p3.g / 255.0f;
                b *= (float)p3.b / 255.0f;

                // Add specular component
                r += p3.a;
                g += p3.a;
                b += p3.a;

                // Apply SSAO
                r *= (float)p.r / 255.0f;
                g *= (float)p.g / 255.0f;
                b *= (float)p.b / 255.0f;*/

                Uint32* a = (Uint32*)screen->pixels + y*screen->pitch / 4 + x;
               // *a = SDL_MapRGB(screen->format, ((float)p.r/255.0f*(float)p2.r/255.0f)*255, ((float)p.g/255.0f*(float)p2.g/255.0f) * 255, ((float)p.b/255.0f*(float)p2.b/255.0f)* 255);
                //*a = SDL_MapRGB(screen->format, p.r, p.g, p.b);
                *a = SDL_MapRGB(screen->format, glm::clamp((int)r, 0, 255), glm::clamp((int)g, 0, 255), glm::clamp((int)b, 0, 255));
                //PutPixelSDL(screen, x, y, glm::vec3(p.r, p.g, p.b));
                //*a = SDL_MapRGB(screen->format, glm::clamp((int)p3.r, 0, 255), glm::clamp((int)p3.g, 0, 255), glm::clamp((int)p3.b, 0, 255));
                // Clear
                frame_buffer->getCPUBuffer_tdata()[x + y * SCREEN_WIDTH].depth = 0;

            }
        }

        if (SDL_MUSTLOCK(screen)) SDL_UnlockSurface(screen);

        SDL_UpdateRect(screen, 0, 0, 0, 0);

        // Clear scren
        SDL_FillRect(screen, 0, 0x00000000);
        //while (1);
	}
    

    RENDER::release();
    delete frame_buffer;
    delete ssao_buffer;
    delete lightmap_buffer;
    //delete lighting_buffer;

	SDL_SaveBMP( screen, "screenshot.bmp" );
	return 0;
}

void Update()
{
    Uint8* keystate = SDL_GetKeyState(0);
    /*if (keystate[SDLK_UP]) {
        // Move camera forward
        campos.z += 0.080f;
    }
    if (keystate[SDLK_DOWN]) {
        // Move camera backward
        campos.z -= 0.080f;
    }
    if (keystate[SDLK_LEFT]) {
        // Move camera to the left
        campos.x -= 0.080f;
    }
    if (keystate[SDLK_RIGHT]) {
        // Move 
        campos.x += 0.080f;
    }

    if (keystate[SDLK_PAGEUP]) {
        // Move camera to the left
        campos.y -= 0.080f;
    }
    if (keystate[SDLK_PAGEDOWN]) {
        // Move 
        campos.y += 0.080f;
    }*/
    

    if (keystate[SDLK_ESCAPE]) {
        running = false;
    }

    if (keystate[SDLK_p] && !p_pressed) {
        mouse_look_enabled = !mouse_look_enabled;
        p_pressed = true;
    }
    if (!keystate[SDLK_p]) {
        p_pressed = false;
    }
    if (keystate[SDLK_l]) {
        plight_pos = camposs;
    }

    if (mouse_look_enabled) {
        // Mouse look
        int mouse_x, mouse_y;
        SDL_GetMouseState(&mouse_x, &mouse_y);

        int dx, dy;
        dx = mouse_x - SCREEN_WIDTH/2;
        dy = SCREEN_HEIGHT/2 - mouse_y;

        std::cout << "dx: " << dx << std::endl;
        std::cout << "dy: " << dy << std::endl;

        yaw -= (float)dx*0.005f;
        pitch -= (float)dy*0.005f;

        yaws += (yaw - yaws)*0.30f;
        pitchs += (pitch - pitchs)*0.30f;


        camdir.x = cos(yaws) * cos(pitchs);
        camdir.z = sin(yaws) * cos(pitchs);
        camdir.y = sin(pitchs);

        camdir = glm::normalize(camdir);

        /*camdir = glm::rotateX(camdir, dy*0.001f);
        camdir = glm::rotateY(camdir, dx*0.001f);
        camdir = glm::normalize(camdir);*/


        // Lock mouse
        SDL_WarpMouse(SCREEN_WIDTH/2, SCREEN_HEIGHT/2);
    }

    // Camera controls
    float cam_speed = 0.75f;
    if (keystate[SDLK_w]) {
        // Move camera forward
        campos += camdir * cam_speed;
    }
    if (keystate[SDLK_s]) {
        // Move camera backward
        campos -= camdir * cam_speed;
    }
    if (keystate[SDLK_a]) {
        // Move camera to the left
        glm::vec3 rot = glm::rotateY(camdir, glm::pi<float>() / 2);
        rot.y = 0.0f;
        rot = glm::normalize(rot);
        campos -= rot * cam_speed;
    }
    if (keystate[SDLK_d]) {
        // Move camera to the right
        glm::vec3 rot = glm::rotateY(camdir, glm::pi<float>() / 2);
        rot.y = 0.0f;
        rot = glm::normalize(rot);
        campos += rot * cam_speed;
    }

    if (keystate[SDLK_e]) {
        // Move camera to the left
        campos.y += cam_speed;
    }
    if (keystate[SDLK_q]) {
        // Move 
        campos.y -= cam_speed;
    }

    // Smooth camera
    camposs += (campos - camposs)*0.30f;

	// Compute frame time:
	int t2 = SDL_GetTicks();
	float dt = float(t2-t);
	t = t2;
    if (VERBOSE) {
        cout << "Render time: " << dt << " ms." << endl;
        std::cout << "-------------------------------------------" << std::endl;
    }
    else {
        cout << "Render time: " << dt << " ms.          \r";
    }
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

        const float max_draw_length = 100.0f;
        uint a_ = min((uint)glm::length(projpos1 - projpos0), (uint)max_draw_length);
        uint b_ = min((uint)glm::length(projpos2 - projpos0), (uint)max_draw_length);
        uint c_ = min((uint)glm::length(projpos2 - projpos1), (uint)max_draw_length);
        std::vector<ivec2> lineA( a_ ), lineB( b_ ), lineC( c_ );

        //Interpolates between 2 points such that all points are on screen
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

inline bool isNotOnScreen(glm::ivec2 p) {
    return p.x < 0 || p.x > SCREEN_WIDTH || p.y < 0 || p.y > SCREEN_HEIGHT;
}

void Interpolate(glm::ivec2 a, glm::ivec2 b, vector<glm::ivec2>& result) {
    //Adjust a and b to be on line between them and the nearest on-screen point to their original position
    vec2 d = b - a;

    int N = result.size();
    vec2 step = d / float(max(N - 1, 1));
    vec2 current(a);
    for (int i = 0; i<N; ++i) {
        result[i] = current;
        current += step;
    }
}
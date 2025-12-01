// The memory leaks are not from me, idk why
// My install maybe has problems, other classmates installs don't
// We are both using SDL3-devel
// It's from OpenGL
// idk if its because limited graphics on my laptop and having to fall back
// on vm start, it says 3d acceleration not supported by host
// this laptop does not have a GPU

// This uses my homemade pixel graphics library
// Thats why circles are more of squircles at low res
// This beats SDL's SDL_FillSurfaceRect() tho
// Circles are just as fast as rectangles

#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define USE_SSE
#include "graphics.h"

#include "QuadTree.h"

void getMousePos(Sint32* mouseX, Sint32* mouseY)
{
	float x = 0, y = 0;
	SDL_GetGlobalMouseState(&x, &y);
	*mouseX = x;
	*mouseY = y;
}

void getMousePosRel(SDL_Window* window, Sint32* mouseX, Sint32* mouseY)
{
	Sint32 mx, my;
	getMousePos(&mx, &my);
	Sint32 winX, winY;
	SDL_GetWindowPosition(window, &winX, &winY);
	*mouseX = mx - winX;
	*mouseY = my - winY;
}

void textBG(SDL_Surface* surface, Sint32 x, Sint32 y, Sint32 w, Sint32 h, char ch, void* data)
{
	drawRect(surface, x, y, w, h, rgb(0x00, 0x00, 0x00));
}

int main()
{
	SDL_Init(SDL_INIT_VIDEO);

    srand(time(NULL));

	Uint32 width = 1000, height = 600;

	SDL_Window* window = SDL_CreateWindow("Balls", width, height, 0);
	SDL_Surface* surface = SDL_GetWindowSurface(window);
	pixFmt = SDL_GetPixelFormatDetails(surface->format);

	Uint64 lastTime = SDL_GetTicks();
	Uint64 deltaTime = 0;
	Uint64 ups = 60;
	Uint64 lastFPSTime = SDL_GetTicks();
	Uint32 fpsCount = 0;
	Uint32 fps = 0;

	Uint64 ticks = 0;

	colors[0] = rgb(0xFF, 0x00, 0x00);
	colors[1] = rgb(0xFF, 0x7F, 0x00);
	colors[2] = rgb(0xFF, 0xFF, 0x00);
	colors[3] = rgb(0x00, 0xFF, 0x00);
	colors[4] = rgb(0x00, 0xFF, 0xFF);
	colors[5] = rgb(0x00, 0x7F, 0xFF);
	colors[6] = rgb(0x00, 0x00, 0xFF);
	colors[7] = rgb(0x7F, 0x00, 0xFF);
	colors[8] = rgb(0xFF, 0x00, 0xFF);
	colors[9] = rgb(0xFF, 0x00, 0x7F);

    QuadTree tree(width, height, 10, 10);
	int ballMin = 10;
	int ballMax = 10;
    for (int i = 0; i < 500; i++)
        tree.addBall(width / 2, height / 2, ballMin + rand() % (ballMax - ballMin + 1), i % 10);
    //tree.addBall(rand() % (width - 2 * ballRad) + ballRad, rand() % (height - 2 * ballRad) + ballRad, ballRad);
    
	bool running = true; 
	while (running)
	{
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
			case SDL_EVENT_QUIT:
				running = false;
				break;

			case SDL_EVENT_WINDOW_RESIZED:
				surface = SDL_GetWindowSurface(window);
				width = surface->w;
				height = surface->h;
				break;

            case SDL_EVENT_KEY_DOWN:
                switch (e.key.key)
                {
                    case SDLK_0: interact = 0; break;
                    case SDLK_1: interact = 1; break;
                    case SDLK_2: interact = -1; break;
                    case SDLK_3: interact = 2; break;
                    case SDLK_4: interact = -2; break;

					case SDLK_RIGHT: ups = 120; break;
					case SDLK_LEFT: ups = 10; break;
                }
                break;

			case SDL_EVENT_KEY_UP:
				switch (e.key.key)
				{
					case SDLK_RIGHT:
					case SDLK_LEFT: ups = 60; break;
				}
				break;
			}
		}

		getMousePosRel(window, &mouseXR, &mouseYR);
                
		Uint64 now = SDL_GetTicks();

		deltaTime += now - lastTime;
		while (deltaTime >= 1000 / ups)
		{
			deltaTime -= 1000 / ups;
			ticks++;
            if (interact == 2)
                tree.addBall(mouseXR, mouseYR, ballMin + rand() % (ballMax - ballMin + 1), rand() % 10);
            tree.update();
		}
		lastTime = now;

		Uint32 fpsScale = 1;
		if (now - lastFPSTime >= 1000 / fpsScale)
		{
			lastFPSTime = now;
			fps = fpsCount * fpsScale;
			fpsCount = 0;
		}

		clearScreen(surface, rgb(0x00, 0x00, 0x00));

        tree.draw(surface);
        int ballCount = tree.getBallCount();

        
		// Info display
		drawTextFFn(surface, 10, 10, 1, rgb(0xFF, 0xFF, 0xFF), textBG, NULL, "FPS       (%9u)", fps);

		drawTextFFn(surface, 10, 30, 1, rgb(0xFF, 0xFF, 0xFF), textBG, NULL, "Size      (%4d %4d)", width, height);
        
		drawTextFFn(surface, 10, 50, 1, rgb(0xFF, 0xFF, 0xFF), textBG, NULL, "Count     (%9d)", ballCount);

		drawTextFFn(surface, 10, 70, 1, rgb(0xFF, 0xFF, 0xFF), textBG, NULL, "Mode      (%9s)",
            interact == 1 ? "Attract" :
            interact == -1 ? "Repel" :
            interact == -2 ? "Delete" :
            interact == 2 ? "Place" :
            "None");

		drawTextFFn(surface, 10, 90, 1, rgb(0xFF, 0xFF, 0xFF), textBG, NULL, "TPS       (%9llu)", ups);
        
		SDL_UpdateWindowSurface(window);
		fpsCount++;
	}

	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}

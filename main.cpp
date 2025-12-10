// This uses my homemade pixel graphics library
// This beats SDL's SDL_FillSurfaceRect() tho
// Circles are just as fast as rectangles

#include "SDL.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define USE_SSE
#include "graphics.h"

#include "QuadTree.h"

void getMousePos(Sint32* mouseX, Sint32* mouseY)
{
	int x = 0, y = 0;
	SDL_GetMouseState(&x, &y);
	*mouseX = x;
	*mouseY = y;
}

int main()
{
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_NOPARACHUTE);

    srand(time(NULL));

	Uint32 width = 1200, height = 500;

	SDL_Surface* surface = SDL_SetVideoMode(width, height, 32, SDL_ANYFORMAT | SDL_DOUBLEBUF);
	pixFmt = surface->format;
    SDL_WM_SetCaption("Balls", NULL);

	Uint64 lastTime = SDL_GetTicks();
	Uint64 deltaTime = 0;
	Uint64 ups = 30;
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

	int sidebarW = 200;
	int maxDepth = 10;
    QuadTree tree(width - sidebarW, height, 10, maxDepth);
	int ballMin = 2;
	int ballMax = 5;
    for (int i = 0; i < 2000; i++)
        tree.addBall(width / 2, height / 2, ballMin + rand() % (ballMax - ballMin + 1), rand() % 2);
    
	bool useTree = true;
	bool drawTree = true;
	bool colorSpeed = true;

	float gravity = 0.f;
	float friction = 0.005f;
	Sint32 mouseXR = 0;
	Sint32 mouseYR = 0;
	int interact = 0;
	
	bool running = true; 
	while (running)
	{
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
			case SDL_QUIT:
				running = false;
				break;

            case SDL_KEYDOWN:
                switch (e.key.keysym.sym)
                {
                    case SDLK_0: interact = 0; break; // changes mode to no interaction
                    case SDLK_1: interact = 1; break; // attracts balls to mouse
                    case SDLK_2: interact = -1; break; // repels balls from mouse
                    case SDLK_3: interact = 2; break; // places balls at mouse
                    case SDLK_4: interact = -2; break; // removes balls around mouse
                    case SDLK_5: interact = -3; break; // attracts balls to mouse and removes balls in center

					// speeds up or slows down simulation
					case SDLK_UP: ups = 120; break; 
					case SDLK_RIGHT: ups = 60; break;
					case SDLK_LEFT: ups = 30; break;
					case SDLK_DOWN: ups = 10; break; 

					case SDLK_TAB: // changes between using tree normally and using as 0 depth (same as pairwise)
						useTree = !useTree; 
						tree.setMaxDepth(useTree ? maxDepth : 0);
						break;

					case SDLK_g:
						if (gravity == 0.f)
							gravity = 0.05f;
						else
							gravity = 0.f;
						break;
					case SDLK_f:
						if (friction == 0.f)
							friction = 0.005f;
						else
							friction = 0.f;
						break;
					case SDLK_d:
						drawTree = !drawTree;
						break;

					case SDLK_s:
						colorSpeed = !colorSpeed;
						break;
                }
                break;
			}
		}

		getMousePos(&mouseXR, &mouseYR);
                
		Uint64 now = SDL_GetTicks();

		deltaTime += now - lastTime;
		while (deltaTime >= 1000 / ups)
		{
			deltaTime -= 1000 / ups;
			ticks++;
            if (interact == 2)
                tree.addBall(mouseXR, mouseYR, ballMin + rand() % (ballMax - ballMin + 1), rand() % 10);
            tree.update(interact, mouseXR, mouseYR, friction, gravity);
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

        tree.draw(surface, drawTree && useTree, colorSpeed);
        int ballCount = tree.getBallCount();


		drawRect(surface, width - sidebarW, 0, sidebarW, height, rgb(0x0F, 0x0F, 0x0F));
        
		// Info display
		drawTextF(surface, width - sidebarW, 10, 1, rgb(0xFF, 0xFF, 0xFF), "FPS:%12u", fps);

		drawTextF(surface, width - sidebarW, 30, 1, rgb(0xFF, 0xFF, 0xFF), "Size:%5d %5d", width - sidebarW, height);
        
		drawTextF(surface, width - sidebarW, 50, 1, rgb(0xFF, 0xFF, 0xFF), "Count:%10d", ballCount);

		drawTextF(surface, width - sidebarW, 70, 1, rgb(0xFF, 0xFF, 0xFF), "Interact:%7s",
            interact == 1 ? "Attract" :
            interact == -1 ? "Repel" :
            interact == -2 ? "Delete" :
            interact == 2 ? "Place" :
            interact == -3 ? "Vacuum" :
            "None");

		drawTextF(surface, width - sidebarW, 90, 1, rgb(0xFF, 0xFF, 0xFF), "TPS:%12llu", ups);
        
		drawText(surface, width - sidebarW, 130, 1, rgb(0xFF, 0xFF, 0xFF),
			"Controls\n\n"
			" 0 No Interact\n"
			" 1 Attract balls\n"
			" 2 Repel balls\n"
			" 3 Place balls\n"
			" 4 Delete balls\n"
			" 5 Vacuum balls\n\n"
			" Speed");
		drawTextA(surface, width - sidebarW, 130 + 10 * font_h, 0.75, 2, 0, rgb(0xFF, 0xFF, 0xFF), 
		"  /\\");
		drawText(surface, width - sidebarW, 130 + 10 * font_h, 1, rgb(0xFF, 0xFF, 0xFF),
			"   Fast\n"
			" > Normal\n"
			" < Slow");
		drawTextA(surface, width - sidebarW, 130 + 13 * font_h, 0.75, 2, 0, rgb(0xFF, 0xFF, 0xFF),
		"  \\/");
		drawText(surface, width - sidebarW, 130 + 13 * font_h, 1, rgb(0xFF, 0xFF, 0xFF),
			"   Snail");

		drawText(surface, width - sidebarW, 130 + 15 * font_h, 1, rgb(0xFF, 0xFF, 0xFF),
			" Tab");
		drawText(surface, width - sidebarW, 130 + 15 * font_h, 1, 
			useTree ? rgb(0x00, 0xFF, 0x00) : rgb(0xFF, 0x00, 0x00), "     Toggle Tree");

		drawText(surface, width - sidebarW, 130 + 16 * font_h, 1, rgb(0xFF, 0xFF, 0xFF), " D");
		drawText(surface, width - sidebarW, 130 + 16 * font_h, 1,
			drawTree ? rgb(0x00, 0xFF, 0x00) : rgb(0xFF, 0x00, 0x00), "   Draw Tree");

		drawText(surface, width - sidebarW, 130 + 17 * font_h, 1, rgb(0xFF, 0xFF, 0xFF), " S");
		drawText(surface, width - sidebarW, 130 + 17 * font_h, 1,
			colorSpeed ? rgb(0x00, 0xFF, 0x00) : rgb(0xFF, 0x00, 0x00), "   Color Speed");

		drawText(surface, width - sidebarW, 130 + 19 * font_h, 1, rgb(0xFF, 0xFF, 0xFF), " G");
		drawText(surface, width - sidebarW, 130 + 19 * font_h, 1,
			gravity ? rgb(0x00, 0xFF, 0x00) : rgb(0xFF, 0x00, 0x00), "   Gravity");

		drawText(surface, width - sidebarW, 130 + 20 * font_h, 1, rgb(0xFF, 0xFF, 0xFF), " F");
		drawText(surface, width - sidebarW, 130 + 20 * font_h, 1,
			friction ? rgb(0x00, 0xFF, 0x00) : rgb(0xFF, 0x00, 0x00), "   Friction");

		SDL_Flip(surface);
		fpsCount++;
	}

	SDL_FreeSurface(surface);
	SDL_Quit();

	return 0;
}

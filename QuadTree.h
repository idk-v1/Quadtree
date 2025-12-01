#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define PI 3.14159f
#define TO_RAD(x) ((x) * PI / 180.f)
#define TO_DEG(x) ((x) * 180.f / PI)

#define USE_SSE
#include "graphics.h"

#include <vector>


Uint32 colors[10];

float fast_atan2(float y, float x)
{
	//http://pubs.opengroup.org/onlinepubs/009695399/functions/atan2.html
	//Volkan SALMA

	const float ONEQTR_PI = 3.1415f / 4.f;
	const float THRQTR_PI = 3.f * 3.1415f / 4.f;
	float r, angle;
	float abs_y = fabsf(y) + 1e-10f;      // kludge to prevent 0/0 condition
	if (x < 0.0f)
	{
		r = (x + abs_y) / (abs_y - x);
		angle = THRQTR_PI;
	}
	else
	{
		r = (x - abs_y) / (x + abs_y);
		angle = ONEQTR_PI;
	}
	angle += (0.1963f * r * r - 0.9817f) * r;
	if (y < 0.0f)
		return(-angle);     // negate if in quad III or IV
	else
		return(angle);
}

float fast_sqrt(float x)
{	
	union
	{
		int i;
		float x;
	} u;
	u.x = x;
	u.i = (1 << 29) + (u.i >> 1) - (1 << 22);

	// Two Babylonian Steps (simplified from:)
	// u.x = 0.5f * (u.x + x/u.x);
	// u.x = 0.5f * (u.x + x/u.x);
	u.x = u.x + x / u.x;
	u.x = 0.25f * u.x + x / u.x;

	return u.x;
}

float powf_2(float x)
{
    return x * x;
}

static float gravity = 0.05f;
static float friction = 0.01f;
static Sint32 mouseXR = 0;
static Sint32 mouseYR = 0;
static int interact = 0;

struct Ball
{
    float x, y, rad, vx, vy;
    int type;
    int colCount = 0;
    bool reqDel = false;
    bool onWall = false;

    Ball(float x, float y, float rad, int type) :
        x(x), y(y), rad(rad), type(type)
    {
        float r = rand() / (float)RAND_MAX * 360.f;
        float vel = 2.f;
        vx = cosf(TO_RAD(r)) * vel;
        vy = sinf(TO_RAD(r)) * vel;
    }

    // Move circle according to velocity
    // If hit wall, move back and flip velocity
    void update(int width, int height)
    {   
        vy += gravity;
        float vel = fast_sqrt(vx * vx + vy * vy);
        float vAngle = fast_atan2(vy, vx);

        vel *= 1 - friction;
        float maxVel = 10.f;
        if (vel > maxVel)
            vel = maxVel;
        
        vx = cosf(vAngle) * vel;
        vy = sinf(vAngle) * vel;

        int strength = 200;
        int strSq = strength * strength;
        float mouseDist = powf_2(mouseXR - x) + powf_2(mouseYR - y);
        if (mouseDist < strSq)
        {
            if (interact == -2 && mouseDist < strSq * 0.25f)
                reqDel = true;
            else if (interact == 1 || interact == -1)
            {
                mouseDist = 1 - fast_sqrt(mouseDist) / (float)strength;

                float mouseAngle = fast_atan2(mouseYR - y, mouseXR - x);
                if (interact == -1)
                    mouseDist = -mouseDist;
            
                vx += cosf(mouseAngle) * mouseDist;
                vy += sinf(mouseAngle) * mouseDist;
            }
        }
        
        x += vx;
        y += vy;
        
        onWall = false;
        if (x - rad < 0)
        {
            vx = -vx;
            x -= x - rad;
            onWall = true;
        }
        if (y - rad < 0)
        {
            vy = -vy;
            y -= y - rad;
            onWall = true;
        }
        if (x + rad >= width)
        {
            vx = -vx;
            x -= x + rad - width + 1;
            onWall = true;
        }
        if (y + rad >= height)
        {
            vy = -vy;
            y -= y + rad - height + 1;
            onWall = true;
        }

        colCount = 0;
    }

    // Draw white if colliding
    // Depth color otherwise
    void draw(SDL_Surface* surface, int depth)
    {
        //drawCircle(surface, x, y, rad, wasCol ? rgb(0xFF, 0xFF, 0xFF) : colors[depth % 10]);
        drawCircle(surface, x, y, rad, colors[type]);
    }

    // Circle circle collision
    // If they collide, change both
    // Trying to compare a lower depth circle to a higher depth circle would be hard,
    // So doing both parts of the collision in 1 is way easier
    void collide(Ball& o, int width, int height)
    {
        float dist = powf_2(x - o.x) + powf_2(y - o.y);
        if (dist <= powf_2(rad + o.rad))
        {
            float angle = fast_atan2(y - o.y, x - o.x);

            float sqrt = fast_sqrt(dist);
            float invDist = 1 - sqrt / (float)(rad + o.rad);
            
            float cos = cosf(angle);
            float sin = sinf(angle);

            vx += cos * invDist;
            vy += sin * invDist;
            x += cos * invDist;
            y += sin * invDist;

            o.vx += -cos * invDist;
            o.vy += -sin * invDist;
            o.x += cos * -invDist;
            o.y += sin * -invDist;
            
            colCount++;
            o.colCount++;
        }
    }
};

struct QuadNode
{
    QuadNode* parent;
    QuadNode* quad[4];
    std::vector<Ball> balls;
    int x, y, width, height, maxBalls, maxDepth, depth;

    QuadNode(QuadNode* parent, int x, int y, int width, int height, int maxBalls, int maxDepth, int depth) :
        parent(parent), x(x), y(y), width(width), height(height), maxBalls(maxBalls), maxDepth(maxDepth), depth(depth)
    {
        quad[0] = NULL;
        quad[1] = NULL;
        quad[2] = NULL;
        quad[3] = NULL;
    }

    ~QuadNode()
    {
        if (quad[0])
        {
            delete quad[0];
            delete quad[1];
            delete quad[2];
            delete quad[3];
        }
        balls.clear();
    }

    void draw(SDL_Surface* surface)
    {   
        if (quad[0])
        {
            quad[0]->draw(surface);
            quad[1]->draw(surface);
            quad[2]->draw(surface);
            quad[3]->draw(surface);
        }

        drawRectOut(surface, x, y, width, height, 1, colors[depth % 10]);
        
        for (int i = 0; i < balls.size(); i++)
            balls[i].draw(surface, depth);
    }

    int getBallCount()
    {
        int count = 0;
        if (quad[0])
        {
            count += quad[0]->getBallCount();
            count += quad[1]->getBallCount();
            count += quad[2]->getBallCount();
            count += quad[3]->getBallCount();
        }
        count += balls.size();
        return count;
    }

    // This will only be called for root
    // Just place it in root for now
    // After one update, the balls will be placed where they need to go
    void addBall(float x, float y, float rad, int type)
    {
        balls.push_back(Ball(x, y, rad, type));
    }

    // Moves circles according to velocities
    void update(int w, int h)
    {
        if (quad[0])
            for (int i = 0; i < 4; i++)
                quad[i]->update(w, h);

        for (int i = balls.size() - 1; i >= 0; i--)
        {
            if (balls[i].reqDel)
            {
                balls[i] = balls.back();
                balls.pop_back();
            }
            else
                balls[i].update(w, h);
        }
    }

    // Attempts to move circles up the tree if they aren't entirely contained in quad
    void tryMove()
    {
        int w2 = width / 2;
        int h2 = height / 2;
        int x0 = this->x;
        int y0 = this->y;
        int x1 = this->x + w2;
        int y1 = this->y + h2;
        int x2 = this->x + width;
        int y2 = this->y + height;

        if (quad[0])
            for (int i = 0; i < 4; i++)
                quad[i]->tryMove();

        if (depth > 0)
        {
            for (int i = balls.size() - 1; i >= 0; i--)
            {
                int bx = balls[i].x;
                int by = balls[i].y;
                int br = balls[i].rad;

                // If square around circle is not entirely contained in quad, move up
                if (bx - br < x0 || by - br < y0 || bx + br >= x2 || by + br >= y2)
                {
                    parent->balls.push_back(balls[i]);
                    balls[i] = balls.back();
                    balls.pop_back();
                }
            }
        }
    }

    // Attempts to subdivide circles into child quads
    // Try to move balls back down
    // They get stuck after they move up from crossing a border,
    // but there aren't enough to be force moved down
    void trySubdivide()
    {
        int w2 = width / 2;
        int h2 = height / 2;
        int x0 = this->x;
        int y0 = this->y;
        int x1 = this->x + w2;
        int y1 = this->y + h2;
        int x2 = this->x + width;
        int y2 = this->y + height;

        if (balls.size() > maxBalls && depth < maxDepth)
        {
            // Create child quads if need to
            if (!quad[0])
            {
                quad[0] = new QuadNode(this, x0, y0, w2, h2, maxBalls, maxDepth, depth + 1);
                quad[1] = new QuadNode(this, x1, y0, w2, h2, maxBalls, maxDepth, depth + 1);
                quad[2] = new QuadNode(this, x0, y1, w2, h2, maxBalls, maxDepth, depth + 1);
                quad[3] = new QuadNode(this, x1, y1, w2, h2, maxBalls, maxDepth, depth + 1);
            }
        }

        if (quad[0])
        {
            for (int i = balls.size() - 1; i >= 0; i--)
            {
                int br = balls[i].rad;
                int bx = balls[i].x;
                int by = balls[i].y;

                if (bx - br >= x0 && by - br >= y0 && bx + br < x1 && by + br < y1)
                    quad[0]->balls.push_back(balls[i]);
                else if (bx - br >= x1 && by - br >= y0 && bx + br < x2 && by + br < y1)
                    quad[1]->balls.push_back(balls[i]);
                else if (bx - br >= x0 && by - br >= y1 && bx + br < x1 && by + br < y2)
                    quad[2]->balls.push_back(balls[i]);
                else if (bx - br >= x1 && by - br >= y1 && bx + br < x2 && by + br < y2)
                    quad[3]->balls.push_back(balls[i]);
                else
                    continue;
                
                balls[i] = balls.back();
                balls.pop_back();
            }
            
            for (int i = 0; i < 4; i++)
                quad[i]->trySubdivide();
        }
    }

    // If number of circles in child quads is less than the split limit,
    // move them up into this quad
    void tryUndivide()
    {
        if (quad[0])
        {
            for (int i = 0; i < 4; i++)
                quad[i]->tryUndivide();

            if (getBallCount() <= maxBalls)
            {
                for (int i = 0; i < 4; i++)
                {
                    for (int ii = 0; ii < quad[i]->balls.size(); ii++)
                        balls.push_back(quad[i]->balls[ii]);
                    delete quad[i];
                    quad[i] = NULL;
                }
            }
        }
    }

    // n^2 collision with circles in this quad only
    void collide(int width, int height)
    {
        for (int i = 0; i < balls.size(); i++)
        {
            for (int ii = i + 1; ii < balls.size(); ii++)
                balls[i].collide(balls[ii], width, height);
        }
    }

    // Collision with a circle with all circles in child quads
    void collideWith(Ball& ball, int width, int height)
    {
        if (quad[0])
        {
            for (int i = 0; i < 4; i++)
                quad[i]->collideWith(ball, width, height);
        }

        for (int i = 0; i < balls.size(); i++)
        {
            ball.collide(balls[i], width, height);
        }
    }

    // Collision with local circles,
    // Then collision with each circle in this quad with all circles in child quads
    void tryCollide(int width, int height)
    {
        collide(width, height);
        
        if (quad[0])
        {
            for (int i = 0; i < balls.size(); i++)
                for (int ii = 0; ii < 4; ii++)
                    quad[ii]->collideWith(balls[i], width, height);
            
            for (int i = 0; i < 4; i++)
            {
                quad[i]->tryCollide(width, height);
            }
        }   
    }
};

struct QuadTree
{
    QuadNode root;

    QuadTree(int width, int height, int maxBalls, int maxDepth) :
        root(NULL, 0, 0, width, height, maxBalls, maxDepth, 0) {}

    // Probably don't have to do these steps seperately,
    // But it works fine, so I'm keeping it
    void update()
    {
        root.update(root.width, root.height);
        root.tryMove();
        root.trySubdivide();
        root.tryUndivide();
        root.tryCollide(root.width, root.height);
    }

    void draw(SDL_Surface* surface)
    {
        root.draw(surface);
    }

    int getBallCount()
    {
        return root.getBallCount();
    }

    void addBall(float x, float y, float rad, int type)
    {
        root.addBall(x, y, rad, type);
    }
};

#include <SDL2/SDL.h>
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>


struct Circle {
    float x, y;        
    float vx, vy;      
    int radius;        
    Uint8 r, g, b;     
};


void drawCircle(SDL_Renderer* renderer, int centerX, int centerY, int radius) {
    for (int w = 0; w < radius * 2; w++) {
        for (int h = 0; h < radius * 2; h++) {
            int dx = radius - w; 
            int dy = radius - h; 
            if ((dx*dx + dy*dy) <= (radius * radius)) {
                SDL_RenderDrawPoint(renderer, centerX + dx, centerY + dy);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    
    int numCircles = 50;  
    if (argc > 1) {
        numCircles = atoi(argv[1]);
        if (numCircles <= 0) numCircles = 50;
    }

    
    const int WINDOW_WIDTH = 800;
    const int WINDOW_HEIGHT = 600;

    
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Error inicializando SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    
    SDL_Window* window = SDL_CreateWindow(
        "SDL Screensaver",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cerr << "Error creando ventana: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Error creando renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    
    srand(time(nullptr));

    
    std::vector<Circle> circles(numCircles);
    for (int i = 0; i < numCircles; i++) {
        circles[i].x = rand() % WINDOW_WIDTH;
        circles[i].y = rand() % WINDOW_HEIGHT;
        circles[i].vx = (rand() % 200 - 100) / 50.0f; 
        circles[i].vy = (rand() % 200 - 100) / 50.0f;
        circles[i].radius = rand() % 20 + 10; 
        circles[i].r = rand() % 256;
        circles[i].g = rand() % 256;
        circles[i].b = rand() % 256;
    }

    
    Uint32 frameStart, frameTime;
    const int FPS = 60;
    const int frameDelay = 1000 / FPS;

    
    bool running = true;
    SDL_Event event;

    std::cout << "Screensaver iniciado con " << numCircles << " círculos" << std::endl;

    while (running) {
        frameStart = SDL_GetTicks();

        
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    running = false;
                }
            }
        }

        
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        
        for (int i = 0; i < numCircles; i++) {
            Circle& circle = circles[i];

            
            circle.x += circle.vx;
            circle.y += circle.vy;

            
            if (circle.x - circle.radius <= 0 || circle.x + circle.radius >= WINDOW_WIDTH) {
                circle.vx = -circle.vx;
                
                if (circle.x - circle.radius <= 0) circle.x = circle.radius;
                if (circle.x + circle.radius >= WINDOW_WIDTH) circle.x = WINDOW_WIDTH - circle.radius;
            }
            if (circle.y - circle.radius <= 0 || circle.y + circle.radius >= WINDOW_HEIGHT) {
                circle.vy = -circle.vy;
                
                if (circle.y - circle.radius <= 0) circle.y = circle.radius;
                if (circle.y + circle.radius >= WINDOW_HEIGHT) circle.y = WINDOW_HEIGHT - circle.radius;
            }

            
            SDL_SetRenderDrawColor(renderer, circle.r, circle.g, circle.b, 255);

            
            drawCircle(renderer, (int)circle.x, (int)circle.y, circle.radius);
        }

        
        SDL_RenderPresent(renderer);

        
        frameTime = SDL_GetTicks() - frameStart;
        if (frameDelay > frameTime) {
            SDL_Delay(frameDelay - frameTime);
        }

        
        static Uint32 lastTime = 0;
        static int frameCount = 0;
        frameCount++;
        
        if (SDL_GetTicks() - lastTime >= 1000) {
            std::cout << "FPS: " << frameCount << std::endl;
            frameCount = 0;
            lastTime = SDL_GetTicks();
        }
    }

    
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
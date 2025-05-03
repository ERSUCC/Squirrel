#pragma once

#include <chrono>
#include <mutex>

#include <SDL.h>

struct GUI
{
    GUI();
    ~GUI();

    void run();
    void render();

private:
    std::mutex lock;

    SDL_Window* window;
    SDL_Renderer* renderer;

    int width = 500;
    int height = 500;

    std::chrono::high_resolution_clock clock;
    std::chrono::time_point<std::chrono::high_resolution_clock> lastFrame;

};

int eventWatch(void* userdata, SDL_Event* event);

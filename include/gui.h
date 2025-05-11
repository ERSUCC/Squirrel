#pragma once

#include <chrono>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>

#include <SDL.h>

#include "network.h"

struct GUI
{
    GUI();
    ~GUI();

    void setupEmpty();
    void setupSend(const std::string path);

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

    Socket* socket;

};

int eventWatch(void* userdata, SDL_Event* event);

#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include <SDL.h>
#include <SDL_ttf.h>

#include "network.h"
#include "files.h"

struct GUI
{
    GUI(NetworkManager* networkManager, FileManager* fileManager);
    ~GUI();

    void setupEmpty();
    void setupSend(const std::string path);

    void run();
    void render();

private:
    void handleResponse(const std::string ip);

    std::mutex renderLock;

    SDL_Window* window;
    SDL_Renderer* renderer;

    int width = 500;
    int height = 500;

    TTF_Font* font;

    std::chrono::high_resolution_clock clock;
    std::chrono::time_point<std::chrono::high_resolution_clock> lastFrame;

    NetworkManager* networkManager;
    FileManager* fileManager;

    std::vector<std::string> availableTargets;

    std::string path;

};

int eventWatch(void* userdata, SDL_Event* event);

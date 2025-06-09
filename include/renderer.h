#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <SDL.h>
#include <SDL_ttf.h>

#include "errors.h"
#include "gui.h"
#include "network.h"
#include "files.h"
#include "safe_queue.hpp"

struct TargetButton : public Button
{
    TargetButton(SDL_Renderer* renderer, const std::string name, const std::string ip);

    const std::string name;
    const std::string ip;
};

struct Renderer
{
    Renderer(ErrorHandler* errorHandler, NetworkManager* networkManager, FileManager* fileManager);
    ~Renderer();

    void setupEmpty();
    void setupSend(const std::string path);

    void run();
    void render();

    void resized(const unsigned int width, const unsigned int height);

private:
    void handleResponse(const std::string name, const std::string ip);

    std::mutex renderLock;

    SDL_Window* window;
    SDL_Renderer* renderer;

    int width = 500;
    int height = 500;

    float scale;

    TTF_Font* font;

    std::chrono::high_resolution_clock clock;
    std::chrono::time_point<std::chrono::high_resolution_clock> lastFrame;

    ErrorHandler* errorHandler;
    NetworkManager* networkManager;
    FileManager* fileManager;

    ThreadSafeQueue<std::function<void()>>* mainThreadQueue = new ThreadSafeQueue<std::function<void()>>();

    Layout* root;

    std::vector<TargetButton*> targets;

    std::string path;

};

int eventWatch(void* userdata, SDL_Event* event);

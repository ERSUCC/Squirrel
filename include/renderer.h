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
#include "service.h"

struct Target
{
    Target(const GUIObject* object, const std::string name, const std::string ip);

    const GUIObject* object;

    const std::string name;
    const std::string ip;
};

struct Renderer
{
    Renderer(ThreadSafeQueue<std::function<void()>>* mainThreadQueue, ErrorHandler* errorHandler, NetworkManager* networkManager, FileManager* fileManager, ServiceManager* serviceManager);
    ~Renderer();

    void setPath(const std::string path);

    void setupMain();
    void setupReceive(const std::string name, const std::string& data);

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

    ThreadSafeQueue<std::function<void()>>* mainThreadQueue;

    ErrorHandler* errorHandler;
    NetworkManager* networkManager;
    FileManager* fileManager;
    ServiceManager* serviceManager;

    Layout* root;

    std::vector<Target*> targets;

    std::string path;

};

bool eventWatch(void* userdata, SDL_Event* event);

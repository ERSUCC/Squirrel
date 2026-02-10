#pragma once

#include <filesystem>
#include <functional>
#include <string>

#include <SDL.h>

struct FileManager
{
    typedef const std::function<void(const std::filesystem::path)> SelectHandler;

    virtual void getSelectPath(SDL_Window* parent, SelectHandler complete) const = 0;
    virtual void getSavePath(SDL_Window* parent, const std::string name, SelectHandler complete) const = 0;

    virtual std::filesystem::path getResourcePath(const std::string name) const = 0;
};

#ifdef _WIN32

#include <combaseapi.h>
#include <ShObjIdl.h>
#include <Windows.h>

struct WinFileManager : public FileManager
{
    void getSelectPath(SDL_Window* parent, SelectHandler complete) const override;
    void getSavePath(SDL_Window* parent, const std::string name, SelectHandler complete) const override;

    std::filesystem::path getResourcePath(const std::string name) const override;
};

#elif __APPLE__

struct MacFileManager : public FileManager
{
    void getSelectPath(SDL_Window* parent, SelectHandler complete) const override;
    void getSavePath(SDL_Window* parent, const std::string name, SelectHandler complete) const override;

    std::filesystem::path getResourcePath(const std::string name) const override;
};

#else

#include <limits.h>
#include <stdio.h>

struct LinuxFileManager : public FileManager
{
    void getSelectPath(SDL_Window* parent, SelectHandler complete) const override;
    void getSavePath(SDL_Window* parent, const std::string name, SelectHandler complete) const override;

    std::filesystem::path getResourcePath(const std::string name) const override;
};

#endif

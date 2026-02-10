#include "../include/files.h"

#ifdef _WIN32

void WinFileManager::getSelectPath(SDL_Window* parent, SelectHandler complete) const
{
    SDL_PropertiesID properties = SDL_GetWindowProperties(parent);

    if (!properties)
    {
        complete("");

        return;
    }

    HWND hwnd = (HWND)SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);

    IFileDialog* dialog;

    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)))
    {
        complete("");

        return;
    }

    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_IFileOpenDialog, (void**)&dialog)))
    {
        CoUninitialize();

        complete("");

        return;
    }

    dialog->SetTitle(L"Select File");

    if (FAILED(dialog->Show(hwnd)))
    {
        dialog->Release();

        CoUninitialize();

        complete("");

        return;
    }

    IShellItem* item;

    if (FAILED(dialog->GetResult(&item)))
    {
        dialog->Release();

        CoUninitialize();

        complete("");

        return;
    }

    PWSTR path;

    item->GetDisplayName(SIGDN_FILESYSPATH, &path);

    dialog->Release();

    CoUninitialize();

    complete(path);
}

void WinFileManager::getSavePath(SDL_Window* parent, const std::string name, SelectHandler complete) const
{
    SDL_PropertiesID properties = SDL_GetWindowProperties(parent);

    if (!properties)
    {
        complete("");

        return;
    }

    HWND hwnd = (HWND)SDL_GetPointerProperty(properties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);

    IFileDialog* dialog;

    if (FAILED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)))
    {
        complete("");

        return;
    }

    if (FAILED(CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_IFileSaveDialog, (void**)&dialog)))
    {
        CoUninitialize();

        complete("");

        return;
    }

    dialog->SetTitle(L"Save File");
    dialog->SetFileName(std::wstring(name.begin(), name.end()).c_str());

    if (FAILED(dialog->Show(hwnd)))
    {
        dialog->Release();

        CoUninitialize();

        complete("");

        return;
    }

    IShellItem* item;

    if (FAILED(dialog->GetResult(&item)))
    {
        dialog->Release();

        CoUninitialize();

        complete("");

        return;
    }

    PWSTR path;

    item->GetDisplayName(SIGDN_FILESYSPATH, &path);

    dialog->Release();

    CoUninitialize();

    complete(path);
}

std::filesystem::path WinFileManager::getResourcePath(const std::string name) const
{
    #ifdef SQUIRREL_RELEASE

    char path[MAX_PATH + 1];

    if (GetModuleFileName(nullptr, path, MAX_PATH))
    {
        return std::filesystem::path(std::filesystem::path(path).parent_path() / "resources" / name);
    }

    #endif

    return std::filesystem::path("resources") / name;
}

#elif __linux__

void LinuxFileManager::getSelectPath(SDL_Window* parent, SelectHandler complete) const
{
    const std::string cmd = std::string("zenity --file-selection --title \"Select File\"");

    FILE* proc = popen(cmd.c_str(), "r");

    char buffer[PATH_MAX + 1];

    unsigned int read = fread(buffer, sizeof(char), PATH_MAX, proc);

    pclose(proc);

    if (read == 0)
    {
        complete("");

        return;
    }

    buffer[read - 1] = '\0';

    complete(buffer);
}

void LinuxFileManager::getSavePath(SDL_Window* parent, const std::string name, SelectHandler complete) const
{
    const std::string cmd = std::string("zenity --file-selection --save --title \"Save File\" --filename \"") + name + "\"";

    FILE* proc = popen(cmd.c_str(), "r");

    char buffer[PATH_MAX + 1];

    unsigned int read = fread(buffer, sizeof(char), PATH_MAX, proc);

    pclose(proc);

    if (read == 0)
    {
        complete("");

        return;
    }

    buffer[read - 1] = '\0';

    complete(buffer);
}

std::filesystem::path LinuxFileManager::getResourcePath(const std::string name) const
{
    #ifdef SQUIRREL_RELEASE

    return std::filesystem::canonical("/proc/self/exe").parent_path() / "resources" / name;

    #else

    return std::filesystem::path("resources") / name;

    #endif
}

#endif

#include "../include/files.h"

#ifdef _WIN32

std::filesystem::path WinFileManager::getSavePath(const std::string name) const
{
    IFileDialog* dialog;

    if (!SUCCEEDED(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)))
    {
        return "";
    }

    HRESULT r = CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER, IID_IFileSaveDialog, (void**)&dialog);

    if (!SUCCEEDED(r))
    {
        CoUninitialize();

        return "";
    }

    dialog->SetTitle(L"Save File");
    dialog->SetFileName(std::wstring(name.begin(), name.end()).c_str());

    if (!SUCCEEDED(dialog->Show(nullptr)))
    {
        dialog->Release();

        CoUninitialize();

        return "";
    }

    IShellItem* item;

    if (!SUCCEEDED(dialog->GetResult(&item)))
    {
        dialog->Release();

        CoUninitialize();

        return "";
    }

    PWSTR path;

    item->GetDisplayName(SIGDN_FILESYSPATH, &path);

    dialog->Release();

    CoUninitialize();

    return std::filesystem::path(path);
}

std::filesystem::path WinFileManager::getResourcePath(const std::string name) const
{
    return std::filesystem::path("resources/" + name);
}

#elif __linux__

std::filesystem::path LinuxFileManager::getSavePath(const std::string name) const
{
    const std::string cmd = std::string("zenity --file-selection --save --title \"Save File\" --filename \"") + name + "\"";

    FILE* proc = popen(cmd.c_str(), "r");

    char buffer[PATH_MAX + 1];

    unsigned int read = fread(buffer, sizeof(char), PATH_MAX, proc);

    pclose(proc);

    if (read == 0)
    {
        return "";
    }

    buffer[read - 1] = '\0';

    return buffer;
}

std::filesystem::path LinuxFileManager::getResourcePath(const std::string name) const
{
    return std::filesystem::path("resources/" + name);
}

#endif

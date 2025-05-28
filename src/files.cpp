#include "../include/files.h"

#ifdef _WIN32

std::filesystem::path WinFileManager::getSavePath(const std::string name) const
{
    IFileDialog* dialog;

    if (!SUCCEEDED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)))
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

#elif __APPLE__

std::filesystem::path MacFileManager::getSavePath(const std::string name) const
{
    return "";
}

#else

std::filesystem::path LinuxFileManager::getSavePath(const std::string name) const
{
    return "";
}

#endif

#include "gui.h"

GUI::GUI(ErrorHandler* errorHandler, NetworkManager* networkManager, FileManager* fileManager) :
    errorHandler(errorHandler), networkManager(networkManager), fileManager(fileManager)
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(width, height, SDL_WINDOW_RESIZABLE, &window, &renderer);
    SDL_SetWindowTitle(window, "Squirrel");
    SDL_AddEventWatch(eventWatch, this);

    TTF_Init();

    font = TTF_OpenFont("resources/fonts/OpenSans-Variable.ttf", 12);
}

GUI::~GUI()
{
    TTF_CloseFont(font);
    TTF_Quit();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void GUI::setupEmpty()
{
    networkManager->beginListen([=](const std::string name, const std::string& data)
    {
        mainThreadQueue->push([=]()
        {
            const std::filesystem::path path = fileManager->getSavePath(name);

            std::ofstream file(path);

            if (!file.is_open())
            {
                errorHandler->push(SquirrelFileException("Failed to save file."));

                return;
            }

            file << data;

            file.close();
        });
    });
}

void GUI::setupSend(const std::string path)
{
    this->path = path;

    networkManager->beginBroadcast(std::bind(&GUI::handleResponse, this, std::placeholders::_1));
}

void GUI::run()
{
    SDL_Event event;

    bool running = true;

    render();

    while (running)
    {
        while (std::optional<SquirrelException> error = errorHandler->pop())
        {
            std::cout << error.value().what() << "\n";
        }

        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_QUIT:
                    running = false;

                    break;

                case SDL_MOUSEBUTTONDOWN:
                    // check actual positions later, this is simplified for testing
                    renderLock.lock();

                    if (!availableTargets.empty())
                    {
                        networkManager->beginTransfer(path, availableTargets[0]);
                    }

                    renderLock.unlock();

                    break;
            }
        }

        while (std::optional<std::function<void()>> operation = mainThreadQueue->pop())
        {
            operation.value()();
        }

        if ((clock.now() - lastFrame).count() >= 1e9 / 60)
        {
            render();
        }
    }
}

void GUI::render()
{
    renderLock.lock();

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 128, 128, 255);

    const int size = 50;
    const int gap = 25;

    const int startX = width / 2 - (availableTargets.size() * size + (availableTargets.size() - 1) * gap) / 2;

    for (int i = 0; i < availableTargets.size(); i++)
    {
        SDL_Rect rect { startX + i * (size + gap), height / 2 - size / 2, size, size };

        SDL_RenderFillRect(renderer, &rect);
    }

    SDL_RenderPresent(renderer);

    lastFrame = clock.now();

    renderLock.unlock();
}

void GUI::handleResponse(const std::string ip)
{
    renderLock.lock();

    if (std::find(availableTargets.begin(), availableTargets.end(), ip) == availableTargets.end())
    {
        availableTargets.push_back(ip);
    }

    renderLock.unlock();
}

int eventWatch(void* userdata, SDL_Event* event)
{
    switch (event->type)
    {
        case SDL_WINDOWEVENT:
            switch (event->window.event)
            {
                case SDL_WINDOWEVENT_RESIZED:
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                {
                    ((GUI*)userdata)->render();

                    break;
                }
            }

            break;
    }

    return 1;
}

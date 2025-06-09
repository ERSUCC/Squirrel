#include "renderer.h"

TargetButton::TargetButton(SDL_Renderer* renderer, const std::string name, const std::string ip) :
    Button(renderer), name(name), ip(ip) {}

Renderer::Renderer(ErrorHandler* errorHandler, NetworkManager* networkManager, FileManager* fileManager) :
    errorHandler(errorHandler), networkManager(networkManager), fileManager(fileManager)
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI, &window, &renderer);
    SDL_SetWindowTitle(window, "Squirrel");
    SDL_AddEventWatch(eventWatch, this);

    const int unscaledWidth = width;
    const int unscaledHeight = height;

    SDL_GL_GetDrawableSize(window, &width, &height);

    scale = (float)width / unscaledWidth;

    TTF_Init();

    font = TTF_OpenFont("resources/fonts/OpenSans-Variable.ttf", 12 * scale);

    StackLayout* stack = new StackLayout(renderer);

    stack->setDirection(Direction::Vertical);
    stack->setHorizontalAnchor(Anchor::Center);
    stack->setVerticalAnchor(Anchor::Center);
    stack->setSpacing(10 * scale);

    root = stack;

    resized(unscaledWidth, unscaledHeight);
}

Renderer::~Renderer()
{
    TTF_CloseFont(font);
    TTF_Quit();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void Renderer::setupEmpty()
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

void Renderer::setupSend(const std::string path)
{
    this->path = path;

    networkManager->beginBroadcast(std::bind(&Renderer::handleResponse, this, std::placeholders::_1, std::placeholders::_2));
}

void Renderer::run()
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

                case SDL_MOUSEMOTION:
                    renderLock.lock();

                    for (TargetButton* target : targets)
                    {
                        target->hover(event.button.x * scale, event.button.y * scale);
                    }

                    renderLock.unlock();

                    break;

                case SDL_MOUSEBUTTONDOWN:
                    renderLock.lock();

                    for (TargetButton* target : targets)
                    {
                        target->click(event.button.x * scale, event.button.y * scale);
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

void Renderer::render()
{
    renderLock.lock();

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);

    root->render();

    SDL_RenderPresent(renderer);

    lastFrame = clock.now();

    renderLock.unlock();
}

void Renderer::resized(const unsigned int width, const unsigned int height)
{
    this->width = width * scale;
    this->height = height * scale;

    renderLock.lock();

    root->setSize(this->width, this->height);
    root->layout();

    renderLock.unlock();
}

void Renderer::handleResponse(const std::string name, const std::string ip)
{
    renderLock.lock();

    if (std::find_if(targets.begin(), targets.end(), [=](const TargetButton* target)
    {
        return target->name == name && target->ip == ip;
    }) == targets.end())
    {
        TargetButton* target = new TargetButton(renderer, name, ip);

        target->setSize(100 * scale, 30 * scale);
        target->setFont(font);
        target->setText(name);
        target->setBackgroundColor({ 200, 200, 200, 255 });
        target->setTextColor({ 0, 0, 0, 255 });
        target->setAction([=]()
        {
            networkManager->beginTransfer(path, ip);
        });

        targets.push_back(target);

        root->addObject(target);
        root->layout();
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
                    Renderer* renderer = (Renderer*)userdata;

                    renderer->resized(event->window.data1, event->window.data2);
                    renderer->render();

                    break;
                }
            }

            break;
    }

    return 1;
}

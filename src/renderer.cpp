#include "renderer.h"

Target::Target(const GUIObject* object, const std::string name, const std::string ip) :
    object(object), name(name), ip(ip) {}

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
    stack->setVerticalAnchor(Anchor::Leading);
    stack->setBorder(8 * scale);
    stack->setSpacing(8 * scale);

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

void Renderer::setPath(const std::string path)
{
    this->path = path;
}

void Renderer::setupSend()
{
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

                    root->hover(event.button.x * scale, event.button.y * scale);

                    renderLock.unlock();

                    break;

                case SDL_MOUSEBUTTONDOWN:
                    renderLock.lock();

                    root->click(event.button.x * scale, event.button.y * scale);

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

    if (std::find_if(targets.begin(), targets.end(), [=](const Target* target)
    {
        return target->name == name && target->ip == ip;
    }) == targets.end())
    {
        StackLayout* layout = new StackLayout(renderer);

        layout->setDirection(Direction::Horizontal);
        layout->setVerticalAnchor(Anchor::Center);
        layout->setSize(0, 64 * scale);
        layout->setBorder(8 * scale);
        layout->setSpacing(8 * scale);
        layout->setBackgroundColor({ 220, 220, 220, 255 });

        Label* label = new Label(renderer);

        label->setFont(font);
        label->setText(name);
        label->setTextColor({ 50, 50, 50, 255 });

        layout->addObject(label, Sizing::Fixed, Sizing::Fixed);
        layout->addObject(new StackLayout(renderer), Sizing::Stretch, Sizing::Stretch);

        Button* sendButton = new Button(renderer);

        sendButton->setSize(48 * scale, 48 * scale);
        sendButton->setFont(font);
        sendButton->setText("Send");
        sendButton->setBackgroundColor({ 200, 200, 200, 255 });
        sendButton->setTextColor({ 50, 50, 50, 255 });
        sendButton->setAction([=]()
        {
            if (path.empty())
            {
                // choose file

                return;
            }

            networkManager->beginTransfer(path, ip);
        });

        layout->addObject(sendButton, Sizing::Fixed, Sizing::Fixed);

        root->addObject(layout, Sizing::Stretch, Sizing::Fixed);
        root->layout();

        targets.push_back(new Target(layout, name, ip));
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

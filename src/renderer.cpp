#include "renderer.h"

Target::Target(const GUIObject* object, const std::string name, const std::string ip) :
    object(object), name(name), ip(ip) {}

Renderer::Renderer(ThreadSafeQueue<std::function<void()>>* mainThreadQueue, ErrorHandler* errorHandler, NetworkManager* networkManager, FileManager* fileManager, ServiceManager* serviceManager) :
    mainThreadQueue(mainThreadQueue), errorHandler(errorHandler), networkManager(networkManager), fileManager(fileManager), serviceManager(serviceManager)
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer("Squirrel", width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY, &window, &renderer);
    SDL_AddEventWatch(eventWatch, this);
    SDL_RaiseWindow(window);

    const int unscaledWidth = width;

    SDL_GetWindowSizeInPixels(window, &width, &height);

    scale = (float)width / unscaledWidth;

    TTF_Init();

    font = TTF_OpenFont(fileManager->getResourcePath("fonts/OpenSans-Variable.ttf").c_str(), 12 * scale);

    StackLayout* stack = new StackLayout(renderer);

    stack->setDirection(Direction::Vertical);
    stack->setVerticalAnchor(Anchor::Leading);
    stack->setBorder(8 * scale);
    stack->setSpacing(8 * scale);

    root = stack;

    resized(width, height);
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

void Renderer::setupMain()
{
    serviceManager->connectService(std::bind(&Renderer::handleResponse, this, std::placeholders::_1, std::placeholders::_2));
}

void Renderer::setupReceive(const std::string name, const std::string& data)
{
    const std::filesystem::path path = fileManager->getSavePath(name);

    std::ofstream file(path, std::ios::binary);

    if (!file.is_open())
    {
        errorHandler->push(SquirrelFileException("Failed to save file."));

        return;
    }

    file << data;

    file.close();
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
                case SDL_EVENT_QUIT:
                    running = false;

                    break;

                case SDL_EVENT_MOUSE_MOTION:
                    renderLock.lock();

                    root->hover(event.button.x * scale, event.button.y * scale);

                    renderLock.unlock();

                    break;

                case SDL_EVENT_MOUSE_BUTTON_DOWN:
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
    this->width = width;
    this->height = height;

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

            const size_t length = 9;

            char* data = (char*)malloc(sizeof(char) * length);

            data[0] = MessageConstants::CONNECTION;

            *(uint16_t*)(data + 2) = length;
            *(uint32_t*)(data + 4) = networkManager->convertAddress(ip);

            data[length - 1] = '\0';

            serviceManager->writeMessage(MessageType::Service, new DataArray(data, length));
            networkManager->beginTransfer(path, ip);
        });

        layout->addObject(sendButton, Sizing::Fixed, Sizing::Fixed);

        root->addObject(layout, Sizing::Stretch, Sizing::Fixed);
        root->layout();

        targets.push_back(new Target(layout, name, ip));
    }

    renderLock.unlock();
}

bool eventWatch(void* userdata, SDL_Event* event)
{
    switch (event->type)
    {
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        {
            Renderer* renderer = (Renderer*)userdata;

            renderer->resized(event->window.data1, event->window.data2);
            renderer->render();

            return false;
        }
    }

    return true;
}

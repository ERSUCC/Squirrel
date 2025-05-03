#include "gui.h"

GUI::GUI()
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_CreateWindowAndRenderer(width, height, SDL_WINDOW_RESIZABLE, &window, &renderer);
    SDL_SetWindowTitle(window, "Squirrel");
    SDL_AddEventWatch(eventWatch, this);
}

GUI::~GUI()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void GUI::run()
{
    SDL_Event event;

    bool running = true;

    render();

    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_QUIT:
                    running = false;

                    break;
            }
        }

        if ((clock.now() - lastFrame).count() >= 1e9 / 60)
        {
            render();
        }
    }
}

void GUI::render()
{
    lock.lock();

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    lastFrame = clock.now();

    lock.unlock();
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

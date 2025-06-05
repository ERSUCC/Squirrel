#include "../include/gui.h"

GUIObject::GUIObject(SDL_Renderer* renderer) :
    renderer(renderer) {}

void GUIObject::click(const unsigned int x, const unsigned int y) const {}

Label::Label(SDL_Renderer* renderer) :
    GUIObject(renderer) {}

void Label::render() const
{
    SDL_SetRenderDrawColor(renderer, textColor.r, textColor.g, textColor.b, textColor.a);
    SDL_RenderCopy(renderer, textTexture, nullptr, &rect);
}

void Label::setLocation(const unsigned int x, const unsigned int y)
{
    rect.x = x;
    rect.y = y;
}

void Label::setSize(const unsigned int width, const unsigned int height)
{
    rect.w = width;
    rect.h = height;
}

void Label::setFont(TTF_Font* font)
{
    this->font = font;

    renderTexture();
}

void Label::setText(const std::string text)
{
    this->text = text;

    renderTexture();
}

void Label::setTextColor(const SDL_Color color)
{
    textColor = color;

    renderTexture();
}

void Label::renderTexture()
{
    if (textTexture)
    {
        SDL_DestroyTexture(textTexture);

        textTexture = nullptr;
    }

    if (text.empty())
    {
        return;
    }

    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), textColor);

    textTexture = SDL_CreateTextureFromSurface(renderer, surface);

    rect.w = surface->w;
    rect.h = surface->h;

    SDL_FreeSurface(surface);
}

Button::Button(SDL_Renderer* renderer) :
    GUIObject(renderer), label(new Label(renderer)) {}

void Button::render() const
{
    SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    SDL_RenderFillRect(renderer, &rect);

    label->render();
}

void Button::setLocation(const unsigned int x, const unsigned int y)
{
    rect.x = x;
    rect.y = y;

    label->setLocation(x + rect.w / 2 - label->rect.w / 2, y + rect.h / 2 - label->rect.h / 2);
}

void Button::setSize(const unsigned int width, const unsigned int height)
{
    rect.w = width;
    rect.h = height;

    label->setLocation(rect.x + width / 2 - label->rect.w / 2, rect.y + height / 2 - label->rect.h / 2);
}

void Button::click(const unsigned int x, const unsigned int y) const
{
    if (x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h)
    {
        action();
    }
}

void Button::setFont(TTF_Font* font)
{
    label->setFont(font);
    label->setLocation(rect.x + rect.w / 2 - label->rect.w / 2, rect.y + rect.h / 2 - label->rect.h / 2);
}

void Button::setText(const std::string text)
{
    label->setText(text);
    label->setLocation(rect.x + rect.w / 2 - label->rect.w / 2, rect.y + rect.h / 2 - label->rect.h / 2);
}

void Button::setBackgroundColor(const SDL_Color color)
{
    backgroundColor = color;
}

void Button::setTextColor(const SDL_Color color)
{
    label->setTextColor(color);
}

void Button::setAction(const std::function<void()> action)
{
    this->action = action;
}

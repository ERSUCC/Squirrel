#include "../include/gui.h"

GUIObject::GUIObject(SDL_Renderer* renderer) :
    renderer(renderer) {}

void GUIObject::setLocation(const int x, const int y)
{
    rect.x = x;
    rect.y = y;
}

void GUIObject::setSize(const int width, const int height)
{
    rect.w = width;
    rect.h = height;
}

void GUIObject::layout() {}

void GUIObject::hover(const int x, const int y) {}
void GUIObject::click(const int x, const int y) {}

Layout::Layout(SDL_Renderer* renderer) :
    GUIObject(renderer) {}

void Layout::render() const
{
    for (const GUIObject* object : objects)
    {
        object->render();
    }
}

void Layout::addObject(GUIObject* object)
{
    for (const GUIObject* existing : objects)
    {
        if (existing == object)
        {
            return;
        }
    }

    objects.push_back(object);
}

void Layout::removeObject(GUIObject* object)
{
    for (unsigned int i = 0; i < objects.size(); i++)
    {
        if (objects[i] == object)
        {
            objects.erase(objects.begin() + i);

            return;
        }
    }
}

StackLayout::StackLayout(SDL_Renderer* renderer) :
    Layout(renderer) {}

void StackLayout::layout()
{
    if (objects.empty())
    {
        return;
    }

    switch (direction)
    {
        case Direction::Horizontal:
        {
            int width = objects[0]->rect.w;

            for (unsigned int i = 1; i < objects.size(); i++)
            {
                width += spacing + objects[i]->rect.w;
            }

            int x;

            switch (horizontalAnchor)
            {
                case Anchor::Leading:
                    x = rect.x;

                    break;

                case Anchor::Center:
                    x = rect.x + rect.w / 2 - width / 2;

                    break;

                case Anchor::Trailing:
                    x = rect.x + rect.w - width;

                    break;
            }

            for (GUIObject* object : objects)
            {
                switch (verticalAnchor)
                {
                    case Anchor::Leading:
                        object->setLocation(x, rect.y);

                        break;

                    case Anchor::Center:
                        object->setLocation(x, rect.y + rect.h / 2 - object->rect.h / 2);

                        break;

                    case Anchor::Trailing:
                        object->setLocation(x, rect.y + rect.h - object->rect.h);

                        break;
                }

                object->layout();

                x += object->rect.w + spacing;
            }

            break;
        }

        case Direction::Vertical:
        {
            int height = objects[0]->rect.h;

            for (unsigned int i = 1; i < objects.size(); i++)
            {
                height += spacing + objects[i]->rect.h;
            }

            int y;

            switch (horizontalAnchor)
            {
                case Anchor::Leading:
                    y = rect.y;

                    break;

                case Anchor::Center:
                    y = rect.y + rect.h / 2 - height / 2;

                    break;

                case Anchor::Trailing:
                    y = rect.y + rect.h - height;

                    break;
            }

            for (GUIObject* object : objects)
            {
                switch (verticalAnchor)
                {
                    case Anchor::Leading:
                        object->setLocation(rect.x, y);

                        break;

                    case Anchor::Center:
                        object->setLocation(rect.x + rect.w / 2 - object->rect.w / 2, y);

                        break;

                    case Anchor::Trailing:
                        object->setLocation(rect.x + rect.w - object->rect.w, y);

                        break;
                }

                object->layout();

                y += object->rect.h + spacing;
            }

            break;
        }
    }
}

void StackLayout::setDirection(const Direction direction)
{
    this->direction = direction;
}

void StackLayout::setHorizontalAnchor(const Anchor anchor)
{
    horizontalAnchor = anchor;
}

void StackLayout::setVerticalAnchor(const Anchor anchor)
{
    verticalAnchor = anchor;
}

void StackLayout::setSpacing(const int spacing)
{
    this->spacing = spacing;
}

Label::Label(SDL_Renderer* renderer) :
    GUIObject(renderer) {}

void Label::render() const
{
    SDL_SetRenderDrawColor(renderer, textColor.r, textColor.g, textColor.b, textColor.a);
    SDL_RenderCopy(renderer, textTexture, nullptr, &rect);
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
    if (isHover)
    {
        SDL_SetRenderDrawColor(renderer, 255, 128, 128, 255);
    }

    else
    {
        SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
    }

    SDL_RenderFillRect(renderer, &rect);

    label->render();
}

void Button::layout()
{
    label->setLocation(rect.x + rect.w / 2 - label->rect.w / 2, rect.y + rect.h / 2 - label->rect.h / 2);
}

void Button::hover(const int x, const int y)
{
    isHover = x >= rect.x && y >= rect.y && x < rect.x + rect.w && y < rect.y + rect.h;
}

void Button::click(const int x, const int y)
{
    if (isHover)
    {
        action();
    }
}

void Button::setFont(TTF_Font* font)
{
    label->setFont(font);
}

void Button::setText(const std::string text)
{
    label->setText(text);
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

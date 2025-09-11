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

LayoutObject::LayoutObject(GUIObject* object, const Sizing horizontalSizing, const Sizing verticalSizing) :
    object(object), horizontalSizing(horizontalSizing), verticalSizing(verticalSizing) {}

Layout::Layout(SDL_Renderer* renderer) :
    GUIObject(renderer) {}

void Layout::render() const
{
    if (backgroundColor.a != 0)
    {
        SDL_SetRenderDrawColor(renderer, backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
        SDL_RenderFillRect(renderer, &rect);
    }

    for (const LayoutObject* object : objects)
    {
        object->object->render();
    }
}

void Layout::hover(const int x, const int y)
{
    for (LayoutObject* object : objects)
    {
        object->object->hover(x, y);
    }
}

void Layout::click(const int x, const int y)
{
    for (LayoutObject* object : objects)
    {
        object->object->click(x, y);
    }
}

void Layout::setBackgroundColor(const SDL_Color color)
{
    backgroundColor = color;
}

void Layout::addObject(GUIObject* object, const Sizing horizontalSizing, const Sizing verticalSizing)
{
    for (const LayoutObject* existing : objects)
    {
        if (existing->object == object)
        {
            return;
        }
    }

    objects.push_back(new LayoutObject(object, horizontalSizing, verticalSizing));
}

void Layout::removeObject(GUIObject* object)
{
    for (unsigned int i = 0; i < objects.size(); i++)
    {
        if (objects[i]->object == object)
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
            int fixedWidth = 0;
            int numFixed = 0;

            for (const LayoutObject* object : objects)
            {
                if (object->horizontalSizing == Sizing::Fixed)
                {
                    fixedWidth += object->object->rect.w;

                    numFixed++;
                }
            }

            int x;
            int stretchWidth;

            if (numFixed == objects.size())
            {
                switch (horizontalAnchor)
                {
                    case Anchor::Leading:
                        x = rect.x + border;

                        break;

                    case Anchor::Center:
                        x = rect.x + rect.w / 2 - (fixedWidth + spacing * (numFixed - 1)) / 2;

                        break;

                    case Anchor::Trailing:
                        x = rect.x + rect.w - (fixedWidth + spacing * (numFixed - 1)) - border;

                        break;
                }

                stretchWidth = 0;
            }

            else
            {
                x = rect.x + border;
                stretchWidth = (rect.w - fixedWidth - spacing * (objects.size() - 1) - border * 2) / (objects.size() - numFixed);
            }

            for (LayoutObject* object : objects)
            {
                int width;

                if (object->horizontalSizing == Sizing::Fixed)
                {
                    width = object->object->rect.w;
                }

                else
                {
                    width = stretchWidth;
                }

                if (object->verticalSizing == Sizing::Fixed)
                {
                    switch (verticalAnchor)
                    {
                        case Anchor::Leading:
                            object->object->setLocation(x, rect.y + border);

                            break;

                        case Anchor::Center:
                            object->object->setLocation(x, rect.y + rect.h / 2 - object->object->rect.h / 2);

                            break;

                        case Anchor::Trailing:
                            object->object->setLocation(x, rect.y + rect.h - object->object->rect.h - border);

                            break;
                    }

                    object->object->setSize(width, object->object->rect.h);
                }

                else
                {
                    object->object->setLocation(x, rect.y + border);
                    object->object->setSize(width, rect.h - border * 2);
                }

                x += width + spacing;

                object->object->layout();
            }

            break;
        }

        case Direction::Vertical:
        {
            int fixedHeight = 0;
            int numFixed = 0;

            for (const LayoutObject* object : objects)
            {
                if (object->verticalSizing == Sizing::Fixed)
                {
                    fixedHeight += object->object->rect.h;

                    numFixed++;
                }
            }

            int y;
            int stretchHeight;

            if (numFixed == objects.size())
            {
                switch (verticalAnchor)
                {
                    case Anchor::Leading:
                        y = rect.y + border;

                        break;

                    case Anchor::Center:
                        y = rect.y + rect.h / 2 - (fixedHeight + spacing * (numFixed - 1)) / 2;

                        break;

                    case Anchor::Trailing:
                        y = rect.y + rect.h - (fixedHeight + spacing * (numFixed - 1)) - border;

                        break;
                }

                stretchHeight = 0;
            }

            else
            {
                y = rect.y + border;
                stretchHeight = (rect.h - fixedHeight - spacing * (objects.size() - 1) - border * 2) / (objects.size() - numFixed);
            }

            for (LayoutObject* object : objects)
            {
                int height;

                if (object->verticalSizing == Sizing::Fixed)
                {
                    height = object->object->rect.h;
                }

                else
                {
                    height = stretchHeight;
                }

                if (object->horizontalSizing == Sizing::Fixed)
                {
                    switch (verticalAnchor)
                    {
                        case Anchor::Leading:
                            object->object->setLocation(rect.x + border, y);

                            break;

                        case Anchor::Center:
                            object->object->setLocation(rect.x + rect.w / 2 - object->object->rect.w / 2, y);

                            break;

                        case Anchor::Trailing:
                            object->object->setLocation(rect.x + rect.w - object->object->rect.w - border, y);

                            break;
                    }

                    object->object->setSize(object->object->rect.w, height);
                }

                else
                {
                    object->object->setLocation(rect.x + border, y);
                    object->object->setSize(rect.w - border * 2, height);
                }

                y += height + spacing;

                object->object->layout();
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

void StackLayout::setBorder(const int border)
{
    this->border = border;
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
    SDL_RenderTexture(renderer, textTexture, nullptr, &rect);
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

    SDL_Surface* surface = TTF_RenderText_Blended(font, text.c_str(), text.size(), textColor);

    textTexture = SDL_CreateTextureFromSurface(renderer, surface);

    rect.w = surface->w;
    rect.h = surface->h;

    SDL_DestroySurface(surface);
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

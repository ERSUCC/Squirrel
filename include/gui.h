#pragma once

#include <functional>
#include <string>
#include <vector>

#include <SDL.h>
#include <SDL_ttf.h>

struct GUIObject
{
    GUIObject(SDL_Renderer* renderer);

    virtual void render() const = 0;

    void setLocation(const int x, const int y);
    void setSize(const int width, const int height);

    virtual void layout();

    virtual void hover(const int x, const int y);
    virtual void click(const int x, const int y);

    SDL_Rect rect = { 0, 0, 0, 0 };

protected:
    SDL_Renderer* renderer;

};

enum Sizing
{
    Fixed,
    Stretch
};

struct LayoutObject
{
    LayoutObject(GUIObject* object, const Sizing horizontalSizing, const Sizing verticalSizing);

    GUIObject* object;

    const Sizing horizontalSizing;
    const Sizing verticalSizing;
};

struct Layout : public GUIObject
{
    Layout(SDL_Renderer* renderer);

    void render() const override;

    void addObject(GUIObject* object, const Sizing horizontalSizing, const Sizing verticalSizing);
    void removeObject(GUIObject* object);

protected:
    std::vector<LayoutObject*> objects;

};

enum Direction
{
    Horizontal,
    Vertical
};

enum Anchor
{
    Leading,
    Center,
    Trailing
};

struct StackLayout : public Layout
{
    StackLayout(SDL_Renderer* renderer);

    void layout() override;

    void setDirection(const Direction direction);

    void setHorizontalAnchor(const Anchor anchor);
    void setVerticalAnchor(const Anchor anchor);

    void setBorder(const int border);
    void setSpacing(const int spacing);

private:
    Direction direction;

    Anchor horizontalAnchor;
    Anchor verticalAnchor;

    int border;
    int spacing;

};

struct Label : public GUIObject
{
    Label(SDL_Renderer* renderer);

    void render() const override;

    void setFont(TTF_Font* font);

    void setText(const std::string text);

    void setTextColor(const SDL_Color color);

private:
    void renderTexture();

    std::string text;

    SDL_Color textColor = { 0, 0, 0, 0 };

    TTF_Font* font = nullptr;

    SDL_Texture* textTexture = nullptr;

};

struct Button : public GUIObject
{
    Button(SDL_Renderer* renderer);

    void render() const override;

    void layout() override;

    void hover(const int x, const int y) override;
    void click(const int x, const int y) override;

    void setFont(TTF_Font* font);

    void setText(const std::string text);

    void setBackgroundColor(const SDL_Color color);
    void setTextColor(const SDL_Color color);

    void setAction(const std::function<void()> action);

private:
    Label* label;

    SDL_Color backgroundColor = { 0, 0, 0, 0 };

    std::function<void()> action;

    bool isHover = false;

};

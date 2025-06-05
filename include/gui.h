#pragma once

#include <functional>
#include <string>

#include <SDL.h>
#include <SDL_ttf.h>

struct GUIObject
{
    GUIObject(SDL_Renderer* renderer);

    virtual void render() const = 0;

    virtual void setLocation(const unsigned int x, const unsigned int y) = 0;
    virtual void setSize(const unsigned int width, const unsigned int height) = 0;

    virtual void click(const unsigned int x, const unsigned int y) const;

    SDL_Rect rect = { 0, 0, 0, 0 };

protected:
    SDL_Renderer* renderer;

};

struct Label : public GUIObject
{
    Label(SDL_Renderer* renderer);

    void render() const override;

    void setLocation(const unsigned int x, const unsigned int y) override;
    void setSize(const unsigned int width, const unsigned int height) override;

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

    void setLocation(const unsigned int x, const unsigned int y) override;
    void setSize(const unsigned int width, const unsigned int height) override;

    void click(const unsigned int x, const unsigned int y) const override;

    void setFont(TTF_Font* font);

    void setText(const std::string text);

    void setBackgroundColor(const SDL_Color color);
    void setTextColor(const SDL_Color color);

    void setAction(const std::function<void()> action);

private:
    Label* label;

    SDL_Color backgroundColor = { 0, 0, 0, 0 };

    std::function<void()> action;

};

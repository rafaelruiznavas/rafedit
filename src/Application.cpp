#include "Application.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
    const int WindowWidth = 1280;
    const int WindowHeight = 720;

    constexpr float FontSize = 24.0f;
    constexpr float LineSpacing = 6.0f;

    constexpr float EditorLeft = 32.0f;
    constexpr float EditorTop = 32.0f;

    constexpr float CursorWidth = 2.0f;

    constexpr const char* FontPath = "assets/fonts/JetBrainsMono-Regular.ttf";

    constexpr SDL_Color TextColor { 220, 220, 225, 255 };
    constexpr SDL_Color BackgroundColor { 24, 24, 27, 255 };
    constexpr SDL_Color CursorColor { 235, 235, 240, 255 };
}

Application::Application()
{
    if(!SDL_Init(SDL_INIT_VIDEO))
    {
        throw std::runtime_error(std::string{"SDL_Init Error:"} + SDL_GetError());
    }

    if(!TTF_Init())
    {
        SDL_Quit();
        throw std::runtime_error(std::string{"TTF_Init Error:"} + SDL_GetError());
    }

    m_window = SDL_CreateWindow("Rafedit", WindowWidth, WindowHeight, SDL_WINDOW_RESIZABLE);

    if(m_window == nullptr)
    {
        TTF_Quit();
        SDL_Quit();
        throw std::runtime_error(std::string{"SDL_CreateWindow Error:"} + SDL_GetError());
    }

    m_renderer = SDL_CreateRenderer(m_window, nullptr);

    if(m_renderer == nullptr)
    {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;

        TTF_Quit();
        SDL_Quit();
        throw std::runtime_error(std::string{"SDL_CreateRenderer Error:"} + SDL_GetError());
    }

    m_font = TTF_OpenFont(FontPath, FontSize);

    if(m_font == nullptr)
    {
        throw std::runtime_error(std::string{"TTF_OpenFont Error:"} + SDL_GetError());
    }

    if(!SDL_StartTextInput(m_window))
    {
        throw std::runtime_error(std::string{"SDL_StartTextInput Error:"} + SDL_GetError());
    }
}

Application::~Application()
{
    if(m_window != nullptr)
    {
        SDL_StopTextInput(m_window);
    }
    
    destroyLineTextures();

    if(m_font != nullptr)
    {
        TTF_CloseFont(m_font);
        m_font = nullptr;
    }

    if(m_renderer)
    {
        SDL_DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
    
    if(m_window)
    {
        SDL_DestroyWindow(m_window);
        m_window = nullptr;
    }
    
    TTF_Quit();
    SDL_Quit();
}

int Application::run()
{
    while(m_running)
    {
        processEvents();
        update();
        render();

        SDL_Delay(1);
    }

    return 0;
}

void Application::processEvents()
{
    SDL_Event event{};
    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_EVENT_QUIT:
                m_running = false;
                break;
            case SDL_EVENT_TEXT_INPUT:
                handleTextInput(event.text.text);
                break;
            case SDL_EVENT_KEY_DOWN:
                handleKeyDown(event.key.key);
            default:
                break;
        }
    }
}

void Application::update()
{
    if(!m_editor.isDirty())
    {
        return;
    }
    rebuildLineTextures();
    m_editor.clearDirty();
}

void Application::render()
{
    SDL_SetRenderDrawColor(m_renderer, BackgroundColor.r, BackgroundColor.g, BackgroundColor.b, BackgroundColor.a);
    SDL_RenderClear(m_renderer);

    float currentY = EditorTop;

    for(const RenderedLine& line : m_renderedLines)
    {
        if(line.texture != nullptr)
        {
            const SDL_FRect destination { EditorLeft, currentY, line.width, line.height };
            SDL_RenderTexture(m_renderer, line.texture, nullptr, &destination);
        }
        currentY += FontSize + LineSpacing;
    }

    const SDL_FRect cursorRect { EditorLeft + calculateCursorX(), EditorTop + calculateCursorY(), CursorWidth, FontSize };
    SDL_SetRenderDrawColor(m_renderer, CursorColor.r, CursorColor.g, CursorColor.b, CursorColor.a);
    SDL_RenderFillRect(m_renderer, &cursorRect);

    SDL_RenderPresent(m_renderer);
}

void Application::handleTextInput(const char *l_input)
{
    if(l_input == nullptr)
    {
        return;
    }

    m_editor.insertText(l_input);
}

void Application::handleKeyDown(int key)
{
    switch(key)
    {
        case SDLK_ESCAPE:
            m_running = false;
            break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            m_editor.insertNewLine();
            break;
        case SDLK_BACKSPACE:
            m_editor.erasePreviousCharacter();
            break;
        case SDLK_DELETE:
            m_editor.eraseNextCharacter();
            break;
        case SDLK_LEFT:
            m_editor.moveCursorLeft();
            break;
        case SDLK_RIGHT:
            m_editor.moveCursorRight();
            break;
        case SDLK_UP:
            m_editor.moveCursorUp();
            break;
        case SDLK_DOWN:
            m_editor.moveCursorDown();
            break;
        case SDLK_HOME:
            m_editor.moveCursorToLineStart();
            break;
        case SDLK_END:
            m_editor.moveCursorToLineEnd();
            break;
        default:
            break;
    }
}

void Application::rebuildLineTextures()
{
    destroyLineTextures();
    const std::vector<std::string_view> lines = m_editor.lines();

    m_renderedLines.reserve(lines.size());

    for(const std::string_view line : lines)
    {
        RenderedLine renderedLine{};

        if(line.empty())
        {
            renderedLine.height = FontSize;
            m_renderedLines.push_back(renderedLine);
            continue;
        }
        SDL_Surface* textSurface = TTF_RenderText_Blended(m_font, line.data(), line.size(), TextColor);
        if(textSurface == nullptr)
        {
            throw std::runtime_error(std::string{"TTF_RenderText_Blended Error:"} + SDL_GetError());
        }

        renderedLine.width = static_cast<float>(textSurface->w);
        renderedLine.height = static_cast<float>(textSurface->h);
        renderedLine.texture = SDL_CreateTextureFromSurface(m_renderer, textSurface);

        SDL_DestroySurface(textSurface);

        if(renderedLine.texture == nullptr)
        {
            throw std::runtime_error(std::string{"SDL_CreateTextureFromSurface Error:"} + SDL_GetError());
        }
        m_renderedLines.push_back(renderedLine);
    }
}

void Application::destroyLineTextures()
{
    for(RenderedLine& line : m_renderedLines)
    {
        if(line.texture != nullptr)
        {
            SDL_DestroyTexture(line.texture);
            line.texture = nullptr;
        }
    }
    m_renderedLines.clear();
}

float Application::calculateCursorX() const
{
    const std::string textBeforeCursor = m_editor.textBeforeCursorOnCurrentLine();

    if(textBeforeCursor.empty())
    {
        return 0.0f;
    }

    int width = 0;
    int height = 0;
    
    if(!TTF_GetStringSize(m_font, textBeforeCursor.c_str(), textBeforeCursor.size(), &width, &height))
    {
        return 0.0f;
    }

    return static_cast<float>(width);
}

float Application::calculateCursorY() const
{
    const TextPosition cursorPosition = m_editor.cursorTextPosition();
    return static_cast<float>(cursorPosition.line) * (FontSize + LineSpacing);
}

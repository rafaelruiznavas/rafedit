#include "Application.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
    const int InitialWindowWidth = 1280;
    const int InitialWindowHeight = 720;

    constexpr float FontSize = 24.0f;
    constexpr float LineSpacing = 6.0f;

    constexpr float LineHeight = FontSize + LineSpacing;

    constexpr float EditorTop = 20.0f;
    constexpr float EditorBottom = 20.0f;

    constexpr float GutterLeft = 12.0f;
    constexpr float GutterWidth = 72.0f;

    constexpr float EditorLeft = GutterLeft + GutterWidth + 16.0f;

    constexpr float CursorWidth = 2.0f;

    constexpr float MouseScrollLines = 3.0f;
    constexpr const char* FontPath = "assets/fonts/JetBrainsMono-Regular.ttf";

    constexpr SDL_Color TextColor { 220, 220, 225, 255 };
    constexpr SDL_Color BackgroundColor { 24, 24, 27, 255 };
    constexpr SDL_Color CursorColor { 235, 235, 240, 255 };
    constexpr SDL_Color GutterColor { 31, 31, 35, 255 };
    constexpr SDL_Color CurrentLineColor { 31, 32, 38, 255 };
    constexpr SDL_Color GutterSeparatorColor { 55, 55, 62, 255};
}

Application::Application()
    : m_viewport{LineHeight}
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

    m_window = SDL_CreateWindow("Rafedit", InitialWindowWidth, InitialWindowHeight, SDL_WINDOW_RESIZABLE);

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

    SDL_GetWindowSize(m_window, &m_windowWidth, &m_windowHeight);
    m_viewport.setHeight(static_cast<float>(m_windowHeight) - EditorTop - EditorBottom);
    ensureCursorVisible();
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
            case SDL_EVENT_MOUSE_WHEEL:
                handleMouseWheel(event.wheel.y);
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                handleWindowResize(event.window.data1, event.window.data2);
                break;
            default:
                break;
        }
    }
}

void Application::update()
{
    if(!m_editor.isDirty() && !m_viewportDirty)
    {
        return;
    }

    rebuildVisibleLineTextures();
    m_editor.clearDirty();
    m_viewportDirty = false;
}

void Application::render()
{
    SDL_SetRenderDrawColor(m_renderer, BackgroundColor.r, BackgroundColor.g, BackgroundColor.b, BackgroundColor.a);
    SDL_RenderClear(m_renderer);

    const SDL_FRect gutterRectangle{0.0f, 0.0f, GutterLeft + GutterWidth, static_cast<float>(m_windowHeight)};
    SDL_SetRenderDrawColor(m_renderer, GutterColor.r, GutterColor.g, GutterColor.b, GutterColor.a);
    SDL_RenderFillRect(m_renderer, &gutterRectangle);

    const TextPosition cursorPosition = m_editor.cursorTextPosition();
    const float currentLineY = EditorTop + m_viewport.lineY(cursorPosition.line);
    const SDL_FRect currentLineRectangle{
        GutterLeft + GutterWidth,
        currentLineY,
        static_cast<float>(m_windowWidth) - GutterLeft - GutterWidth, LineHeight};
    SDL_SetRenderDrawColor(m_renderer, CurrentLineColor.r, CurrentLineColor.g, CurrentLineColor.b, CurrentLineColor.a);
    SDL_RenderFillRect(m_renderer, &currentLineRectangle);

    for(std::size_t index = 0; index < m_renderedLines.size(); ++index)
    {
        const std::size_t documentLine = m_renderedFirstLine + index;

        const float y = EditorTop + m_viewport.lineY(documentLine);

        const RenderedLine& line = m_renderedLines[index];

        if(line.number.texture != nullptr)
        {
            const float numberX = GutterLeft + GutterWidth - line.number.width - 10.0f;
            const SDL_FRect destination { numberX, y, line.number.width, line.number.height };
            SDL_RenderTexture(m_renderer, line.number.texture, nullptr, &destination);
        }

        if(line.content.texture != nullptr)
        {
            const SDL_FRect destination { EditorLeft, y, line.content.width, line.content.height };
            SDL_RenderTexture(m_renderer, line.content.texture, nullptr, &destination);
        }
    }

    const float separatorX = GutterLeft + GutterWidth;

    SDL_SetRenderDrawColor(m_renderer, GutterSeparatorColor.r, GutterSeparatorColor.g, GutterSeparatorColor.b, GutterSeparatorColor.a);

    SDL_RenderLine(m_renderer, separatorX, 0.0f, separatorX, static_cast<float>(m_windowHeight));

    const SDL_FRect cursorRectangle{EditorLeft + calculateCursorX(), EditorTop + calculateCursorY(), CursorWidth, FontSize};
    SDL_SetRenderDrawColor(m_renderer, CursorColor.r, CursorColor.g, CursorColor.b, CursorColor.a);
    SDL_RenderFillRect(m_renderer, &cursorRectangle);

    SDL_RenderPresent(m_renderer);
}

void Application::handleTextInput(const char *l_input)
{
    if(l_input == nullptr || l_input[0] == '\0')
    {
        return;
    }

    m_editor.insertText(l_input);
    ensureCursorVisible();
}

void Application::handleKeyDown(int l_key)
{
    bool cursorChanged = true;
    switch(l_key)
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
            cursorChanged = false;
            break;
    }

    if(cursorChanged)
    {
        ensureCursorVisible();
    }
}

void Application::handleMouseWheel(const float l_amount)
{
    m_viewport.scrollLines(-l_amount * MouseScrollLines, m_editor.lineCount());
    m_viewportDirty = true;
}

void Application::handleWindowResize(const int width, const int height)
{
    m_windowWidth = width;
    m_windowHeight = height;
    m_viewport.setHeight(std::max(LineHeight, static_cast<float>(height) - EditorTop - EditorBottom));

    ensureCursorVisible();
    m_viewportDirty = true;
}

void Application::rebuildVisibleLineTextures()
{
    destroyLineTextures();
    const std::vector<std::string_view> lines = m_editor.lines();

    m_renderedFirstLine = m_viewport.firstVisibleLine();
    const std::size_t lastLine = m_viewport.lastVisibleLine(lines.size());

    if(m_renderedFirstLine >= lastLine)
    {
        return;
    }

    m_renderedLines.reserve(lastLine - m_renderedFirstLine);

    for(std::size_t lineIndex = m_renderedFirstLine; lineIndex < lastLine; ++lineIndex)
    {
        RenderedLine renderedLine{};
        renderedLine.number = createRenderedText(std::to_string(lineIndex + 1), 115, 115, 125);
        const std::string_view line = lines[lineIndex];

        if(!line.empty())
        {
            renderedLine.content = createRenderedText(std::string{line}, 220, 220, 225);
        }
        m_renderedLines.push_back(renderedLine);
    }
}

void Application::destroyLineTextures()
{
    const auto destroyText = [](RenderedText& text)
    {
        if (text.texture != nullptr)
        {
            SDL_DestroyTexture(
                text.texture
            );

            text.texture = nullptr;
        }

        text.width = 0.0F;
        text.height = 0.0F;
    };

    for (RenderedLine& line : m_renderedLines)
    {
        destroyText(line.number);
        destroyText(line.content);
    }

    m_renderedLines.clear();
}

RenderedText Application::createRenderedText(const std::string &l_text, unsigned char l_red, unsigned char l_green, unsigned char l_blue) const
{
    if(l_text.empty())
    {
        return {};   
    }

    const SDL_Color color{l_red, l_green, l_blue, 255};
    SDL_Surface* surface = TTF_RenderText_Blended(m_font, l_text.c_str(), l_text.size(), color);

    if (surface == nullptr)
    {
        throw std::runtime_error(std::string{"No se pudo renderizar texto: "} + SDL_GetError());
    }

    RenderedText result{};

    result.width = static_cast<float>(surface->w);

    result.height = static_cast<float>(surface->h);

    result.texture = SDL_CreateTextureFromSurface(m_renderer, surface);

    SDL_DestroySurface(surface);

    if (result.texture == nullptr)
    {
        throw std::runtime_error(std::string{"No se pudo crear la textura: "} + SDL_GetError());
    }

    return result;
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

void Application::ensureCursorVisible()
{
    m_viewport.ensureLineVisible(
        m_editor.cursorTextPosition().line,
        m_editor.lineCount()
    );

    m_viewportDirty = true;
}

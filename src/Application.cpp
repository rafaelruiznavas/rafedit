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

std::size_t utf8BytePositionAtColumn(const std::string_view l_text, const std::size_t l_column)
{
    std::size_t bytePosition = 0;
    std::size_t currentColumn = 0;

    while (bytePosition < l_text.size() && currentColumn < l_column)
    {
        ++bytePosition;

        while (bytePosition < l_text.size() && (static_cast<unsigned char>(l_text[bytePosition]) & 0b1100'0000U) == 0b1000'0000U)
        {
            ++bytePosition;
        }
        ++currentColumn;
    }
    return bytePosition;
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
    m_viewport.setHeight(static_cast<float>(m_windowHeight) - EditorTop - EditorBottom, m_editor.lineCount());
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
                handleKeyDown(event.key.key, static_cast<unsigned int>(event.key.mod));
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

    renderSelection();

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

void Application::handleKeyDown(int l_key, unsigned int l_modifiers)
{
    const bool shiftPressed = (l_modifiers & static_cast<unsigned int>(SDL_KMOD_SHIFT)) != 0U;
    const bool controlPressed = (l_modifiers & static_cast<unsigned int>(SDL_KMOD_CTRL)) != 0U;

    if (controlPressed && l_key == SDLK_A)
    {
        m_editor.selectAll();
        ensureCursorVisible();
        m_viewportDirty = true;
        return;
    }
    bool cursorChanged = true;

    switch(l_key)
    {
        case SDLK_ESCAPE:
            if (m_editor.hasSelection())
            {
                m_editor.clearSelection();
                m_viewportDirty = true;
            }
            else
            {
                m_running = false;
            }
            return;
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
            m_editor.moveCursorLeft(shiftPressed);
            break;
        case SDLK_RIGHT:
            m_editor.moveCursorRight(shiftPressed);
            break;
        case SDLK_UP:
            m_editor.moveCursorUp(shiftPressed);
            break;
        case SDLK_DOWN:
            m_editor.moveCursorDown(shiftPressed);
            break;
        case SDLK_HOME:
            m_editor.moveCursorToLineStart(shiftPressed);
            break;
        case SDLK_END:
            m_editor.moveCursorToLineEnd(shiftPressed);
            break;
        default:
            cursorChanged = false;
            break;
    }

    if(cursorChanged)
    {
        ensureCursorVisible();
        m_viewportDirty = true;
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
    m_viewport.setHeight(std::max(LineHeight, static_cast<float>(height) - EditorTop - EditorBottom), m_editor.lineCount());

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

void Application::renderSelection()
{
     if (!m_editor.hasSelection())
    {
        return;
    }
    const std::vector<std::string_view> lines = m_editor.lines();
    const TextPosition selectionStart = m_editor.textPositionAt(m_editor.selectionStart());
    const TextPosition selectionEnd = m_editor.textPositionAt(m_editor.selectionEnd());
    const std::size_t firstVisibleLine = m_viewport.firstVisibleLine();
    const std::size_t lastVisibleLine = m_viewport.lastVisibleLine(lines.size());
    const std::size_t firstSelectionLine = std::max(selectionStart.line, firstVisibleLine);
    const std::size_t lastSelectionLine = std::min(selectionEnd.line, lastVisibleLine > 0 ? lastVisibleLine - 1 : 0);

    if (firstSelectionLine > lastSelectionLine)
    {
        return;
    }

    SDL_SetRenderDrawColor(m_renderer, 55, 78, 120, 180);

    for(std::size_t lineIndex = firstSelectionLine; lineIndex <= lastSelectionLine; ++lineIndex)
    {
        const std::string_view line = lineAt(lines, lineIndex);
        const std::size_t startColumn = lineIndex == selectionStart.line ? selectionStart.column : 0;
        const std::size_t endColumn = lineIndex == selectionEnd.line ? selectionEnd.column : static_cast<std::size_t>(-1);
        const std::size_t startByte = utf8BytePositionAtColumn(line, startColumn);
        const std::size_t endByte = endColumn == static_cast<std::size_t>(-1) ? line.size() : utf8BytePositionAtColumn(line, endColumn);
        const float startX = measureTextWidth(line.substr(0,startByte));
        float endX = measureTextWidth(line.substr(0, endByte));

        /*
         * Cuando la selección atraviesa un salto de línea,
         * mostramos una pequeña extensión después del último
         * carácter para representar visualmente el '\n'.
         */
        if (lineIndex < selectionEnd.line && endX <= startX)
        {
            endX = startX + 10.0F;
        }
        else if (lineIndex < selectionEnd.line)
        {
            endX += 10.0F;
        }

        const SDL_FRect rectangle{EditorLeft + startX, EditorTop + m_viewport.lineY(lineIndex), std::max(endX - startX,2.0F),LineHeight};
        SDL_RenderFillRect(m_renderer, &rectangle);
    }
}

float Application::measureTextWidth(const std::string_view l_text)
{
    if (l_text.empty())
    {
        return 0.0F;
    }

    int width = 0;
    int height = 0;

    if (!TTF_GetStringSize(m_font,l_text.data(),l_text.size(),&width,&height))
    {
        return 0.0F;
    }

    return static_cast<float>(width);
}

std::string_view Application::lineAt(const std::vector<std::string_view> &l_lines, std::size_t l_line) const
{
    if (l_line >= l_lines.size())
    {
        return {};
    }

    return l_lines[l_line];
}

#include "Application.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <memory>
#include <cmath>

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

    constexpr int MouseScrollLines = 3;
    constexpr std::size_t TabSize = 4;
    constexpr const char* FontPath = "assets/fonts/JetBrainsMono-Regular.ttf";

    constexpr SDL_Color TextColor { 220, 220, 225, 255 };
    constexpr SDL_Color BackgroundColor { 24, 24, 27, 255 };
    constexpr SDL_Color CursorColor { 235, 235, 240, 255 };
    constexpr SDL_Color GutterColor { 31, 31, 35, 255 };
    constexpr SDL_Color CurrentLineColor { 31, 32, 38, 255 };
    constexpr SDL_Color GutterSeparatorColor { 55, 55, 62, 255};

    constexpr std::uint64_t AutoScrollIntervalMs = 45;
    constexpr int AutoScrollMinimumLines = 1;
    constexpr int AutoScrollMaximumLines = 5;

    std::string expandTabs(const std::string_view text, const std::size_t tabSize)
    {
        if (tabSize == 0)
        {
            return std::string{text};
        }

        std::string result;
        result.reserve(text.size());
        std::size_t column = 0;

        for (const char character : text)
        {
            if (character == '\t')
            {
                const std::size_t spaces = tabSize - column % tabSize;
                result.append(spaces, ' ');
                column += spaces;
            }
            else
            {
                result.push_back(character);
                // Los bytes de continuacion UTF-8 no avanzan la columna.
                if ((static_cast<unsigned char>(character) & 0b1100'0000U) != 0b1000'0000U)
                {
                    ++column;
                }
            }
        }

        return result;
    }
}

struct SDLMemoryDeleter
{
    void operator()(void* pointer)const noexcept
    {
        SDL_free(pointer);
    }
};

using SDLStringPointer = std::unique_ptr<char, SDLMemoryDeleter>;

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

std::string normalizeLineEndings(std::string_view l_text)
{
    std::string result;
    result.reserve(l_text.size());

    for (std::size_t index = 0; index < l_text.size();++index)
    {
        if (l_text[index] == '\r')
        {
            if (index + 1 < l_text.size() && l_text[index + 1] == '\n')
            {
                ++index;
            }

            result.push_back('\n');
            continue;
        }

        result.push_back(l_text[index]);
    }
    return result;
}

std::size_t utf8CharacterCount(const std::string_view text) noexcept
{
    std::size_t count = 0;

    for (const unsigned char byte : text)
    {
        if ((byte & 0b1100'0000U) != 0b1000'0000U)
        {
            ++count;
        }
    }

    return count;
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
    int characterWidth = 0;
    int characterHeight = 0;

    if (!TTF_GetStringSize(m_font,"M",1, &characterWidth, &characterHeight))
    {
        throw std::runtime_error(std::string{"No se pudo medir la fuente: "} + SDL_GetError());
    }

    m_characterWidth = static_cast<float>(characterWidth);

    if(!SDL_StartTextInput(m_window))
    {
        throw std::runtime_error(std::string{"SDL_StartTextInput Error:"} + SDL_GetError());
    }

    updateViewportSize();
    syncViewportWithCursor();
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
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                handleMouseButtonDown(event.button.x, event.button.y, event.button.button, event.button.clicks, static_cast<unsigned int>(SDL_GetModState()));
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                handleMouseButtonUp(event.button.x,event.button.y,event.button.button);
                break;
            case SDL_EVENT_MOUSE_MOTION:
                handleMouseMotion(event.motion.x, event.motion.y);
                break;
            case SDL_EVENT_MOUSE_WHEEL:
                handleMouseWheel(event.wheel.x, event.wheel.y, event.wheel.integer_y, event.wheel.integer_x,
                    event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED, static_cast<unsigned int>(SDL_GetModState()));
                break;
            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                updateViewportSize();
                break;
            default:
                break;
        }
    }
}

void Application::update()
{
    updateMouseAutoScroll();

    if (!m_editor.isDirty() && !m_visibleLinesDirty)
    {
        return;
    }

    rebuildVisibleLineTextures();

    m_editor.clearDirty();
    m_visibleLinesDirty = false;
}

void Application::render()
{
    SDL_SetRenderDrawColor(m_renderer, BackgroundColor.r, BackgroundColor.g, BackgroundColor.b, BackgroundColor.a);
    SDL_RenderClear(m_renderer);
    const SDL_FRect gutterRectangle{0.0F,0.0F,GutterLeft + GutterWidth,static_cast<float>(m_windowHeight)};
    SDL_SetRenderDrawColor(m_renderer,GutterColor.r,GutterColor.g,GutterColor.b,GutterColor.a);
    SDL_RenderFillRect(m_renderer, &gutterRectangle);

    const SDL_Rect editorClip{
        static_cast<int>(EditorLeft),
        static_cast<int>(EditorTop),
        std::max(0,m_windowWidth - static_cast<int>(EditorLeft)),
        std::max(0,m_windowHeight - static_cast<int>(EditorTop + EditorBottom))
    };
    SDL_SetRenderClipRect(m_renderer,&editorClip);

    const TextPosition cursorPosition = m_editor.cursorTextPosition();
    const bool cursorVisible = m_viewport.isLineVisible(cursorPosition.line,m_editor.lineCount());

    if (cursorVisible)
    {
        const std::size_t visualLine = cursorPosition.line - m_viewport.firstVisibleLine();
        const float currentLineY = EditorTop + static_cast<float>(visualLine) * LineHeight;

        const SDL_FRect currentLineRectangle{EditorLeft, currentLineY, std::max(0.0F,
                static_cast<float>(
                    m_windowWidth
                ) -
                    EditorLeft
            ),
            LineHeight
        };

        SDL_SetRenderDrawColor(
            m_renderer,
            CurrentLineColor.r,
            CurrentLineColor.g,
            CurrentLineColor.b,
            CurrentLineColor.a
        );

        SDL_RenderFillRect(
            m_renderer,
            &currentLineRectangle
        );
    }

    renderSelection();

    for (
        std::size_t index = 0;
        index < m_renderedLines.size();
        ++index
    )
    {
        const float y =
            EditorTop +
            static_cast<float>(index) *
                LineHeight;

        const RenderedLine& line =
            m_renderedLines[index];

        if (line.content.texture != nullptr)
        {
            const SDL_FRect destination{
                textOriginX(),
                y,
                line.content.width,
                line.content.height
            };

            SDL_RenderTexture(
                m_renderer,
                line.content.texture,
                nullptr,
                &destination
            );
        }
    }

    if (cursorVisible)
    {
        const std::size_t visualLine =
            cursorPosition.line -
            m_viewport.firstVisibleLine();

        const SDL_FRect cursorRectangle{
            documentXToScreenX(
                calculateCursorX()
            ),
            EditorTop +
                static_cast<float>(
                    visualLine
                ) *
                    LineHeight,
            CursorWidth,
            FontSize
        };

        SDL_SetRenderDrawColor(
            m_renderer,
            CursorColor.r,
            CursorColor.g,
            CursorColor.b,
            CursorColor.a
        );

        SDL_RenderFillRect(
            m_renderer,
            &cursorRectangle
        );
    }

    SDL_SetRenderClipRect(
        m_renderer,
        nullptr
    );

    /*
     * Los números se dibujan fuera del clipping,
     * para permanecer fijos.
     */
    for (
        std::size_t index = 0;
        index < m_renderedLines.size();
        ++index
    )
    {
        const RenderedLine& line =
            m_renderedLines[index];

        if (line.number.texture == nullptr)
        {
            continue;
        }

        const float y =
            EditorTop +
            static_cast<float>(index) *
                LineHeight;

        const float numberX =
            GutterLeft +
            GutterWidth -
            line.number.width -
            10.0F;

        const SDL_FRect destination{
            numberX,
            y,
            line.number.width,
            line.number.height
        };

        SDL_RenderTexture(
            m_renderer,
            line.number.texture,
            nullptr,
            &destination
        );
    }

    const float separatorX =
        GutterLeft + GutterWidth;

    SDL_SetRenderDrawColor(
        m_renderer,
        GutterSeparatorColor.r,
        GutterSeparatorColor.g,
        GutterSeparatorColor.b,
        GutterSeparatorColor.a
    );

    SDL_RenderLine(
        m_renderer,
        separatorX,
        0.0F,
        separatorX,
        static_cast<float>(
            m_windowHeight
        )
    );

    SDL_RenderPresent(m_renderer);
}

void Application::updateViewportSize()
{
    int width = 0;
    int height = 0;

    if (!SDL_GetWindowSize(m_window,&width,&height))
    {
        SDL_Log("No se pudo obtener el tamaño de ventana: %s",SDL_GetError());
        return;
    }

    m_windowWidth = width;
    m_windowHeight = height;

    const float usableHeight = std::max(LineHeight,static_cast<float>(height) - EditorTop - EditorBottom);
    const float usableWidth = std::max(1.0f, static_cast<float>(width) - EditorLeft);

    m_viewport.setHeight(usableHeight, m_editor.lineCount());
    m_viewport.setWidth(usableWidth);

    m_visibleLinesDirty = true;
    syncViewportWithCursor();

    SDL_Log(
        "windowHeight=%d, usableHeight=%.2f, lineHeight=%.2f, visibleLines=%llu",
        height,
        usableHeight,
        LineHeight,
        m_viewport.visibleLineCount()
    );
}

void Application::handleTextInput(const char *l_input)
{
    if(l_input == nullptr || l_input[0] == '\0')
    {
        return;
    }

    m_editor.insertText(l_input);
    syncViewportWithCursor();
}

void Application::handleKeyDown(const int key,const unsigned int modifiers)
{
    const bool shiftPressed = (modifiers & static_cast<unsigned int>(SDL_KMOD_SHIFT)) != 0U;
    const bool controlPressed = (modifiers & static_cast<unsigned int>(SDL_KMOD_CTRL)) != 0U;

    if (controlPressed)
    {
        switch (key)
        {
            case SDLK_A:
                m_editor.selectAll();
                syncViewportWithCursor();
                return;

            case SDLK_C:
                copySelectionToClipboard();
                return;

            case SDLK_X:
                cutSelectionToClipboard();
                syncViewportWithCursor();
                return;

            case SDLK_V:
                pasteFromClipboard();
                syncViewportWithCursor();
                return;
            case SDLK_TAB:
                if(shiftPressed)
                {
                    m_editor.unindent();
                }
                else
                {
                    m_editor.indent();
                }
                syncViewportWithCursor();
                return;

            default:
                break;
        }
    }

    switch (key)
    {
        case SDLK_ESCAPE:
            if (m_editor.hasSelection())
            {
                m_editor.clearSelection();
            }
            else
            {
                m_running = false;
            }
            return;

        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            m_editor.insertNewLine();
            syncViewportWithCursor();
            return;

        case SDLK_BACKSPACE:
            m_editor.erasePreviousCharacter();
            syncViewportWithCursor();
            return;

        case SDLK_DELETE:
            m_editor.eraseNextCharacter();
            syncViewportWithCursor();
            return;

        case SDLK_LEFT:
            m_editor.moveCursorLeft(shiftPressed);
            syncViewportWithCursor();
            return;

        case SDLK_RIGHT:
            m_editor.moveCursorRight(shiftPressed);
            syncViewportWithCursor();
            return;

        case SDLK_UP:
            m_editor.moveCursorUp(shiftPressed);
            syncViewportWithCursor();
            return;

        case SDLK_DOWN:
            m_editor.moveCursorDown(shiftPressed);
            syncViewportWithCursor();
            return;

        case SDLK_HOME:
            m_editor.moveCursorToLineStart(shiftPressed);
            syncViewportWithCursor();
            return;

        case SDLK_END:
            m_editor.moveCursorToLineEnd(shiftPressed);
            syncViewportWithCursor();
            return;

        default:
            return;
    }
}

void Application::handleMouseWheel(float verticalAmount, float horizontalAmount, int verticalTicks, int horizontalTicks, const bool flipped, const unsigned int modifiers)
{
    if (flipped)
    {
        verticalAmount = -verticalAmount;
        horizontalAmount = -horizontalAmount;
        verticalTicks = -verticalTicks;
        horizontalTicks = -horizontalTicks;
    }

    const bool shiftPressed = ( modifiers & static_cast<unsigned int>(SDL_KMOD_SHIFT)) != 0U;

    /*
     * Shift + rueda:
     * scroll horizontal.
     */
    if (shiftPressed)
    {
        float amount = 0.0F;
        if (verticalTicks != 0)
        {
            amount = static_cast<float>(verticalTicks);
        }
        else
        {
            amount = verticalAmount;
        }

        if (amount != 0.0F)
        {
            m_viewport.scrollHorizontally(-amount * m_characterWidth * MouseScrollLines);
        }
        return;
    }

    /*
     * Rueda normal:
     * prioridad absoluta al scroll vertical.
     */
    if (verticalTicks != 0 || verticalAmount != 0.0F)
    {
        int lineDelta = 0;

        if (verticalTicks != 0)
        {
            /*
             * Una muesca de rueda ->
             * MouseScrollLines líneas.
             */
            lineDelta = -verticalTicks * static_cast<int>(MouseScrollLines);
        }
        else
        {
            /*
             * Trackpad / rueda de alta precisión.
             */
            lineDelta = verticalAmount > 0.0F ? -1 : 1;
        }

        if (lineDelta == 0)
        {
            return;
        }

        const std::size_t previousFirstLine = m_viewport.firstVisibleLine();

        m_viewport.scrollByLines(lineDelta,m_editor.lineCount());

        if (previousFirstLine != m_viewport.firstVisibleLine())
        {
            m_visibleLinesDirty = true;
        }

        if (m_selectingWithMouse)
        {
            updateMouseSelection();
        }

        return;
    }

    /*
     * Solo si NO existe desplazamiento vertical,
     * aceptamos un gesto horizontal real.
     */
    if (horizontalTicks != 0 || std::abs(horizontalAmount) > 0.05F)
    {
        const float amount = horizontalTicks != 0 ? static_cast<float>(horizontalTicks) : horizontalAmount;

        m_viewport.scrollHorizontally(-amount * m_characterWidth * MouseScrollLines);
    }
}

void Application::handleWindowResize(const int width, const int height)
{
    m_windowWidth = width;
    m_windowHeight = height;
    m_viewport.setHeight(std::max(LineHeight, static_cast<float>(height) - EditorTop - EditorBottom), m_editor.lineCount());

    syncViewportWithCursor();
}

void Application::handleMouseButtonDown(const float x, const float y, const unsigned char button, const unsigned char clicks, const unsigned int modifiers)
{
    if (button != SDL_BUTTON_LEFT)
    {
        return;
    }

    m_mouseX = x;
    m_mouseY = y;

    const bool shiftPressed = (modifiers & static_cast<unsigned int>(SDL_KMOD_SHIFT)) != 0U;
    const bool clickedGutter = x < EditorLeft;

    const std::size_t bytePosition = documentPositionFromMouse(x,y);

    if (clicks >= 3 || clickedGutter)
    {
        m_mouseSelectionMode = MouseSelectionMode::Line;
        m_mouseAnchorRange = lineRangeFromMouse(y);
        m_editor.selectRange(m_mouseAnchorRange.start, m_mouseAnchorRange.end);
    }
    /*
     * Doble clic: seleccionar palabra.
     */
    else if (clicks == 2)
    {
        m_mouseSelectionMode = MouseSelectionMode::Word;
        const std::size_t position = documentPositionFromMouse(x, y);
        m_mouseAnchorRange = m_editor.wordRangeAt(position);
        m_editor.selectRange(m_mouseAnchorRange.start,m_mouseAnchorRange.end);
    }
    /*
     * Shift + clic: extender desde el ancla actual.
     */
    else if (shiftPressed)
    {
        m_mouseSelectionMode = MouseSelectionMode::Character;
        const std::size_t position = documentPositionFromMouse(x, y);
        m_editor.moveCursorTo(position, true);
    }
    /*
     * Clic normal.
     */
    else
    {
        m_mouseSelectionMode = MouseSelectionMode::Character;
        const std::size_t position = documentPositionFromMouse(x, y);
        m_mouseAnchorRange = {position, position};
        m_editor.beginSelectionAt(position);
    }

    m_selectingWithMouse = true;
    m_lastAutoScrollTime = SDL_GetTicks();

    /*
     * SDL normalmente captura automáticamente durante un
     * arrastre, pero la captura explícita hace inequívoco
     * el comportamiento del editor.
     */
    if (!SDL_CaptureMouse(true))
    {
        SDL_Log("No se pudo capturar el ratón: %s", SDL_GetError());
    }
}

void Application::handleMouseButtonUp(const float x, const float y, const unsigned char button)
{
    if (button != SDL_BUTTON_LEFT)
    {
        return;
    }

    m_mouseX = x;
    m_mouseY = y;

    if (m_selectingWithMouse)
    {
        updateMouseSelection();
    }

    m_selectingWithMouse = false;

    if(!SDL_CaptureMouse(false))
    {
        SDL_Log("No se pudo liberar el ratón: %s", SDL_GetError());
    }
}

void Application::handleMouseMotion(const float x, const float y)
{
    const bool positionChanged = x != m_mouseX || y != m_mouseY;
    m_mouseX = x;
    m_mouseY = y;

    if (!positionChanged || !m_selectingWithMouse)
    {
        return;
    }



    updateMouseSelection();
}

std::size_t Application::documentPositionFromMouse(const float mouseX, const float mouseY) const
{
    if (m_editor.lineCount() == 0)
    {
        return 0;
    }
    const std::size_t documentLine = documentLineFromMouseY(mouseY);
    const std::size_t column = columnFromMouseX(documentLine, mouseX);
    return m_editor.bytePositionAt(documentLine, column);
}

std::size_t Application::documentLineFromMouseY(const float y) const noexcept
{
    if (m_editor.lineCount() == 0)
    {
        return 0;
    }

    std::size_t visualLine = 0;

    if (y > EditorTop)
    {
        visualLine = static_cast<std::size_t>(std::floor((y - EditorTop) / LineHeight));
    }

    const std::size_t documentLine = m_viewport.firstVisibleLine() + visualLine;

    return std::min(documentLine,m_editor.lineCount() - 1);
}

std::size_t Application::columnFromMouseX(const std::size_t documentLine, const float mouseX) const
{
    const LineLayout* layout = layoutForLine(documentLine);

    if (layout == nullptr)
    {
        return 0;
    }

    const float documentX = mouseX - EditorLeft + m_viewport.horizontalOffset();

    return layout->columnAtX(documentX);
}

float Application::textOriginX() const noexcept
{
    return EditorLeft - m_viewport.horizontalOffset();
}

float Application::documentXToScreenX(const float documentX) const noexcept
{
    return EditorLeft + documentX - m_viewport.horizontalOffset();
}

TextRange Application::lineRangeFromMouse(const float y) const
{
    const std::size_t documentLine = documentLineFromMouseY(y);
    const std::size_t position = m_editor.bytePositionAt(documentLine, 0);
    return m_editor.lineRangeAt(position,true);
}

const LineLayout* Application::layoutForLine(const std::size_t documentLine) const noexcept
{
    if (documentLine < m_renderedFirstLine)
    {
        return nullptr;
    }

    const std::size_t index = documentLine - m_renderedFirstLine;
    if (index >= m_renderedLines.size())
    {
        return nullptr;
    }

    return &m_renderedLines[index].layout;
}

void Application::updateMouseSelection()
{
    switch (m_mouseSelectionMode)
    {
        case MouseSelectionMode::Character:
        {
            const std::size_t position = documentPositionFromMouse(m_mouseX, m_mouseY);
            m_editor.updateSelectionTo(position);
            break;
        }

        case MouseSelectionMode::Word:
        {
            const std::size_t position = documentPositionFromMouse(m_mouseX,m_mouseY);
            const TextRange currentRange = m_editor.wordRangeAt(position);
            if (currentRange.start < m_mouseAnchorRange.start)
            {
                /*
                 * Selección hacia atrás:
                 * el extremo fijo queda al final de la
                 * palabra inicial.
                 */
                m_editor.selectRange(m_mouseAnchorRange.end, currentRange.start);
            }
            else
            {
                m_editor.selectRange(m_mouseAnchorRange.start, currentRange.end);
            }

            break;
        }

        case MouseSelectionMode::Line:
        {
            const TextRange currentRange = lineRangeFromMouse(m_mouseY);

            if (currentRange.start < m_mouseAnchorRange.start)
            {
                m_editor.selectRange(m_mouseAnchorRange.end, currentRange.start);
            }
            else
            {
                m_editor.selectRange(m_mouseAnchorRange.start, currentRange.end);
            }
            break;
        }
    }
}

void Application::updateMouseAutoScroll()
{
    if (!m_selectingWithMouse)
    {
        return;
    }

    const std::uint64_t now = SDL_GetTicks();

    if (now - m_lastAutoScrollTime < AutoScrollIntervalMs)
    {
        return;
    }

    bool viewportChanged = false;

    const float viewportBottom = static_cast<float>(m_windowHeight) - EditorBottom;

    int verticalDelta = 0;

    if (m_mouseY < EditorTop)
    {
        verticalDelta = -1;
    }
    else if (m_mouseY > viewportBottom)
    {
        verticalDelta = 1;
    }

    if (verticalDelta != 0)
    {
        const std::size_t previousFirstLine = m_viewport.firstVisibleLine();

        m_viewport.scrollByLines(verticalDelta,m_editor.lineCount());

        viewportChanged = previousFirstLine != m_viewport.firstVisibleLine();

        if (viewportChanged)
        {
            m_visibleLinesDirty = true;
        }
    }

    if (m_mouseX < EditorLeft)
    {
        m_viewport.scrollHorizontally(-m_characterWidth * 2.0F);
        viewportChanged = true;
    }
    else if (m_mouseX > static_cast<float>(m_windowWidth))
    {
        m_viewport.scrollHorizontally(m_characterWidth * 2.0F);
        viewportChanged = true;
    }

    if (viewportChanged)
    {
        updateMouseSelection();
    }

    m_lastAutoScrollTime = now;
}

void Application::copySelectionToClipboard()
{
    if (!m_editor.hasSelection())
    {
        return;
    }

    const std::string selectedText = m_editor.selectedText();

    if (selectedText.empty())
    {
        return;
    }

    if (!SDL_SetClipboardText(selectedText.c_str()))
    {
        SDL_Log("No se pudo copiar al portapapeles: %s", SDL_GetError());
    }
}

void Application::cutSelectionToClipboard()
{
    if (!m_editor.hasSelection())
    {
        return;
    }

    const std::string selectedText = m_editor.selectedText();

    if (selectedText.empty())
    {
        return;
    }

    if (!SDL_SetClipboardText(selectedText.c_str()))
    {
        SDL_Log("No se pudo cortar al portapapeles: %s",SDL_GetError());
        return;
    }

    m_editor.deleteSelection();

    syncViewportWithCursor();
    m_visibleLinesDirty = true;
}

void Application::pasteFromClipboard()
{
    if (!SDL_HasClipboardText())
    {
        return;
    }

    SDLStringPointer clipboardText{SDL_GetClipboardText()};

    if (clipboardText == nullptr)
    {
        SDL_Log("No se pudo leer el portapapeles: %s",SDL_GetError());
        return;
    }

    if(clipboardText.get()[0] == '\0')
    {
        return;
    }

    const std::string normalizedText = normalizeLineEndings(clipboardText.get());
    if(normalizedText.empty())
    {
        return;
    }
    m_editor.insertText(normalizedText);

    syncViewportWithCursor();
    m_visibleLinesDirty = true;
}

void Application::rebuildVisibleLineTextures()
{
    destroyLineTextures();
    const std::vector<std::string_view> lines = m_editor.lines();

    m_renderedFirstLine = m_viewport.firstVisibleLine();

    const std::size_t lastExclusive = m_viewport.lastVisibleLineExclusive(lines.size());
    if (m_renderedFirstLine >= lastExclusive)
    {
        return;
    }

    m_renderedLines.reserve(lastExclusive - m_renderedFirstLine);
    const float tabWidth = m_characterWidth * static_cast<float>(TabSize);
    for (std::size_t lineIndex = m_renderedFirstLine; lineIndex < lastExclusive; ++lineIndex)
    {
        RenderedLine renderedLine{};

        renderedLine.documentLine = lineIndex;
        renderedLine.number = createRenderedText(std::to_string(lineIndex + 1),115, 115, 125);
        const std::string_view line = lines[lineIndex];
        renderedLine.layout.rebuild(line, m_font, tabWidth);
        const std::string displayText = expandTabs(line, TabSize);
        if (!line.empty())
        {
            renderedLine.content = createRenderedText(displayText, 220, 220, 225);
        }

        m_renderedLines.push_back(std::move(renderedLine));
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

void Application::syncViewportWithCursor()
{
    const std::size_t previousFirstLine = m_viewport.firstVisibleLine();
    const TextPosition cursorPosition = m_editor.cursorTextPosition();
    m_viewport.ensureLineVisible(cursorPosition.line, m_editor.lineCount());
    m_viewport.ensureXVisible(calculateCursorX(), CursorWidth);

    if (previousFirstLine != m_viewport.firstVisibleLine())
    {
        m_visibleLinesDirty = true;
    }
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
    const TextPosition cursorPosition = m_editor.cursorTextPosition();

    const LineLayout* layout = layoutForLine(cursorPosition.line);

    if (layout == nullptr)
    {
        return 0.0F;
    }

    return layout->xAtColumn(cursorPosition.column);
}

void Application::renderSelection()
{
    if (!m_editor.hasSelection())
    {
        return;
    }
    const TextPosition selectionStart = m_editor.textPositionAt(m_editor.selectionStart());
    const TextPosition selectionEnd = m_editor.textPositionAt(m_editor.selectionEnd());
    const std::size_t firstVisibleLine = m_viewport.firstVisibleLine();
    const std::size_t lastVisibleLineExclusive = m_viewport.lastVisibleLineExclusive(m_editor.lineCount());

    if(firstVisibleLine >= lastVisibleLineExclusive)
    {
        return;
    }
    const std::size_t firstSelectionLine = std::max(selectionStart.line, firstVisibleLine);
    const std::size_t lastSelectionLine = std::min(selectionEnd.line, lastVisibleLineExclusive - 1);
    if(firstSelectionLine > lastSelectionLine)
    {
        return;
    }

    SDL_SetRenderDrawColor(m_renderer, 55, 78, 120, 180);

    for(std::size_t lineIndex = firstSelectionLine; lineIndex <= lastSelectionLine; ++lineIndex)
    {
        const LineLayout* layout = layoutForLine(lineIndex);
        if (layout == nullptr)
        {
            continue;
        }
        const std::size_t startColumn = lineIndex == selectionStart.line ? selectionStart.column : 0;
        const std::size_t endColumn = lineIndex == selectionEnd.line ? selectionEnd.column : layout->columnCount();
        const float startX = layout->xAtColumn(startColumn);
        float endX = layout->xAtColumn(endColumn);

        if (lineIndex < selectionEnd.line)
        {
            endX += m_characterWidth;
        }

        const std::size_t visualLine = lineIndex - firstVisibleLine;
        const SDL_FRect rectangle{documentXToScreenX(startX), EditorTop + static_cast<float>(visualLine) * LineHeight, std::max(endX - startX,2.0F),LineHeight};
        SDL_RenderFillRect(m_renderer, &rectangle);
    }
}

float Application::measureTextWidth(std::string_view l_text) const
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

std::size_t Application::bytePositionFromMouseX(const std::string_view line, const float mouseX) const
{
    if (line.empty() || m_characterWidth <= 0.0F)
    {
        return 0;
    }

    const float localX = mouseX - EditorLeft;

    if (localX <= 0.0F)
    {
        return 0;
    }

    const std::size_t requestedColumn = static_cast<std::size_t>(std::floor(localX / m_characterWidth +0.5F));

    std::size_t bytePosition = 0;
    std::size_t currentColumn = 0;

    while (bytePosition < line.size() && currentColumn < requestedColumn)
    {
        ++bytePosition;

        while (bytePosition < line.size() && (static_cast<unsigned char>(line[bytePosition]) & 0b1100'0000U) == 0b1000'0000U)
        {
            ++bytePosition;
        }

        ++currentColumn;
    }

    return bytePosition;
}

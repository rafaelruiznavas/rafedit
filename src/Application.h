#pragma once
#include "editor/Editor.h"
#include "editor/Viewport.h"
#include <string_view>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
struct TTF_Font;

struct RenderedText
{
    SDL_Texture* texture{nullptr};
    float width{0.0f};
    float height{0.0f};
};

struct RenderedLine
{
    RenderedText number;
    RenderedText content;
};

class Application
{
    void processEvents();
    void update();
    void render();

    void updateViewportSize();

    void handleTextInput(const char* l_input);
    void handleKeyDown(int l_key, unsigned int l_modifiers);
    void handleMouseWheel(float l_amount, const bool l_flipped);
    void handleWindowResize(int width, int height);
    void handleMouseButtonDown(float x, float y, unsigned char button, unsigned int modifiers);
    void handleMouseButtonUp(float x, float y, unsigned char button);
    void handleMouseMotion(float x, float y);

    [[nodiscard]]
    std::size_t documentPositionFromMouse(float x, float y) const;

    [[nodiscard]]
    std::size_t columnFromMouseX(std::string_view line, float x) const;

    void updateMouseSelection();

    void copySelectionToClipboard();
    void cutSelectionToClipboard();
    void pasteFromClipboard();

    void rebuildVisibleLineTextures();
    void destroyLineTextures();

    void syncViewportWithCursor();

    [[nodiscard]]
    RenderedText createRenderedText(const std::string& l_text, unsigned char l_red, unsigned char l_green, unsigned char l_blue) const;

    [[nodiscard]]
    float calculateCursorX() const;

    void ensureCursorVisible();

    void renderSelection();

    [[nodiscard]]
    float measureTextWidth(std::string_view l_text) const;

    [[nodiscard]]
    std::string_view lineAt(const std::vector<std::string_view>& l_lines, std::size_t l_line) const;

    bool m_running {true};
    bool m_visibleLinesDirty{true};

    int m_windowWidth{1280};
    int m_windowHeight{720};

    bool m_selectingWithMouse{false};
    float m_mouseX{0.0f};
    float m_mouseY{0.0f};

    std::uint64_t m_lastAutoScrollTime{0};

    SDL_Window* m_window {nullptr};
    SDL_Renderer* m_renderer {nullptr};

    TTF_Font* m_font {nullptr};
    std::vector<RenderedLine> m_renderedLines;
    std::size_t m_renderedFirstLine{0};

    Editor m_editor{
        "Rafedit\n"
        "\n"
        "Editor escrito de forma incremental\n"
        "Linea 4\n"
        "Linea 5\n"
        "Linea 6\n"
        "Linea 7\n"
        "Linea 8\n"
        "Linea 9\n"
        "Linea 10\n"
        "Linea 11\n"
        "Linea 12\n"
        "Linea 13\n"
        "Linea 14\n"
        "Linea 15\n"
        "Linea 16\n"
        "Linea 17\n"
        "Linea 18\n"
        "Linea 19\n"
        "Linea 20\n"
        "Linea 21\n"
        "Linea 22\n"
        "Linea 23\n"
        "Linea 24\n"
    };
    
    Viewport m_viewport;
    
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(const Application&) = delete;
    Application& operator=(Application&&) = delete;

    int run();
};
#pragma once
#include "editor/Editor.h"
#include "editor/Viewport.h"
#include <string>
#include <vector>
#include <cstddef>

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

    void handleTextInput(const char* l_input);
    void handleKeyDown(int l_key);
    void handleMouseWheel(float l_amount);
    void handleWindowResize(int width, int height);

    void rebuildVisibleLineTextures();
    void destroyLineTextures();

    [[nodiscard]]
    RenderedText createRenderedText(const std::string& l_text, unsigned char l_red, unsigned char l_green, unsigned char l_blue) const;

    [[nodiscard]]
    float calculateCursorX() const;

    [[nodiscard]]
    float calculateCursorY() const;

    void ensureCursorVisible();

    bool m_running {true};
    bool m_viewportDirty{true};

    int m_windowWidth{1280};
    int m_windowHeight{720};

    SDL_Window* m_window {nullptr};
    SDL_Renderer* m_renderer {nullptr};

    TTF_Font* m_font {nullptr};
    std::vector<RenderedLine> m_renderedLines;
    std::size_t m_renderedFirstLine{0};

    Editor m_editor{
        "Rafedit\n"
        "\n"
        "Editor escrito de forma incremental"
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
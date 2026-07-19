#pragma once
#include "editor/Editor.h"
#include <string>
#include <vector>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
struct TTF_Font;

struct RenderedLine
{
    SDL_Texture* texture {nullptr};
    float width {0.0f};
    float height {0.0f};
};

class Application
{
    void processEvents();
    void update();
    void render();

    void handleTextInput(const char* l_input);
    void handleKeyDown(int key);

    void rebuildLineTextures();
    void destroyLineTextures();

    [[nodiscard]]
    float calculateCursorX() const;

    [[nodiscard]]
    float calculateCursorY() const;

    bool m_running {true};

    SDL_Window* m_window {nullptr};
    SDL_Renderer* m_renderer {nullptr};

    TTF_Font* m_font {nullptr};
    std::vector<RenderedLine> m_renderedLines;

    Editor m_editor{
        "Rafedit\n"
        "\n"
        "Editor escrito de forma incremental"
    };
    
    float m_textWidth {0.0f};
    float m_textHeight {0.0f};
    
public:
    Application();
    ~Application();

    Application(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(const Application&) = delete;
    Application& operator=(Application&&) = delete;

    int run();
};
#pragma once
#include "editor/Editor.h"
#include <string>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;
struct TTF_Font;

class Application
{
    void processEvents();
    void update();
    void render();

    void handleTextInput(const char* l_input);
    void handleKeyDown(int key);

    void rebuildTextTexture();
    void destroyTextTexture();

    [[nodiscard]]
    float calculateCursorX() const;

    bool m_running {true};

    SDL_Window* m_window {nullptr};
    SDL_Renderer* m_renderer {nullptr};

    TTF_Font* m_font {nullptr};
    SDL_Texture* m_textTexture {nullptr};

    Editor m_editor{"Escribe aqui..."};
    
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
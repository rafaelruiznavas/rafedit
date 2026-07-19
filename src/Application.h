#pragma once
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

    void insertText(const std::string& text);
    void erasePreviousCharacter();
    void eraseNextCharacter();

    void moveCursorLeft();
    void moveCursorRight();
    void moveCursorToStart();
    void moveCursorToEnd();

    [[nodiscard]]
    std::size_t previousUtf8Position(std::size_t position) const;

    [[nodiscard]]
    std::size_t nextUtf8Position(std::size_t position) const;

    void rebuildTextTexture();
    void destroyTextTexture();

    [[nodiscard]]
    float calculateCursorX() const;

    bool m_running {true};
    bool m_textDirty{true};

    SDL_Window* m_window {nullptr};
    SDL_Renderer* m_renderer {nullptr};

    TTF_Font* m_font {nullptr};
    SDL_Texture* m_textTexture {nullptr};

    std::string m_text;
    std::size_t m_cursorPosition {0};
    
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
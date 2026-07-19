#pragma once
#include "TextBuffer.h"
#include "Cursor.h"
#include <string>
#include <string_view>

class Editor
{
    [[nodiscard]]
    std::size_t previousUtf8Position(std::size_t l_position) const noexcept;

    [[nodiscard]]
    std::size_t nextUtf8Position(std::size_t l_position) const noexcept;

    [[nodiscard]]
    static bool isUtf8ContinuationByte(const unsigned char l_byte) noexcept;

    void markDirty() noexcept;

    TextBuffer m_textBuffer;
    Cursor m_cursor;

    bool m_dirty {true};
  
public:
    Editor();
    explicit Editor(std::string l_initialText);

    [[nodiscard]]
    const std::string& text() const noexcept;

    [[nodiscard]]
    const std::string textBeforeCursor() const;

    [[nodiscard]]
    std::size_t cursorPosition() const noexcept;

    [[nodiscard]]
    bool empty() const noexcept;

    [[nodiscard]]
    bool isDirty() const noexcept;

    void clearDirty() noexcept;

    void insertText(std::string_view l_text);

    void erasePreviousCharacter();
    void eraseNextCharacter();

    void moveCursorLeft();
    void moveCursorRight();
    void moveCursorToStart();
    void moveCursorToEnd();

};
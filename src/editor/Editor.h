#pragma once
#include "TextBuffer.h"
#include "TextPosition.h"
#include "Cursor.h"
#include "TextSelection.h"
#include <string>
#include <string_view>
#include <vector>

class Editor
{
    void prepareSelection(bool l_selecting) noexcept;
    void finishSelectionMovement(bool l_selecting) noexcept;

    void collapseSelectionLeft() noexcept;
    void collapseSelectionRight() noexcept;

    [[nodiscard]]
    std::size_t previousUtf8Position(std::size_t l_position) const noexcept;

    [[nodiscard]]
    std::size_t nextUtf8Position(std::size_t l_position) const noexcept;

    [[nodiscard]]
    std::size_t utf8CharacterCount(std::size_t l_start, std::size_t l_end) const noexcept;

    [[nodiscard]]
    std::size_t positionAtColumn(std::size_t l_lineStart, std::size_t l_lineEnd, std::size_t l_column) const noexcept;

    [[nodiscard]]
    static bool isUtf8ContinuationByte(const unsigned char l_byte) noexcept;

    void markDirty() noexcept;

    TextBuffer m_textBuffer;
    Cursor m_cursor;
    TextSelection m_selection;

    std::size_t m_preferredColumn{0};

    bool m_dirty {true};
  
public:
    Editor();
    explicit Editor(std::string l_initialText);

    [[nodiscard]]
    const std::string& text() const noexcept;

    [[nodiscard]]
    const std::vector<std::string_view> lines() const;
    
    [[nodiscard]]
    std::size_t lineCount() const noexcept;

    [[nodiscard]]
    const std::string textBeforeCursorOnCurrentLine() const;

    [[nodiscard]]
    std::size_t cursorPosition() const noexcept;

    [[nodiscard]]
    TextPosition cursorTextPosition() const noexcept;

    [[nodiscard]]
    TextPosition textPositionAt(std::size_t l_bytePosition) const noexcept;

    [[nodiscard]]
    bool empty() const noexcept;

    [[nodiscard]]
    bool isDirty() const noexcept;

    void clearDirty() noexcept;

    [[nodiscard]]
    bool hasSelection() const noexcept;

    [[nodiscard]]
    std::size_t selectionStart() const noexcept;

    [[nodiscard]]
    std::size_t selectionEnd() const noexcept;

    [[nodiscard]]
    std::string selectedText() const;

    void clearSelection() noexcept;
    void selectAll() noexcept;
    void deleteSelection();

    void insertText(std::string_view l_text);

    void insertNewLine();
    void erasePreviousCharacter();
    void eraseNextCharacter();

    void moveCursorLeft(bool selection = false);
    void moveCursorRight(bool selection = false);
    void moveCursorUp(bool selection = false);
    void moveCursorDown(bool selection = false);

    void moveCursorToLineStart(bool selection = false);
    void moveCursorToLineEnd(bool selection = false);

};
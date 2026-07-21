#include "Editor.h"

std::size_t Editor::previousUtf8Position(std::size_t l_position) const noexcept
{
    if(l_position == 0 || m_textBuffer.empty())
    {
        return 0;
    }

    l_position = std::min(l_position, m_textBuffer.size());
    --l_position;
    const std::string& text = m_textBuffer.text();
    while(l_position > 0 && isUtf8ContinuationByte(static_cast<unsigned char>(text[l_position])))
    {
        --l_position;
    }
    return l_position;
}

std::size_t Editor::nextUtf8Position(std::size_t l_position) const noexcept
{
    if(l_position >= m_textBuffer.size())
    {
        return m_textBuffer.size();
    }

    const std::string& text = m_textBuffer.text();
    ++l_position;
    while(l_position < m_textBuffer.size() && isUtf8ContinuationByte(static_cast<unsigned char>(text[l_position])))
    {
        ++l_position;
    }
    return l_position;
}

std::size_t Editor::utf8CharacterCount(std::size_t l_start, std::size_t l_end) const noexcept
{
    //const std::string& text = m_textBuffer.text();
    std::size_t count = 0;
    std::size_t position = l_start;

    while(position < l_end)
    {
        position = nextUtf8Position(position);
        ++count;
    }

    return count;
}

std::size_t Editor::positionAtColumn(std::size_t l_lineStart, std::size_t l_lineEnd, std::size_t l_column) const noexcept
{
    std::size_t position = l_lineStart;
    std::size_t currentColumn = 0;

    while(position < l_lineEnd && currentColumn < l_column)
    {
        position = nextUtf8Position(position);
        ++currentColumn;
    }

    return std::min(position, l_lineEnd);
}

bool Editor::isUtf8ContinuationByte(const unsigned char l_byte) noexcept
{
    return (l_byte & 0b1100'0000U) == 0b1000'0000U;
}

void Editor::markDirty() noexcept
{
    m_dirty = true;
}

Editor::Editor() = default;

Editor::Editor(std::string l_initialText)
    : m_textBuffer(std::move(l_initialText)),
    m_cursor(m_textBuffer.size()) 
{
    m_preferredColumn = cursorTextPosition().column;
}

const std::string &Editor::text() const noexcept
{
    return m_textBuffer.text();
}

const std::vector<std::string_view> Editor::lines() const
{
    return m_textBuffer.lines();
}

const std::string Editor::textBeforeCursorOnCurrentLine() const
{
    const std::size_t lineStart = m_textBuffer.lineStart(m_cursor.position());

    return m_textBuffer.substr(lineStart, m_cursor.position() - lineStart);
}

std::size_t Editor::cursorPosition() const noexcept
{
    return m_cursor.position();
}

TextPosition Editor::cursorTextPosition() const noexcept
{
    const std::string& text = m_textBuffer.text();
    const std::size_t cursorPosition = m_cursor.position();

    std::size_t line = 0;

    for(std::size_t index=0; index < cursorPosition; ++index)
    {
        if(text[index] == '\n')
        {
            ++line;
        }
    }

    const std::size_t currentLineStart = m_textBuffer.lineStart(cursorPosition);

    const std::size_t column = utf8CharacterCount(currentLineStart, cursorPosition);

    return TextPosition{line, column};
}

bool Editor::empty() const noexcept
{
    return m_textBuffer.empty();
}

bool Editor::isDirty() const noexcept
{
    return m_dirty;
}

std::size_t Editor::lineCount() const noexcept
{
    return m_textBuffer.lineCount();
}

void Editor::clearDirty() noexcept
{
    m_dirty = false;
}

void Editor::insertText(std::string_view l_text)
{
    if(l_text.empty())
    {
        return;
    }

    const std::size_t insertionPosition = m_cursor.position();
    m_textBuffer.insert(insertionPosition, l_text);
    m_cursor.setPosition(insertionPosition + l_text.size(), m_textBuffer.size());

    m_preferredColumn = cursorTextPosition().column;

    markDirty();
}

void Editor::insertNewLine()
{
    insertText("\n");
}

void Editor::erasePreviousCharacter()
{
    if(m_cursor.position() == 0)
    {
        return;
    }

    const std::size_t currentPosition = m_cursor.position();
    const std::size_t previousPosition = previousUtf8Position(currentPosition);
    m_textBuffer.erase(previousPosition, currentPosition - previousPosition);
    m_cursor.setPosition(previousPosition, m_textBuffer.size());
    m_preferredColumn = cursorTextPosition().column;

    markDirty();
}

void Editor::eraseNextCharacter()
{
    const std::size_t currentPosition = m_cursor.position();

    if(currentPosition >= m_textBuffer.size())
    {
        return;
    }

    const std::size_t nextPosition = nextUtf8Position(currentPosition);
    m_textBuffer.erase(currentPosition, nextPosition - currentPosition);

    m_preferredColumn = cursorTextPosition().column;
    markDirty();
}

void Editor::moveCursorLeft()
{
    m_cursor.setPosition(previousUtf8Position(m_cursor.position()), m_textBuffer.size());
    m_preferredColumn = cursorTextPosition().column;
}

void Editor::moveCursorRight()
{
    m_cursor.setPosition(nextUtf8Position(m_cursor.position()), m_textBuffer.size());
    m_preferredColumn = cursorTextPosition().column;
}

void Editor::moveCursorUp()
{
    const std::size_t currentLineStart = m_textBuffer.lineStart(m_cursor.position());
    if(currentLineStart == 0)
    {
        return;
    }

    const std::size_t previousLineEnd = currentLineStart - 1;
    const std::size_t previousLineStart = m_textBuffer.lineStart(previousLineEnd);
    m_cursor.setPosition(positionAtColumn(previousLineStart, previousLineEnd, m_preferredColumn), m_textBuffer.size());
}

void Editor::moveCursorDown()
{
    const std::size_t currentLineEnd = m_textBuffer.lineEnd(m_cursor.position());
    if(currentLineEnd == m_textBuffer.size())
    {
        return;
    }

    const std::size_t nextLineStart = currentLineEnd + 1;
    const std::size_t nextLineEnd = m_textBuffer.lineEnd(nextLineStart);
    m_cursor.setPosition(positionAtColumn(nextLineStart, nextLineEnd, m_preferredColumn), m_textBuffer.size());
}

void Editor::moveCursorToLineStart()
{
    m_cursor.setPosition(m_textBuffer.lineStart(m_cursor.position()), m_textBuffer.size());
    m_preferredColumn = 0;
}

void Editor::moveCursorToLineEnd()
{
    m_cursor.setPosition(m_textBuffer.lineEnd(m_cursor.position()), m_textBuffer.size());
    m_preferredColumn = cursorTextPosition().column;
}


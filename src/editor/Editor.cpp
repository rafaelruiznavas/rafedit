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
    m_cursor(m_textBuffer.size()) {}

const std::string &Editor::text() const noexcept
{
    return m_textBuffer.text();
}

const std::string Editor::textBeforeCursor() const
{
    return m_textBuffer.substr(0, m_cursor.position());
}

std::size_t Editor::cursorPosition() const noexcept
{
    return m_cursor.position();
}

bool Editor::empty() const noexcept
{
    return m_textBuffer.empty();
}

bool Editor::isDirty() const noexcept
{
    return m_dirty;
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
    markDirty();
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

    m_cursor.setPosition(currentPosition, m_textBuffer.size());
    markDirty();
}

void Editor::moveCursorLeft()
{
    m_cursor.setPosition(previousUtf8Position(m_cursor.position()), m_textBuffer.size());
}

void Editor::moveCursorRight()
{
    m_cursor.setPosition(nextUtf8Position(m_cursor.position()), m_textBuffer.size());
}

void Editor::moveCursorToStart()
{
    m_cursor.moveToStart();
}

void Editor::moveCursorToEnd()
{
    m_cursor.moveToEnd(m_textBuffer.size());
}


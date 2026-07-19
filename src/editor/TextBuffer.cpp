#include "TextBuffer.h"

TextBuffer::TextBuffer(std::string l_text) :
    m_text(std::move(l_text)) {}

const std::string &TextBuffer::text() const noexcept
{
    return m_text;
}

std::size_t TextBuffer::size() const noexcept
{
    return m_text.size();
}

bool TextBuffer::empty() const noexcept
{
    return m_text.empty();
}

void TextBuffer::insert(std::size_t l_position, std::string_view l_text)
{
    m_text.insert(clampPosition(l_position), l_text.data(), l_text.size());
}

void TextBuffer::erase(std::size_t l_position, std::size_t l_length)
{
    m_text.erase(clampPosition(l_position), l_length);
}

std::string TextBuffer::substr(std::size_t l_position, std::size_t l_length) const
{
    return m_text.substr(clampPosition(l_position), l_length);
}

std::vector<std::string_view> TextBuffer::lines() const
{
    std::vector<std::string_view> result;
    std::size_t lineStartPosition = 0;

    while(lineStartPosition < m_text.size())
    {
        const std::size_t lineEndPosition = m_text.find('\n', lineStartPosition);

        if(lineEndPosition == std::string::npos)
        {
            result.emplace_back(m_text.data() + lineStartPosition, m_text.size() - lineStartPosition);
            break;
        }

        result.emplace_back(m_text.data() + lineStartPosition, lineEndPosition - lineStartPosition);
        lineStartPosition = lineEndPosition + 1;
    }

    if(result.empty())
    {
        result.emplace_back();
    }

    return result;
}

std::size_t TextBuffer::lineCount() const noexcept
{
    return static_cast<std::size_t>(std::count(m_text.begin(), m_text.end(), '\n'))+1;
}

std::size_t TextBuffer::lineStart(std::size_t l_position) const noexcept
{
    const std::size_t safePosition = clampPosition(l_position);

    if(safePosition == 0)
    {
        return 0;
    }

    const std::size_t previousNewLine = m_text.rfind('\n', safePosition - 1);
    
    if(previousNewLine == std::string::npos)
    {
        return 0;
    }

    return previousNewLine + 1;
}

std::size_t TextBuffer::lineEnd(std::size_t l_position) const noexcept
{
    const std::size_t safePosition = clampPosition(l_position);

    const std::size_t nextNewLine = m_text.find('\n', safePosition);

    if(nextNewLine == std::string::npos)
    {
        return m_text.size();
    }
    
    return nextNewLine;
}

std::size_t TextBuffer::clampPosition(std::size_t l_position) const noexcept
{
    return std::min(l_position, m_text.size());
}

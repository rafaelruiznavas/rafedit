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

std::size_t TextBuffer::clampPosition(std::size_t l_position) const noexcept
{
    return std::min(l_position, m_text.size());
}

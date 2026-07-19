#include "Cursor.h"
#include <algorithm>

Cursor::Cursor(std::size_t l_position)
    : m_position(l_position) {}

std::size_t Cursor::position() const noexcept
{
    return m_position;
}

void Cursor::setPosition(std::size_t l_position, std::size_t l_maxPosition) noexcept
{
    m_position = std::min(l_position, l_maxPosition);
}

void Cursor::moveToStart() noexcept
{
    m_position = 0;
}

void Cursor::moveToEnd(std::size_t l_bufferSize) noexcept
{
    m_position = l_bufferSize;
}

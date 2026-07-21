#include "Viewport.h"

#include <algorithm>
#include <cmath>

Viewport::Viewport(const float l_lineHeight)
    : m_lineHeight(l_lineHeight)
{
    std::max(l_lineHeight, 1.0f);
}

void Viewport::setHeight(const float l_height) noexcept
{
    m_height = std::max(l_height, m_lineHeight);
}

void Viewport::scrollLines(const float l_amount, const std::size_t l_lineCount) noexcept
{
    m_scrollOffset += l_amount * m_lineHeight;
    clamp(l_lineCount);
}

void Viewport::ensureLineVisible(const std::size_t l_line, const std::size_t l_lineCount) noexcept
{
    if(l_lineCount == 0)
    {
        m_scrollOffset = 0.0f;
        return;
    }

    const float lineTop = static_cast<float>(l_line) * m_lineHeight;
    const float lineBottom = lineTop + m_lineHeight;

    if(lineTop < m_scrollOffset)
    {
        m_scrollOffset = lineTop;
    }
    else if(lineBottom > m_scrollOffset + m_height)
    {
        m_scrollOffset = lineBottom - m_height;
    }

    clamp(l_lineCount);
}

std::size_t Viewport::firstVisibleLine() const noexcept
{
    return static_cast<std::size_t>(std::floor(m_scrollOffset / m_lineHeight));
}

std::size_t Viewport::visibleLineCount() const noexcept
{
    return static_cast<std::size_t>(std::ceil(m_height / m_lineHeight));
}

std::size_t Viewport::lastVisibleLine(const std::size_t l_lineCount) const noexcept
{
    if(l_lineCount == 0)
    {
        return 0;
    }

    return std::min(firstVisibleLine() + visibleLineCount(), l_lineCount);
}

float Viewport::lineY(const std::size_t l_line) const noexcept
{
    return (static_cast<float>(l_line) * m_lineHeight) - m_scrollOffset;
}

float Viewport::scrollOffset() const noexcept
{
    return m_scrollOffset;
}

void Viewport::clamp(const std::size_t l_lineCount) noexcept
{
    const float documentHeight = static_cast<float>(l_lineCount) * m_lineHeight;
    const float maximunOffset = std::max(0.0f, documentHeight - m_height);
    m_scrollOffset = std::clamp(m_scrollOffset, 0.0f, maximunOffset);
}
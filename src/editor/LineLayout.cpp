#include "LineLayout.h"

#include <SDL3_ttf/SDL_ttf.h>

#include <algorithm>
#include <cmath>
#include <string>

namespace
{
    bool isUtf8ContinuationByte(const unsigned char byte) noexcept
    {
        return (byte & 0b1100'0000U) == 0b1000'0000U;
    }

    std::size_t nextUtf8Position(const std::string_view text,std::size_t position) noexcept
    {
        if (position >= text.size())
        {
            return text.size();
        }

        ++position;

        while (position < text.size() && isUtf8ContinuationByte(static_cast<unsigned char>(text[position])))
        {
            ++position;
        }

        return position;
    }

    float measureText(TTF_Font* font, const std::string_view text)
    {
        if (font == nullptr || text.empty())
        {
            return 0.0F;
        }

        int width = 0;
        int height = 0;

        if (!TTF_GetStringSize(font, text.data(), text.size(), &width, &height))
        {
            return 0.0F;
        }

        return static_cast<float>(width);
    }
}

void LineLayout::rebuild(const std::string_view text, TTF_Font* font, const float tabWidth)
{
    m_positions.clear();
    m_positions.push_back(0.0F);

    m_width = 0.0F;

    std::size_t bytePosition = 0;

    while (bytePosition < text.size())
    {
        const std::size_t nextPosition = nextUtf8Position(text, bytePosition);

        /*
         * Tab necesita tratamiento visual especial.
         */
        if (text[bytePosition] == '\t')
        {
            const float nextTabStop = (std::floor(m_width / tabWidth) + 1.0F) * tabWidth;
            m_width = nextTabStop;
        }
        else
        {
            const std::string_view character{text.data() + bytePosition, nextPosition - bytePosition};

            m_width += measureText(font, character);
        }

        m_positions.push_back(m_width);
        bytePosition = nextPosition;
    }
}

float LineLayout::width() const noexcept
{
    return m_width;
}

std::size_t LineLayout::columnCount() const noexcept
{
    if (m_positions.empty())
    {
        return 0;
    }

    return m_positions.size() - 1;
}

float LineLayout::xAtColumn(const std::size_t column) const noexcept
{
    if (m_positions.empty())
    {
        return 0.0F;
    }

    const std::size_t safeColumn = std::min(column, m_positions.size() - 1);
    return m_positions[safeColumn];
}

std::size_t LineLayout::columnAtX(const float x) const noexcept
{
    if (x <= 0.0F || m_positions.size() <= 1)
    {
        return 0;
    }

    if (x >= m_width)
    {
        return columnCount();
    }

    const auto upper = std::upper_bound(m_positions.begin(),m_positions.end(),x);

    if (upper == m_positions.begin())
    {
        return 0;
    }

    const std::size_t rightColumn = static_cast<std::size_t>(std::distance(m_positions.begin(), upper));

    const std::size_t leftColumn = rightColumn - 1;

    const float leftX = m_positions[leftColumn];

    const float rightX = m_positions[rightColumn];

    const float middle = leftX + (rightX - leftX) * 0.5F;

    return x < middle ? leftColumn : rightColumn;
}
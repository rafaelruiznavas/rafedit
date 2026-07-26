#pragma once

#include <algorithm>
#include <cstddef>

class TextSelection
{
    std::size_t m_anchor{0};
    std::size_t m_cursor{0};
    bool m_active{true};
public:
    [[nodiscard]]
    bool active() const noexcept
    {
        return m_active && m_anchor != m_cursor;
    }

    [[nodiscard]]
    std::size_t anchor() const noexcept
    {
        return m_anchor;
    }

    [[nodiscard]]
    std::size_t cursor() const noexcept
    {
        return m_cursor;
    }

    [[nodiscard]]
    std::size_t start() const noexcept
    {
        return std::min(m_anchor, m_cursor);
    }

    [[nodiscard]]
    std::size_t end() const noexcept
    {
        return std::max(m_anchor, m_cursor);
    }

    [[nodiscard]]
    std::size_t length() const noexcept
    {
        return end() - start();
    }

    void begin(const std::size_t l_anchor) noexcept
    {
        m_anchor = l_anchor;
        m_cursor = l_anchor;
        m_active = true;
    }

    void update(const std::size_t l_cursor) noexcept
    {
        m_cursor = l_cursor;
    }

    void select(const std::size_t l_start, const std::size_t l_end)
    {
        m_anchor = l_start;
        m_cursor = l_end;
        m_active = l_start != l_end;
    }

    void clear() noexcept
    {
        m_anchor = 0;
        m_cursor = 0;
        m_active = false;
    }
};
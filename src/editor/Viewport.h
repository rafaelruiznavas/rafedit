#pragma once

#include <cstddef>

class Viewport
{
    void clamp(std::size_t l_lineCount) noexcept;

    float m_lineHeight{1.0f};
    float m_height{0.0f};
    float m_scrollOffset{0.0f};

public:
    explicit Viewport(float l_lineHeight);

    void setHeight(float l_height) noexcept;

    void scrollLines(float amount, std::size_t lineCount) noexcept;

    void ensureLineVisible(std::size_t l_line, std::size_t l_lineCount) noexcept;

    [[nodiscard]]
    std::size_t firstVisibleLine() const noexcept;

    [[nodiscard]]
    std::size_t visibleLineCount() const noexcept;
    
    [[nodiscard]]
    std::size_t lastVisibleLine(std::size_t l_lineCount) const noexcept;

    [[nodiscard]]
    float lineY(std::size_t l_line) const noexcept;

    [[nodiscard]]
    float scrollOffset() const noexcept;
};
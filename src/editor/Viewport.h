#pragma once

#include <cstddef>

class Viewport
{
    void clamp(std::size_t l_lineCount) noexcept;

    [[nodiscard]]
    std::size_t maximumFirstLine(std::size_t l_lineCount) const noexcept;

    float m_lineHeight{1.0f};
    float m_height{1.0f};
    std::size_t m_firstVisibleLine{0};


public:
    explicit Viewport(float l_lineHeight);

    void setHeight(float l_height, std::size_t l_lineCount) noexcept;

    void scrollByLines(int l_lineDelta, std::size_t l_lineCount) noexcept;

    void ensureLineVisible(std::size_t l_line, std::size_t l_lineCount) noexcept;

    [[nodiscard]]
    std::size_t firstVisibleLine() const noexcept;

    [[nodiscard]]
    std::size_t visibleLineCount() const noexcept;
    
    [[nodiscard]]
    std::size_t lastVisibleLineExclusive(std::size_t l_lineCount) const noexcept;

    [[nodiscard]]
    bool isLineVisible(std::size_t l_line, std::size_t l_documentLineCount) const noexcept;

    [[nodiscard]]
    float lineY(std::size_t l_line) const noexcept;

    [[nodiscard]]
    float scrollOffset() const noexcept;
};
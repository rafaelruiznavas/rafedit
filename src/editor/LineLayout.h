#pragma once

#include <cstddef>
#include <string_view>
#include <vector>

struct TTF_Font;

class LineLayout
{
public:
    void rebuild(std::string_view text,TTF_Font* font, float tabWidth);

    [[nodiscard]]
    float width() const noexcept;

    [[nodiscard]]
    std::size_t columnCount() const noexcept;

    [[nodiscard]]
    float xAtColumn(std::size_t column) const noexcept;

    [[nodiscard]]
    std::size_t columnAtX(float x) const noexcept;

private:
    std::vector<float> m_positions{0.0F};
    float m_width{0.0F};
};
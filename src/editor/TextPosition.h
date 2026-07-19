#pragma once
#include <cstddef>

struct TextPosition
{
    std::size_t line{0};
    std::size_t column{0};

    auto operator<=>(const TextPosition&) const = default;
};
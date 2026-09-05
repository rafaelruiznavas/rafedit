#pragma once

#include <cstddef>

struct TextRange
{
    std::size_t start{0};
    std::size_t end{0};

    [[nodiscard]]
    bool empty() const noexcept
    {
        return start == end;
    }
};
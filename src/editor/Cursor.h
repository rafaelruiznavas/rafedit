#pragma once
#include <cstddef>

class Cursor
{
    std::size_t m_position {0};

public:
    Cursor() = default;
    explicit Cursor(std::size_t l_position);

    [[nodiscard]]
    std::size_t position() const noexcept;

    void setPosition(std::size_t l_position, std::size_t l_maxPosition) noexcept;

    void moveToStart() noexcept;
    void moveToEnd(std::size_t l_bufferSize) noexcept;
};
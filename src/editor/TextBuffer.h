#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
class TextBuffer
{
    [[nodiscard]]
    std::size_t clampPosition(std::size_t l_position) const noexcept;

    std::string m_text;

public:
    TextBuffer() = default;
    explicit TextBuffer(std::string l_text);

    [[nodiscard]]
    const std::string& text() const noexcept;

    [[nodiscard]]
    std::size_t size() const noexcept;

    [[nodiscard]]
    bool empty() const noexcept;

    void insert(std::size_t l_position, std::string_view l_text);
    void erase(std::size_t l_position, std::size_t l_length);

    [[nodiscard]]
    std::string substr(std::size_t l_position, std::size_t l_length) const;

    [[nodiscard]]
    std::vector<std::string_view> lines() const;

    [[nodiscard]]
    std::size_t lineCount() const noexcept;

    [[nodiscard]]
    std::size_t lineStart(std::size_t l_position) const noexcept;

    [[nodiscard]]
    std::size_t lineEnd(std::size_t l_position) const noexcept;    
};
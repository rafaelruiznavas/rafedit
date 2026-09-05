#include "Viewport.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

Viewport::Viewport(const float l_lineHeight)
    : m_lineHeight(l_lineHeight),
    m_height(l_lineHeight)
{
}

void Viewport::setHeight(const float l_height, const std::size_t l_lineCount) noexcept
{
    m_height = std::max(l_height, m_lineHeight);
    clampVertical(l_lineCount);
}

void Viewport::setWidth(const float width) noexcept
{
    m_width = std::max(width,1.0F);

    m_horizontalOffset = std::max(m_horizontalOffset, 0.0F);
}

void Viewport::scrollByLines(int l_lineDelta, std::size_t l_lineCount) noexcept
{
    /*
     * Convertimos temporalmente a entero con signo para
     * permitir desplazamiento negativo sin desbordar size_t.
     */
    const std::int64_t current = static_cast<std::int64_t>(m_firstVisibleLine);
    const std::int64_t requested = current + static_cast<std::int64_t>(l_lineDelta);
    m_firstVisibleLine = requested <= 0 ? 0 : static_cast<std::size_t>(requested);
    clampVertical(l_lineCount);    
}

void Viewport::scrollHorizontally(float pixelDelta) noexcept
{
    m_horizontalOffset = std::max(0.0f, m_horizontalOffset + pixelDelta);
}

void Viewport::ensureLineVisible(const std::size_t l_requestedLine,const std::size_t documentLineCount) noexcept
{
    if (documentLineCount == 0)
    {
        m_firstVisibleLine = 0;
        return;
    }

    const std::size_t line =
        std::min(l_requestedLine,documentLineCount - 1);

    const std::size_t visibleLines = visibleLineCount();

    const std::size_t lastExclusive = m_firstVisibleLine + visibleLines;

    if (line < m_firstVisibleLine)
    {
        /*
         * El cursor está por encima.
         * La línea del cursor pasa a ser la primera.
         */
        m_firstVisibleLine = line;
    }
    else if (line >= lastExclusive)
    {
        /*
         * El cursor está por debajo.
         * La línea del cursor pasa a ser la última visible.
         */
        m_firstVisibleLine = line - visibleLines + 1;
    }

    clampVertical(documentLineCount);
}

void Viewport::ensureXVisible(const float x, const float caretWidth) noexcept
{
    constexpr float Margin = 16.0F;

    /*
     * x está expresado en coordenadas del documento.
     *
     * El intervalo actualmente visible es:
     *
     * [horizontalOffset_,
     *  horizontalOffset_ + width_]
     */

    const float visibleLeft = m_horizontalOffset;
    const float visibleRight = m_horizontalOffset + m_width;

    /*
     * Cursor fuera por la izquierda.
     */
    if (x < visibleLeft + Margin)
    {
        m_horizontalOffset = std::max(0.0F,x - Margin);
        return;
    }

    /*
     * Cursor fuera por la derecha.
     */
    if (x + caretWidth > visibleRight - Margin)
    {
        m_horizontalOffset = x + caretWidth + Margin - m_width;
        m_horizontalOffset = std::max(0.0F,m_horizontalOffset);
    }
}

std::size_t Viewport::firstVisibleLine() const noexcept
{
    return m_firstVisibleLine;
}

std::size_t Viewport::visibleLineCount() const noexcept
{
    return std::max<std::size_t>(1,static_cast<std::size_t>(std::floor(m_height / m_lineHeight)));
}

std::size_t Viewport::lastVisibleLineExclusive(const std::size_t l_lineCount) const noexcept
{
    return std::min(m_firstVisibleLine + visibleLineCount(), l_lineCount);
}

bool Viewport::isLineVisible(std::size_t l_line, std::size_t l_documentLineCount) const noexcept
{
    return l_line >= m_firstVisibleLine && l_line < lastVisibleLineExclusive(l_documentLineCount);
}

float Viewport::lineY(const std::size_t l_line) const noexcept
{
    if (l_line < m_firstVisibleLine)
    {
        return -m_lineHeight;
    }

    return static_cast<float>(l_line - m_firstVisibleLine) * m_lineHeight;    
}

float Viewport::horizontalOffset() const noexcept
{
    return m_horizontalOffset;
}

float Viewport::width() const noexcept
{
    return m_width;
}

void Viewport::clampVertical(const std::size_t l_lineCount) noexcept
{
    m_firstVisibleLine = std::min(m_firstVisibleLine,maximumFirstLine(l_lineCount));
}

std::size_t Viewport::maximumFirstLine(std::size_t l_lineCount) const noexcept
{
    const std::size_t visibleCount = visibleLineCount();

    if (l_lineCount <= visibleCount)
    {
        return 0;
    }

    return l_lineCount - visibleCount;
}

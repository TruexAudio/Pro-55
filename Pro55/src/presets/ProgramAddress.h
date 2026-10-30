#pragma once

namespace pro55
{
constexpr int programCount = 512;

constexpr int programAddress(int index) noexcept
{
    return ((index / 64) % 8 + 1) * 100
         + ((index / 8) % 8 + 1) * 10
         + (index % 8 + 1);
}

// digit: 0 = FILE (hundreds), 1 = BANK (tens), 2 = program (units).
constexpr int withProgramDigit(int index, int digit, int value) noexcept
{
    const int stride = digit == 0 ? 64 : digit == 1 ? 8 : 1;
    return index + (value - 1 - (index / stride) % 8) * stride;
}
}

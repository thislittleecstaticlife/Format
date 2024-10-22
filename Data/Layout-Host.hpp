//
//  Layout-Host.hpp
//
//  Copyright © 2024 Robert Guequierre
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#pragma once

#include <type_traits>

//===------------------------------------------------------------------------===
// • namespace data
//===------------------------------------------------------------------------===

namespace data
{

//===------------------------------------------------------------------------===
//
// • Data layout (Host only)
//
//===------------------------------------------------------------------------===

//===------------------------------------------------------------------------===
// • TrivialLayout concept
//===------------------------------------------------------------------------===

template <class Type_>
concept TrivialLayout = std::is_trivial_v<Type_> && std::is_standard_layout_v<Type_>;

// • is_trivial_layout
//
template <class Type_>
consteval bool is_trivial_layout(void) noexcept
{
    return false;
}

template <TrivialLayout Type_>
consteval bool is_trivial_layout(void) noexcept
{
    return true;
}

//===------------------------------------------------------------------------===
// • Concepts only available on host
//===------------------------------------------------------------------------===

#define TRIVIAL_LAYOUT data::TrivialLayout

//===------------------------------------------------------------------------===
// • Alignment (always 16 bytes)
//===------------------------------------------------------------------------===

enum : uint32_t
{
    alignment = 16
};

//===------------------------------------------------------------------------===
// • Aligned concept
//===------------------------------------------------------------------------===

template <class Type_>
concept Aligned = ( 0 == (alignof(Type_) & 0x0f) );

//===------------------------------------------------------------------------===
// • is_aligned
//===------------------------------------------------------------------------===

template <typename Type_>
consteval bool is_aligned(void) noexcept
{
    return false;
}

template <Aligned Type_>
consteval bool is_aligned(void) noexcept
{
    return true;
}

constexpr bool is_aligned(uint32_t size_or_offset) noexcept
{
    return 0 == (size_or_offset & 0x0f);
}

template <typename Type_>
constexpr bool is_aligned(const Type_* memory) noexcept
{
    return 0 == (reinterpret_cast<uintptr_t>(memory) & 0x0f);
}

//===------------------------------------------------------------------------===
// • aligned_size
//===------------------------------------------------------------------------===

template <typename Type_>
consteval uint32_t aligned_size(void) noexcept
{
    return static_cast<uint32_t>( (sizeof(Type_) + 0x0f) & ~0x0f  );
}

template <Aligned Type_>
consteval uint32_t aligned_size(void) noexcept
{
    return sizeof(Type_);
}

constexpr uint32_t aligned_size(uint32_t actual_size) noexcept
{
    return (actual_size + 0x0f) & ~0x0f;
}

template <typename Type_>
consteval uint32_t aligned_size(uint32_t count) noexcept
{
    return aligned_size( sizeof(Type_) * count );
}

//===------------------------------------------------------------------------===
//
// • Memory Layout Utilities (Host)
//
//===------------------------------------------------------------------------===

namespace detail
{

template <TrivialLayout Root_, TrivialLayout Type_>
uint32_t distance(const Root_* root, const Type_* data)
{
    return static_cast<uint32_t> (
                                  reinterpret_cast<const uint8_t*>(data) - reinterpret_cast<const uint8_t*>(root) );
}

template <TrivialLayout Root_, TrivialLayout Type_>
uint32_t distance(const void* root, const Type_* data)
{
    return static_cast<uint32_t> (
                                  reinterpret_cast<const uint8_t*>(data) - static_cast<const uint8_t*>(root) );
}

template <TrivialLayout Type_, TrivialLayout Root_>
const Type_* offset_by(const Root_* root, uint32_t offset)
{
    return reinterpret_cast<const Type_*>(reinterpret_cast<const uint8_t*>(root) + offset);
}

template <TrivialLayout Type_>
const Type_* offset_by(const void* root, uint32_t offset)
{
    return reinterpret_cast<const Type_*>(static_cast<const uint8_t*>(root) + offset);
}

template <TrivialLayout Type_, TrivialLayout Root_>
Type_* offset_by(Root_* root, uint32_t offset)
{
    return reinterpret_cast<Type_*>(reinterpret_cast<uint8_t*>(root) + offset);
}

template <TrivialLayout Type_>
Type_* offset_by(void* root, uint32_t offset)
{
    return reinterpret_cast<Type_*>(static_cast<uint8_t*>(root) + offset);
}

} // namespace data::detail

} // namespace data

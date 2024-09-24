//
//  Layout.hpp
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

#if !defined ( __METAL_VERSION__ )
#include <type_traits>
#endif

//===------------------------------------------------------------------------===
// • namespace format
//===------------------------------------------------------------------------===

namespace format
{

#if !defined ( __METAL_VERSION__ )

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
// • Alignment (always 16 bytes)
//===------------------------------------------------------------------------===

constexpr bool is_aligned(uint32_t size_or_offset) noexcept
{
    return 0 == (size_or_offset & 0x0f);
}

template <typename Type_>
constexpr bool is_aligned(const Type_* memory) noexcept
{
    return 0 == (reinterpret_cast<uintptr_t>(memory) & 0x0f);
}

constexpr uint32_t aligned_size(uint32_t actual_size) noexcept
{
    return (actual_size + 0x0f) & ~0x0f;
}

//===------------------------------------------------------------------------===
// • Aligned concept
//===------------------------------------------------------------------------===

template <class Type_>
concept Aligned = ( 0 == (alignof(Type_) & 0x0f) );

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

template <typename Type_>
consteval uint32_t aligned_size(void) noexcept
{
    return static_cast<uint32_t>( (sizeof(Type_) + 0x0f) & ~0x0f  );
}

template <Aligned Type_>
consteval uint32_t aligned_size(void) noexcept
{
    return static_cast<uint32_t>( sizeof(Type_) );
}

//===------------------------------------------------------------------------===
// • StructuralLayout concept
//===------------------------------------------------------------------------===

template <typename Data_>
concept StructuralLayout = ( TrivialLayout<Data_> && Aligned<Data_> );

template <typename Type_>
consteval bool is_structural_layout(void) noexcept
{
    return false;
}

template <StructuralLayout Type_>
consteval bool is_structural_layout(void) noexcept
{
    return true;
}

//===------------------------------------------------------------------------===
// • Concepts only available on host
//===------------------------------------------------------------------------===

#define TRIVIAL_LAYOUT    format::TrivialLayout
#define STRUCTURAL_LAYOUT format::StructuralLayout

#else // if defined ( __METAL_VERSION__ )

#define TRIVIAL_LAYOUT    typename
#define STRUCTURAL_LAYOUT typename

#endif

} // namespace format

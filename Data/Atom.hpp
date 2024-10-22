//
//  Atom.hpp
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

#include <Data/Layout-Host.hpp>

#include <cassert>
#include <iterator>

//===------------------------------------------------------------------------===
// • namespace data
//===------------------------------------------------------------------------===

namespace data
{

//===------------------------------------------------------------------------===
// • AtomID
//===------------------------------------------------------------------------===

enum class AtomID : uint32_t
{
    // • Valid layout:
    //
    //  [length] 'data'
    //  [length] 'free'?
    // ([length] 'vctr'
    //  [length] 'free'?)*
    //  [    16] 'end '

    data    = 'data',
    vector  = 'vctr',
    free    = 'free',
    end     = 'end ',
};

//===------------------------------------------------------------------------===
//
// • Atom
//
//===------------------------------------------------------------------------===

struct alignas(16) Atom
{
    uint32_t    length;
    AtomID      identifier;
    uint32_t    previous;
    uint32_t    reserved;
};

enum : uint32_t
{
    atom_header_length  = sizeof(Atom),
    min_contents_length = 2 * sizeof(Atom)
};

static_assert( 16 ==  sizeof(Atom), "Unexpected size" );
static_assert( 16 == alignof(Atom), "Unexpected alignment" );

static_assert( data::is_trivial_layout<Atom>(), "Unexpected layout" );
static_assert( data::is_aligned<Atom>(), "Unexpected alignment" );

//===------------------------------------------------------------------------===
//
// • Validation
//
//===------------------------------------------------------------------------===

bool valid_data(const Atom* data) noexcept;
bool valid_end(const Atom* end) noexcept;

bool valid_alignment_and_length(const void* contents, uint32_t contents_length) noexcept;

bool validate_layout(const void* contents, uint32_t contents_length) noexcept;

//===------------------------------------------------------------------------===
//
// • Iteration
//
//===------------------------------------------------------------------------===

namespace detail
{

//===------------------------------------------------------------------------===
// • Unchecked Atom utilities
//===------------------------------------------------------------------------===

constexpr bool empty(const Atom* atom) noexcept
{
    return atom_header_length == atom->length;
}

constexpr bool is_end(const Atom* atom) noexcept
{
    return AtomID::end == atom->identifier;
}

constexpr uint32_t contents_size(const Atom* atom) noexcept
{
    return atom->length - atom_header_length;
}

template <TrivialLayout Type_>
constexpr uint32_t capacity(const Atom* atom) noexcept
{
    return contents_size(atom) / sizeof(Type_);
}

template <TrivialLayout Type_>
    requires ( alignof(Type_) <= alignof(Atom) )
const Type_* contents(const Atom* atom) noexcept
{
    return reinterpret_cast<const Type_*>(atom + 1);
}

template <TrivialLayout Type_>
    requires ( alignof(Type_) <= alignof(Atom) )
Type_* contents(Atom* atom) noexcept
{
    return reinterpret_cast<Type_*>(atom + 1);
}

inline const Atom* next(const Atom* atom) noexcept
{
    return reinterpret_cast<const Atom*>(reinterpret_cast<const uint8_t*>(atom) + atom->length);
}

inline Atom* next(Atom* atom) noexcept
{
    return reinterpret_cast<Atom*>(reinterpret_cast<uint8_t*>(atom) + atom->length);
}

inline const Atom* previous(const Atom* atom) noexcept
{
    return reinterpret_cast<const Atom*>(reinterpret_cast<const uint8_t*>(atom) - atom->previous);
}

inline Atom* previous(Atom* atom) noexcept
{
    return reinterpret_cast<Atom*>(reinterpret_cast<uint8_t*>(atom) - atom->previous);
}

inline const Atom* offset_by(const Atom* base, uint32_t offset) noexcept
{
    return reinterpret_cast<const Atom*>(reinterpret_cast<const uint8_t*>(base) + offset);
}

inline Atom* offset_by(Atom* base, uint32_t offset) noexcept
{
    return reinterpret_cast<Atom*>(reinterpret_cast<uint8_t*>(base) + offset);
}

constexpr uint32_t contents_offset(const Atom* base, const Atom* atom) noexcept
{
    return detail::distance(base, atom) + atom_header_length;
}

} // namespace detail

//===------------------------------------------------------------------------===
//
// • Bounding iterators
//
//===------------------------------------------------------------------------===

template <typename Type_>
const Atom* data_atom(const Type_* contents, uint32_t contents_length) noexcept(false)
{
    if ( !valid_alignment_and_length(contents, contents_length) ) {
        throw false;
    }

    auto data = reinterpret_cast<const Atom*>(contents);

    if ( !valid_data(data) ) {
        throw false;
    }

    return data;
}

template <typename Type_>
Atom* data_atom(Type_* contents, uint32_t contents_length) noexcept(false)
{
    if ( !valid_alignment_and_length(contents, contents_length) ) {
        throw false;
    }

    auto data = reinterpret_cast<Atom*>(contents);

    if ( !valid_data(data) ) {
        throw false;
    }

    return data;
}

template <typename Type_>
const Atom* end_atom(const Type_* contents, uint32_t contents_length) noexcept(false)
{
    if ( !valid_alignment_and_length(contents, contents_length) ) {
        throw false;
    }

    auto end = detail::offset_by<Atom>(contents, contents_length - atom_header_length);

    if ( !valid_end(end) ) {
        throw false;
    }

    return end;
}

template <typename Type_>
Atom* end_atom(Type_* contents, uint32_t contents_length) noexcept(false)
{
    if ( !valid_alignment_and_length(contents, contents_length) ) {
        throw false;
    }

    auto end = detail::offset_by<Atom>(contents, contents_length - atom_header_length);

    if ( !valid_end(end) ) {
        throw false;
    }

    return end;
}

//===------------------------------------------------------------------------===
//
// • Contents
//
//===------------------------------------------------------------------------===

Atom* format( void* buffer, uint32_t buffer_length,
              uint32_t data_contents_size = 0u ) noexcept(false);

/*
template <TrivialLayout Data_>
std::pair<Data_*,AtomIterator>
prepare_resource_after(void* buffer, uint32_t buffer_length, const Data_& src_data) noexcept(false)
{
    auto rsrcIt = prepare_resource( buffer, buffer_length, data::aligned_size<Data_>() );
    auto data   = static_cast<Data_*>(buffer);

    *data = src_data;

    return { data, rsrcIt };
}

template <TrivialLayout Data_>
constexpr uint32_t distance(const Data_* data, const AtomIterator& it) noexcept
{
    return data::distance( data, it.get() );
}
*/

} // namespace data

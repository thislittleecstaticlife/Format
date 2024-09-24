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

#include <Format/Layout.hpp>

#if !defined ( __METAL_VERSION__ )
#include <cassert>
#include <iterator>
#endif

//===------------------------------------------------------------------------===
// • namespace format
//===------------------------------------------------------------------------===

namespace format
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
    // ([length] 'aloc'
    //  [length] 'free'?)*
    //  [    16] 'end '

    data       = 'data',
    allocation = 'aloc',
    free       = 'free',
    end        = 'end ',
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

#if !defined ( __METAL_VERSION__ )

static_assert( format::is_structural_layout<Atom>(), "Unexpected layout" );

//===------------------------------------------------------------------------===
//
// • Validation
//
//===------------------------------------------------------------------------===

template <typename Type_>
bool valid_alignment_and_length(const Type_* contents, uint32_t contents_length) noexcept
{
    if (   !is_aligned(contents)
        || !is_aligned(contents_length)
        || contents_length < min_contents_length )
    {
        return false;
    }

    return true;
}

bool valid_data(const Atom* data, uint32_t contents_length) noexcept;
bool valid_end(const Atom* end) noexcept;

bool valid_alignment(const Atom* atom) noexcept;

bool validate_layout(const void* contents, uint32_t contents_length) noexcept;

//===------------------------------------------------------------------------===
//
// • Iteration
//
//===------------------------------------------------------------------------===

namespace unchecked
{

//===------------------------------------------------------------------------===
// • Unchecked Atom utilities
//===------------------------------------------------------------------------===

inline bool empty(const Atom* atom) noexcept
{
    return atom_header_length == atom->length;
}

inline uint32_t contents_size(const Atom* atom) noexcept
{
    return atom->length - atom_header_length;
}

template <TrivialLayout Type_>
uint32_t capacity(const Atom* atom) noexcept
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

template <class Type_>
const Atom* offset_by(const Type_* base, uint32_t offset) noexcept
{
    return reinterpret_cast<const Atom*>(reinterpret_cast<const uint8_t*>(base) + offset);
}

template <class Type_>
Atom* offset_by(Type_* base, uint32_t offset) noexcept
{
    return reinterpret_cast<Atom*>(reinterpret_cast<uint8_t*>(base) + offset);
}

template <class Type_>
const Atom* end(const Type_* contents, uint32_t contents_length) noexcept
{
    const auto end_offset = contents_length - atom_header_length;

    return reinterpret_cast<const Atom*>(reinterpret_cast<const uint8_t*>(contents) + end_offset);
}

template <class Type_>
Atom* end(Type_* contents, uint32_t contents_length) noexcept
{
    const auto end_offset = contents_length - atom_header_length;

    return reinterpret_cast<Atom*>(reinterpret_cast<uint8_t*>(contents) + end_offset);
}

} // namespace unchecked

namespace detail
{

//===------------------------------------------------------------------------===
//
// • __AtomIterator
//
//===------------------------------------------------------------------------===

template <bool IsMutable_>
class __AtomIterator
{
public:

    // • Convenience
    //
    template <typename Type_>
    using mutability_for = typename std::conditional_t<IsMutable_, Type_, const Type_>;

    // • Types
    //
    using value_type      = mutability_for<Atom>;
    using difference_type = int32_t;
    using pointer         = value_type*;
    using reference       = value_type&;
    using byte_pointer    = mutability_for<uint8_t>*;

    using iterator_category = std::bidirectional_iterator_tag;
    using iterator_concept  = std::bidirectional_iterator_tag;

    // • Friends
    //
    friend __AtomIterator<!IsMutable_>;

public:

    // • Initialization
    //
    __AtomIterator(pointer atom, uint32_t offset = 0)
        :
            m_atom  { atom   },
            m_offset{ offset }
    {
        assert( valid_alignment(m_atom) && is_aligned(offset) );
    }

    template <bool MakeConst_>
        requires (!IsMutable_)
    __AtomIterator(const __AtomIterator<MakeConst_>& other)
        :
            m_atom  { other.m_atom },
            m_offset{ other.m_offset }
    {
    }

    __AtomIterator(const __AtomIterator& ) = default;
    __AtomIterator(__AtomIterator&& ) = default;

    ~__AtomIterator(void) = default;

private:

    // • Initialization (Deleted)
    //
    __AtomIterator(void) = delete;

public:

    // • Assignment
    //
    __AtomIterator& operator = (const __AtomIterator& ) = default;
    __AtomIterator& operator = (__AtomIterator&& ) = default;

    // • Accessors
    //
    pointer operator -> (void) const noexcept
    {
        return m_atom;
    }

    reference operator * (void) const noexcept
    {
        return *m_atom;
    }

    pointer get(void) const noexcept
    {
        return m_atom;
    }

    // • Non-standard accessors
    //
    uint32_t offset(void) const noexcept
    {
        return m_offset;
    }

    bool is_begin(void) const noexcept
    {
        return AtomID::data == m_atom->identifier;
    }

    bool is_end(void) const noexcept
    {
        return AtomID::end == m_atom->identifier;
    }

    bool has_contents(void) const noexcept
    {
        return AtomID::data == m_atom->identifier || AtomID::allocation == m_atom->identifier;
    }

    bool empty(void) const noexcept
    {
        return atom_header_length == m_atom->length;
    }

    uint32_t contents_size(void) const noexcept
    {
        assert( has_contents() );

        return m_atom->length - atom_header_length;
    }

    byte_pointer contents(void) const noexcept
    {
        assert( has_contents() );

        return unchecked::contents<uint8_t>(m_atom);
    }

    template <TrivialLayout Type_>
    auto contents(void) const noexcept
    {
        assert( has_contents() );

        return unchecked::contents<Type_>(m_atom);
    }

    // • Comparison (pointer only - offset can be different)
    //
    auto operator <=> (const __AtomIterator& other) const noexcept
    {
        return m_atom <=> other.m_atom;
    }

    bool operator == (const __AtomIterator& other) const noexcept
    {
        return m_atom == other.m_atom;
    }

    // • Forward iteration
    //
    __AtomIterator& operator ++ (void) noexcept
    {
        assert( !is_end() );

        m_offset += m_atom->length;
        m_atom    = unchecked::next(m_atom);

        assert( valid_alignment(m_atom) );

        return *this;
    }

    __AtomIterator operator ++ (int) noexcept
    {
        auto result = *this;
        ++(*this);

        return result;
    }

    // • Reverse iteration
    //
    __AtomIterator& operator -- (void) noexcept
    {
        assert( !is_begin() );

        m_offset -= m_atom->previous;
        m_atom    = unchecked::previous(m_atom);

        assert( valid_alignment(m_atom) );

        return *this;
    }

    __AtomIterator operator -- (int) noexcept
    {
        auto result = *this;
        --(*this);

        return result;
    }

private:

    // • Data members
    //
    pointer     m_atom;
    uint32_t    m_offset;
};

} // namespace data::detail

//===------------------------------------------------------------------------===
// • Iterator types
//===------------------------------------------------------------------------===

using AtomIterator      = detail::__AtomIterator<true>;
using ConstAtomIterator = detail::__AtomIterator<false>;

//===------------------------------------------------------------------------===
//
// • Bounding iterators
//
//===------------------------------------------------------------------------===

template <typename Type_>
ConstAtomIterator data_iterator(const Type_* contents, uint32_t contents_length) noexcept(false)
{
    if ( !valid_alignment_and_length(contents, contents_length) ) {
        throw false;
    }

    auto data = reinterpret_cast<const Atom*>(contents);

    if ( !valid_data(data, contents_length) ) {
        throw false;
    }

    return { data, 0 };
}

template <typename Type_>
AtomIterator data_iterator(Type_* contents, uint32_t contents_length) noexcept(false)
{
    if ( !valid_alignment_and_length(contents, contents_length) ) {
        throw false;
    }

    auto data = reinterpret_cast<Atom*>(contents);

    if ( !valid_data(data, contents_length) ) {
        throw false;
    }

    return { data, 0 };
}

template <typename Type_>
ConstAtomIterator end_iterator(const Type_* contents, uint32_t contents_length) noexcept(false)
{
    if ( !valid_alignment_and_length(contents, contents_length) ) {
        throw false;
    }

    auto end_offset = contents_length - atom_header_length;
    auto end        = unchecked::offset_by(contents, end_offset);

    if ( !valid_end(end) ) {
        throw false;
    }

    return { end, end_offset };
}

template <typename Type_>
AtomIterator end_iterator(Type_* contents, uint32_t contents_length) noexcept(false)
{
    if ( !valid_alignment_and_length(contents, contents_length) ) {
        throw false;
    }

    auto end_offset = contents_length - atom_header_length;
    auto end        = unchecked::offset_by(contents, end_offset);

    if ( !valid_end(end) ) {
        throw false;
    }

    return { end, end_offset };
}

//===------------------------------------------------------------------------===
//
// • Contents
//
//===------------------------------------------------------------------------===

AtomIterator prepare_layout(void* contents, uint32_t data_contents_size, uint32_t contents_length) noexcept(false);

template <StructuralLayout Data_>
    requires( min_contents_length + sizeof(Data_) <= std::numeric_limits<uint32_t>::max() )
AtomIterator prepare_layout(void* contents, uint32_t contents_length) noexcept(false)
{
    return prepare_layout(contents, sizeof(Data_), contents_length);
}

#else // if defined ( __METAL_VERSION__ )

//===------------------------------------------------------------------------===
//
// • Vector access (Metal)
//
//===------------------------------------------------------------------------===

// • Default
//
inline bool empty(const device Atom* header)
{
    return header->length == atom_header_length;
}

inline bool empty(constant Atom* header)
{
    return header->length == atom_header_length;
}

inline uint32_t contents_size(const device Atom* header)
{
    return header->length - atom_header_length;
}

inline uint32_t contents_size(constant Atom* header)
{
    return header->length - atom_header_length;
}

template <TRIVIAL_LAYOUT Type_>
auto contents(const device Atom* header)
{
    return reinterpret_cast<const device Type_*>(header + 1);
}

template <TRIVIAL_LAYOUT Type_>
auto contents(device Atom* header)
{
    return reinterpret_cast<device Type_*>(header + 1);
}

template <TRIVIAL_LAYOUT Type_>
auto contents(constant Atom* header)
{
    return reinterpret_cast<constant Type_*>(header + 1);
}

#endif // defined ( __METAL_VERSION__ )

} // namespace format

//
//  Vector.hpp
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

#include <Format/Allocation.hpp>

#include <algorithm>

//===------------------------------------------------------------------------===
// • namespace format
//===------------------------------------------------------------------------===

namespace format
{

//===------------------------------------------------------------------------===
//
// • VectorRef
//
//===------------------------------------------------------------------------===

template <TRIVIAL_LAYOUT Type_>
struct VectorRef
{
    uint32_t offset;    // Offset from the beginning of the Data atom
    uint32_t count;

    constexpr bool is_null(void) noexcept
    {
        return 0 == offset;
    }

    constexpr bool empty(void) noexcept
    {
        return 0 == count;
    }
};

static_assert( 8 ==  sizeof(VectorRef<int>), "Unexpected size" );
static_assert( 4 == alignof(VectorRef<int>), "Unexpected alignment" );

#if !defined ( __METAL_VERSION__ )

static_assert( format::is_trivial_layout<VectorRef<int>>(), "Unexpected layout" );

//===------------------------------------------------------------------------===
// • VectorRef utilities (Host)
//===------------------------------------------------------------------------===

namespace detail
{

template <TrivialLayout Type_>
AtomIterator allocation_header(AtomIterator dataIt, VectorRef<Type_> ref) noexcept(false)
{
    if ( !is_aligned(ref.offset) || ref.offset < dataIt->length + atom_header_length ) 
    {
        assert( false );
        throw false;
    }

    auto allocation_offset = ref.offset - atom_header_length;
    auto allocation        = unchecked::offset_by(dataIt.get(), allocation_offset);

    if ( AtomID::allocation != allocation->identifier
        || allocation->length < atom_header_length + ref.count*sizeof(Type_) )
    {
        assert( false );
        throw false;
    }

    return { allocation, allocation_offset };
}

} // namespace detail

//===------------------------------------------------------------------------===
//
// • Vector
//
//===------------------------------------------------------------------------===

template <TrivialLayout Type_>
class Vector
{
public:

    // • Types : values
    //
    using vector_ref      = VectorRef<Type_>;
    using value_type      = Type_;
    using size_type       = uint32_t;
    using difference_type = int32_t;

    // • Types : pointers and references
    //
    using reference          = value_type&;
    using const_reference    = const value_type&;
    using pointer            = Type_*;
    using const_pointer      = const Type_*;
    using byte_pointer       = uint8_t*;
    using const_byte_pointer = const uint8_t*;

    // • Types : iterators
    //
    using iterator       = pointer;
    using const_iterator = const_pointer;

    // • Types : reverse iterators
    //
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

public:

    // • Initialization
    //
    Vector(AtomIterator dataIt, vector_ref& ref) noexcept(false)
        :
            m_dataIt    { dataIt  },
            m_ref       { ref     },
            m_allocation{ nullptr }
    {
        if ( !m_ref.is_null() )
        {
            m_allocation = detail::allocation_header(m_dataIt, m_ref).get();
        }
        else if ( !m_ref.empty() )
        {
            throw false;
        }
    }

private:

    // • Initialization (deleted)
    //
    Vector(const Vector& ) = delete;
    Vector(Vector&& ) = delete;
    Vector(void) = delete;

    // • Assignment (deleted)
    //
    Vector& operator = (const Vector& ) = default;
    Vector& operator = (Vector&& ) = default;

public:

    // • Accessors : capacity
    //
    size_type size(void) const noexcept
    {
        return m_ref.count;
    }

    difference_type ssize(void) const noexcept
    {
        return static_cast<difference_type>( size() );
    }

    constexpr size_type max_size(void) const noexcept
    {
        return std::numeric_limits<size_type>::max() / sizeof(value_type);
    }

    bool empty(void) const noexcept
    {
        return m_ref.empty();
    }

    size_type capacity(void) const noexcept
    {
        return (nullptr != m_allocation) ? unchecked::capacity<value_type>(m_allocation) : 0;
    }

    size_type available(void) const noexcept
    {
        return capacity() - size();
    }

    // • Accessors : std::range concept
    //
    pointer begin(void) noexcept
    {
        return data();
    }

    const_pointer cbegin(void) const noexcept
    {
        return cdata();
    }

    const_pointer begin(void) const noexcept
    {
        return cbegin();
    }

    pointer end(void) noexcept
    {
        return begin() + size();
    }

    const_pointer cend(void) const noexcept
    {
        return cbegin() + size();
    }

    const_pointer end(void) const noexcept
    {
        return cend();
    }

    // • Accessors : std::range concept, reverse
    //
    reverse_iterator rbegin(void) noexcept
    {
        return { end() };
    }

    const_reverse_iterator crbegin(void) const noexcept
    {
        return { cend() };
    }

    const_reverse_iterator rbegin(void) const noexcept
    {
        return crbegin();
    }

    reverse_iterator rend(void) noexcept
    {
        return { begin() };
    }

    const_reverse_iterator crend(void) const noexcept
    {
        return { cbegin() };
    }

    const_reverse_iterator rend(void) const noexcept
    {
        return crend();
    }

    // • Accessors : elements
    //
    value_type& at(size_type index) noexcept
    {
        assert( index < m_ref.count );

        return data()[index];
    }

    value_type at(size_type index) const noexcept
    {
        assert( index < m_ref.count );

        return data()[index];
    }

    value_type& front(void) noexcept
    {
        return at(0);
    }

    value_type front(void) const noexcept
    {
        return at(0);
    }

    value_type& back(void) noexcept
    {
        assert( !empty() );

        return data()[m_ref.count - 1];
    }

    value_type back(void) const noexcept
    {
        assert( !empty() );

        return data()[m_ref.count - 1];
    }

    value_type& operator [] (size_type index) noexcept
    {
        return at(index);
    }

    value_type operator [] (size_type index) const noexcept
    {
        return at(index);
    }

    pointer data(void) noexcept
    {
        return (nullptr != m_allocation) ? unchecked::contents<value_type>(m_allocation) : nullptr;
    }

    const_pointer cdata(void) const noexcept
    {
        return (nullptr != m_allocation) ? unchecked::contents<value_type>(m_allocation) : nullptr;
    }

    const_pointer data(void) const noexcept
    {
        return cdata();
    }

    // • Methods : container, capacity
    //
    void reserve(uint32_t capacity) noexcept(false)
    {
        if ( capacity <= this->capacity() )
        {
            // • No-op
            //
            return;
        }

        const auto contents_size = static_cast<uint32_t>( sizeof(value_type) * capacity );

        auto allocIt = ( nullptr != m_allocation )
            ? detail::reserve(m_dataIt, allocation_iterator(), contents_size)
            : detail::reserve(m_dataIt, contents_size);

        update_for(allocIt);
    }

    // * Methods : container
    //
    void clear(void) noexcept
    {
        m_ref.count = 0;
    }

    void shrink_to_fit(void) noexcept
    {
        assert( false ); // TODO: Remove once this path has been tested

        if ( empty() && !m_ref.is_null() )
        {
            assert( false ); // TODO: Remove once this path has been tested

            detail::free( m_dataIt, allocation_iterator() );

            m_allocation = nullptr;
            m_ref.offset = 0;
        }
        else if ( size() < capacity() )
        {
            assert( false ); // TODO: Remove once this path has been tested

            const auto contents_size = static_cast<uint32_t>( sizeof(value_type) * m_ref->count );

            auto allocIt = detail::reserve(m_dataIt, allocation_iterator(), contents_size);

            update_for(allocIt);
        }
    }

    iterator erase(const_iterator begin_pos, const_iterator end_pos) noexcept
    {
        assert( cbegin() <= begin_pos && begin_pos <= end_pos && end_pos <= cend() );

        auto destIt = const_cast<iterator>(begin_pos);

        if ( begin_pos == end_pos )
        {
            return destIt;
        }

        auto erase_count = static_cast<uint32_t>( std::distance(begin_pos, end_pos) );

        if ( end_pos < cend() )
        {
            std::move( const_cast<iterator>(end_pos), end(), destIt );
        }

        m_ref.count -= erase_count;

        return destIt;
    }

    iterator erase(const_iterator pos) noexcept
    {
        if ( pos == cend() )
        {
            // • No-op
            return const_cast<iterator>(pos);
        }

        return erase( pos, std::next(pos) );
    }

    void push_back(const_reference value) noexcept(false)
    {
        if ( capacity() < size() + 1 )
        {
            // • Reserve to multiples of 4 when at capacity (?)
            //
            reserve( (size() + 4) & ~3 );
        }

        unchecked::contents<value_type>(m_allocation)[m_ref.count++] = value;
    }

    void pop_back(void) noexcept
    {
        assert( !empty() );

        --m_ref.count;
    }

    // • Assignment
    //
    template <std::forward_iterator FwdIter_>
        requires std::is_constructible_v<Type_, typename std::iterator_traits<FwdIter_>::value_type>
    void assign(FwdIter_ begin, FwdIter_ end) noexcept(false)
    {
        assert( begin <= end && std::distance(begin, end) <= max_size() );

        if ( begin < end )
        {
            const auto new_count = static_cast<difference_type>( std::distance(begin, end) );

            if ( capacity() < new_count )
            {
                reserve(new_count);
            }

            std::copy( begin, end, data() );

            m_ref.count = new_count;
        }
        else
        {
            clear();
        }
    }

    void assign(std::initializer_list<value_type> ilist) noexcept(false)
    {
        assign( ilist.begin(), ilist.end() );
    }

private:

    // • Utilities (private)
    //
    iterator prepare_insert(const_iterator pos, size_type insert_count) noexcept(false)
    {
        assert( 0 < insert_count );

        const auto insert_offset = std::distance( cbegin(), pos );
        const auto new_count     = size() + insert_count;

        if ( capacity() < new_count )
        {
            reserve(new_count);
        }

        auto destIt = begin() + insert_offset;

        if ( insert_offset < ssize() )
        {
            std::move( destIt, end(), destIt + insert_count );
        }

        m_ref.count = new_count;

        return destIt;
    }

public:

    // • Insertion
    //
    iterator insert(const_iterator pos, size_type count, const_reference value) noexcept(false)
    {
        assert( count <= max_size() && size() <= max_size() - count );
        assert( cbegin() <= pos && pos <= cend() );

        if ( 0 == count )
        {
            // • No-op
            return const_cast<iterator>(pos);
        }
        else
        {
            auto destIt = prepare_insert(pos, count);

            std::fill_n( destIt, count, value );

            return destIt;
        }
    }

    iterator insert(const_iterator pos, const_reference value) noexcept(false)
    {
        return insert(pos, 1, value);
    }

    template <std::forward_iterator FwdIter_>
        requires std::is_constructible_v<Type_, typename std::iterator_traits<FwdIter_>::value_type>
    iterator insert(const_iterator pos, FwdIter_ begin, FwdIter_ end) noexcept(false)
    {
        assert( std::distance(begin, end) <= max_size() );
        assert( size() <= max_size() - std::distance(begin, end) );
        assert( cbegin() <= pos && pos <= cend() );

        const auto insert_count = static_cast<size_type>( std::distance(begin, end) );

        if ( 0 == insert_count )
        {
            // • No-op
            return const_cast<iterator>(pos);
        }
        else
        {
            auto destIt = prepare_insert(pos, insert_count);

            std::copy( begin, end, destIt );

            return destIt;
        }
    }

    iterator insert(const_iterator pos, std::initializer_list<value_type> ilist) noexcept(false)
    {
        return insert( pos, ilist.begin(), ilist.end() );
    }

private:

    // • Private Methods
    //
    void update_for(AtomIterator allocIt) noexcept
    {
        m_allocation = allocIt.get();
        m_ref.offset = allocIt.offset() + atom_header_length;
    }

    AtomIterator allocation_iterator(void) noexcept
    {
        return { m_allocation, m_ref.offset - atom_header_length };
    }

private:

    // • Data members
    //
    AtomIterator    m_dataIt;
    vector_ref&     m_ref;
    Atom*           m_allocation;
};

#else // if defined ( __METAL_VERSION__ )

//===------------------------------------------------------------------------===
//
// • Vector utilities (Metal)
//
//===------------------------------------------------------------------------===

template <TRIVIAL_LAYOUT Type_>
const device Type_* contents(const device Atom* data, VectorRef<Type_> ref)
{
    return reinterpret_cast<const device Type_*>( reinterpret_cast<const device uint8_t*>(data) + ref.offset );
}

template <TRIVIAL_LAYOUT Type_>
device Type_* contents(device Atom* data, VectorRef<Type_> ref)
{
    return reinterpret_cast<device Type_*>( reinterpret_cast<device uint8_t*>(data) + ref.offset );
}

template <TRIVIAL_LAYOUT Type_>
constant Type_* contents(constant Atom* data, VectorRef<Type_> ref)
{
    return reinterpret_cast<constant Type_*>( reinterpret_cast<constant uint8_t*>(data) + ref.offset );
}

#endif

} // namespace format

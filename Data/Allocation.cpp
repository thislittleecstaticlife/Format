//
//  Allocation.cpp
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

#include <Data/Allocation.hpp>

namespace data
{

//===------------------------------------------------------------------------===
// • Allocation primitives
//===------------------------------------------------------------------------===

namespace detail
{

constexpr uint32_t get_allocation_length(uint32_t requested_contents_size) noexcept
{
    return atom_header_length + aligned_size(requested_contents_size);
}

Atom* divide(Atom* atom, uint32_t slice_length, AtomID identifier) noexcept
{
    // • First create the tail region fully within the region to divide
    //
    auto tail = detail::offset_by(atom, slice_length);

    tail->identifier = identifier;
    tail->length     = atom->length - slice_length;
    tail->previous   = slice_length;
    tail->reserved   = 0;

    // • Link the next atom backwards to the tail
    //
    detail::next(tail)->previous = tail->length;

    // • Detach the tail
    //
    atom->length = slice_length;

    return tail;
}

void merge_next(Atom* atom) noexcept
{
    atom->length                += detail::next(atom)->length;
    detail::next(atom)->previous = atom->length;
}

Atom* reserve_new(Atom* data, uint32_t allocation_length, AtomID identifier) noexcept(false)
{
    for ( auto atom = next(data); !is_end(atom); atom = next(atom) )
    {
        if ( AtomID::free != atom->identifier || atom->length < allocation_length ) {
            continue;
        }

        if ( allocation_length < atom->length )
        {
            // • Divide the free region into two sub-regions (ignore the second)
            //
            divide( atom, allocation_length, AtomID::free );
        }

        // • Reclaim the beginning of the region as the new allocation
        //
        atom->identifier = identifier;

        return atom;
    }

    // • For now we want to stop if this occurs because it means we didn't allocate
    //   a large enough contents buffer, or it has become too fragmented
    //
    assert( false );

    throw false;
}

Atom* reserve(Atom* data, uint32_t requested_contents_size, AtomID identifier) noexcept(false)
{
    assert( AtomID::data == data->identifier );
    assert( AtomID::vector == identifier );

    return reserve_new( data, get_allocation_length(requested_contents_size), identifier );
}

Atom* reserve(Atom* data, Atom* curr_alloc, uint32_t requested_contents_size) noexcept(false)
{
    assert( AtomID::data == data->identifier );
    assert( AtomID::vector == curr_alloc->identifier );

    const auto allocation_length = get_allocation_length(requested_contents_size);

    if ( allocation_length == curr_alloc->length )
    {
        // • Keeping the same allocation size, perhaps unintended but technically not wrong
        //
        return curr_alloc;
    }
    else if ( allocation_length < curr_alloc->length )
    {
        // • Smaller allocation - free the tail
        //
        auto free = divide(curr_alloc, allocation_length, AtomID::free);

        if ( AtomID::free == next(free)->identifier )
        {
            merge_next(free);
        }

        return curr_alloc;
    }
    else
    {
        // • Larger allocation - first try to extend into the immediately following
        //      region if it's a free region of sufficient length
        //
        const auto extend_length = allocation_length - curr_alloc->length;

        if ( auto extend = next(curr_alloc) ;
            !is_end(extend)
            && AtomID::free == extend->identifier
            && extend_length <= extend->length )
        {
            if ( extend_length <= extend->length )
            {
                divide(extend, extend_length, AtomID::free);
            }

            // • Acquire the free region
            //
            merge_next(curr_alloc);

            return curr_alloc;
        }

        // TODO: Try to claim all or part of the preceding region if it's free

        // • Finally, perform a new full allocation, copy the existing contents,
        //      and free the previous allocation
        //
        auto new_alloc = reserve_new(data, allocation_length, curr_alloc->identifier);

        std::memcpy( contents<uint8_t>(new_alloc), contents<uint8_t>(curr_alloc),
                     contents_size(curr_alloc) );

        free(curr_alloc);

        return new_alloc;
    }

    // • For now we want to stop if this occurs because it means we didn't allocate
    //   a large enough Resource region, or it has become too fragmented
    //
    assert( false );

    throw false;
}

Atom* free(Atom* dealloc) noexcept
{
    assert( AtomID::vector == dealloc->identifier );

    // • Convert to free region of the same length
    //
    dealloc->identifier = AtomID::free;

    // • First try to coalesce with the immediately following region if free
    //
    if ( AtomID::free == next(dealloc)->identifier )
    {
        merge_next(dealloc);
    }

    // • Then try to coalesce with the immediately preceding region if free
    //
    auto prev = previous(dealloc);

    if ( AtomID::free == prev->identifier )
    {
        merge_next(prev);

        return prev;
    }

    return dealloc;
}

} // namespace detail

} // namespace data

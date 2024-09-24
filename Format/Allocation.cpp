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

#include <Format/Allocation.hpp>

//===------------------------------------------------------------------------===
// • namespace format
//===------------------------------------------------------------------------===

namespace format
{

//===------------------------------------------------------------------------===
// • Allocation primitives
//===------------------------------------------------------------------------===

namespace detail
{

uint32_t get_allocation_length(uint32_t requested_contents_size) noexcept
{
    return atom_header_length + aligned_size(requested_contents_size);
}

Atom* divide(Atom* atom, uint32_t slice_length, AtomID identifier) noexcept
{
    // • First create the tail region fully within the region to divide
    //
    auto tail = unchecked::offset_by(atom, slice_length);

    tail->identifier = identifier;
    tail->length     = atom->length - slice_length;
    tail->previous   = slice_length;
    tail->reserved   = 0;

    // • Link the next atom backwards to the tail
    //
    unchecked::next(tail)->previous = tail->length;

    // • Detach the tail
    //
    atom->length = slice_length;

    return tail;
}

void merge_next(Atom* atom) noexcept
{
    atom->length                   += unchecked::next(atom)->length;
    unchecked::next(atom)->previous = atom->length;
}

AtomIterator reserve_new(AtomIterator dataIt, uint32_t allocation_length) noexcept(false)
{
    assert( AtomID::data == dataIt->identifier );

    for ( auto atomIt = std::next(dataIt); !atomIt.is_end(); ++atomIt )
    {
        if ( AtomID::free != atomIt->identifier || atomIt->length < allocation_length ) {
            continue;
        }

        if ( allocation_length < atomIt->length )
        {
            // • Divide the free region into two sub-regions (ignore the second)
            //
            divide( atomIt.get(), allocation_length, AtomID::free );
        }

        // • Reclaim the beginning of the region as the new allocation
        //
        atomIt->identifier = AtomID::allocation;

        return atomIt;
    }

    // • For now we want to stop if this occurs because it means we didn't allocate
    //   a large enough contents buffer, or it has become too fragmented
    //
    assert( false );

    throw false;
}

AtomIterator reserve( AtomIterator dataIt, uint32_t requested_contents_size ) noexcept(false)
{
    return reserve_new( dataIt, get_allocation_length(requested_contents_size) );
}

AtomIterator reserve( AtomIterator dataIt, AtomIterator currAllocIt,
                      uint32_t requested_contents_size ) noexcept(false)
{
    const auto allocation_length = get_allocation_length(requested_contents_size);

    if ( allocation_length == currAllocIt->length )
    {
        // • Keeping the same allocation size, perhaps unintended but technically not wrong
        //
        return currAllocIt;
    }
    else if ( allocation_length < currAllocIt->length )
    {
        // • Smaller allocation - free the tail
        //
        auto free = divide(currAllocIt.get(), allocation_length, AtomID::free);

        if ( AtomID::free == unchecked::next(free)->identifier )
        {
            merge_next(free);
        }

        return currAllocIt;
    }
    else
    {
        // • Larger allocation - first try to extend into the immediately following
        //      region if it's a free region of sufficient length
        //
        const auto extend_length = allocation_length - currAllocIt->length;

        if ( auto extendIt = std::next(currAllocIt) ;
            !extendIt.is_end()
            && AtomID::free == extendIt->identifier
            && extend_length <= extendIt->length )
        {
            if ( extend_length <= extendIt->length )
            {
                divide(extendIt.get(), extend_length, AtomID::free);
            }

            // • Acquire the free region
            //
            merge_next( currAllocIt.get() );

            return currAllocIt;
        }

        // TODO: Try to claim all or part of the preceding region if it's free

        // • Finally, perform a new full allocation, copy the existing contents,
        //      and free the previous allocation
        //
        auto newAllocIt = reserve_new(dataIt, allocation_length);

        if ( newAllocIt.is_end() ) {
            return newAllocIt;
        }

        memcpy( newAllocIt.contents(), currAllocIt.contents(), currAllocIt.contents_size() );

        free(currAllocIt);

        return newAllocIt;
    }

    // • For now we want to stop if this occurs because it means we didn't allocate
    //   a large enough Resource region, or it has become too fragmented
    //
    assert( false );

    throw false;
}

AtomIterator free(AtomIterator deallocIt) noexcept
{
    assert( AtomID::allocation == deallocIt->identifier );

    // • Convert to free region of the same length
    //
    deallocIt->identifier = AtomID::free;

    // • First try to coalesce with the immediately following region if free
    //
    if ( AtomID::free == std::next(deallocIt)->identifier )
    {
        merge_next( deallocIt.get() );
    }

    // • Then try to coalesce with the immediately preceding region if free
    //
    auto prevIt = std::prev(deallocIt);

    if ( AtomID::free == prevIt->identifier )
    {
        merge_next( prevIt.get() );

        return prevIt;
    }

    return deallocIt;
}

} // namespace detail

} // namespace format

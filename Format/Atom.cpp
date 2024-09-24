//
//  Atom.cpp
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

#include <Format/Atom.hpp>

//===------------------------------------------------------------------------===
// • namespace format
//===------------------------------------------------------------------------===

namespace format
{

//===------------------------------------------------------------------------===
//
// • Validation
//
//===------------------------------------------------------------------------===

bool valid_alignment(const Atom* atom) noexcept
{
    if (   !is_aligned(atom)
        || !is_aligned(atom->length)
        || !is_aligned(atom->previous)
        || atom->length < atom_header_length )
    {
        return false;
    }

    return true;
}

bool valid_data(const Atom* data, uint32_t contents_length) noexcept
{
    if ( !is_aligned(data)
        || contents_length < min_contents_length
        || AtomID::data != data->identifier
        || data->length < atom_header_length
        || !is_aligned(data->length)
        || contents_length - atom_header_length < data->length
        || 0 != data->previous )
    {
        return false;
    }

    return true;
}

bool valid_end(const Atom* end) noexcept
{
    if ( !is_aligned(end)
        || AtomID::end != end->identifier
        || atom_header_length != end->length
        || !is_aligned(end->previous) )
    {
        return false;
    }

    return true;
}

bool validate_layout(const void* contents, uint32_t contents_length) noexcept
{
    // • Contents alignment and length
    //
    if ( !valid_alignment_and_length(contents, contents_length) )
    {
        return false;
    }

    // • The first atom is 'data'
    //
    const Atom* data = reinterpret_cast<const Atom*>(contents);

    if ( AtomID::data != data->identifier
        || !is_aligned(data->length)
        || data->length < atom_header_length
        || contents_length - atom_header_length < data->length
        || 0 != data->previous )
    {
        return false;
    }

    // • The last atom is 'end ', which has no content
    //
    const auto end = unchecked::end(contents, contents_length);

    if ( AtomID::end != end->identifier || !unchecked::empty(end) ) {
        return false;
    }

    // • Validate each atom forward to 'end '
    //
    const Atom* curr = unchecked::next(data);
    const Atom* prev = data;

    for ( auto end_distance = contents_length - data->length - end->length ;
          0 < end_distance ;
          end_distance -= curr->length, prev = curr, curr = unchecked::next(curr) )
    {
        if ( !is_aligned(curr->length) || end_distance < curr->length ) {
            return false;
        }

        if ( AtomID::allocation == curr->identifier )
        {
            // • There shall be no zero-length vector atoms
            //
            if ( unchecked::empty(curr) ) {
                return false;
            }
        }
        else if ( AtomID::free == curr->identifier )
        {
            // • There shall be no sequential free atoms
            //
            if ( AtomID::free == prev->identifier ) {
                return false;
            }
        }
        else
        {
            // • Currently only two atom types before 'end '
            //
            return false;
        }

        if (prev->length != curr->previous) {
            return false;
        }
    }

    if ( curr != end ) {
        return false;
    }

    return true;
}

//===------------------------------------------------------------------------===
//
// • Contents
//
//===------------------------------------------------------------------------===

AtomIterator prepare_layout(void* contents, uint32_t data_contents_size, uint32_t contents_length) noexcept(false)
{
    // • Validate alignment and minimum possible size
    //
    if (   !is_aligned(contents)
        || !is_aligned(contents_length)
        || !is_aligned(data_contents_size)
        || contents_length < 2*sizeof(Atom) + data_contents_size )
    {
        throw false;
    }

    // • Data
    //
    auto data = static_cast<Atom*>(contents);

    *data = {
        .length     = atom_header_length + data_contents_size,
        .identifier = AtomID::data,
        0
    };

    // • End
    //
    auto end_offset = contents_length - atom_header_length;
    auto end        = unchecked::offset_by(contents, end_offset);

    if ( data->length < end_offset )
    {
        // • Free
        //
        auto free = unchecked::next(data);

        *free = {
            .length     = contents_length - data->length - atom_header_length,
            .identifier = AtomID::free,
            .previous   = data->length,
            0
        };

        *end = {
            .length     = atom_header_length,
            .identifier = AtomID::end,
            .previous   = free->length,
            0
        };

        return { data, 0 };
    }
    else
    {
        *end = {
            .length     = atom_header_length,
            .identifier = AtomID::end,
            .previous   = data->length,
            0
        };

        return { data, 0 };
    }
}

} // namespace format

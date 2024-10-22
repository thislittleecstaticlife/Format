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

#include <Data/Atom.hpp>

//===------------------------------------------------------------------------===
// • namespace data
//===------------------------------------------------------------------------===

namespace data
{

//===------------------------------------------------------------------------===
//
// • Validation
//
//===------------------------------------------------------------------------===

bool valid_data(const Atom* data) noexcept
{
    if (   !is_aligned(data)
        || AtomID::data != data->identifier
        || !is_aligned(data->length)
        || data->length < atom_header_length
        || 0 != data->previous )
    {
        return false;
    }

    return true;
}

bool valid_end(const Atom* end) noexcept
{
    if (   !is_aligned(end)
        || AtomID::end != end->identifier
        || atom_header_length != end->length
        || !is_aligned(end->previous) )
    {
        return false;
    }

    return true;
}

bool valid_alignment_and_length(const void* contents, uint32_t contents_length) noexcept
{
    if (   !is_aligned(contents)
        || !is_aligned(contents_length)
        || contents_length < min_contents_length )
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

    if ( !valid_data(data) )
    {
        return false;
    }

    // • The last atom is 'end ', which has no content
    //
    const auto end = detail::offset_by(data, contents_length - atom_header_length);

    if ( AtomID::end != end->identifier || !detail::empty(end) ) {
        return false;
    }

    // • Validate each atom forward to 'end '
    //
    const Atom* curr = detail::next(data);
    const Atom* prev = data;

    for ( auto end_distance = contents_length - data->length - end->length ;
          0 < end_distance ;
          end_distance -= curr->length, prev = curr, curr = detail::next(curr) )
    {
        if ( !is_aligned(curr->length) || end_distance < curr->length ) {
            return false;
        }

        if ( AtomID::vector == curr->identifier )
        {
            // • There shall be no zero-length vector atoms
            //
            if ( detail::empty(curr) ) {
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

Atom* format(void* buffer, uint32_t buffer_length, uint32_t data_contents_size) noexcept(false)
{
    // • Validate alignment and minimum possible size
    //
    const auto aligned_data_contents_size = aligned_size(data_contents_size);

    if (   !is_aligned(buffer)
        || !is_aligned(buffer_length)
        || buffer_length < aligned_data_contents_size + 2*sizeof(Atom) )
    {
        throw false;
    }

    // • Data
    //
    auto data = static_cast<Atom*>(buffer);

    *data = {
        .length     = atom_header_length + aligned_data_contents_size,
        .identifier = AtomID::data,
        0
    };

    // • Zero-init the data contents
    //
    if ( 0 < aligned_data_contents_size )
    {
        std::memset( detail::contents<uint8_t>(data), 0, aligned_data_contents_size );
    }

    // • End
    //
    auto end_offset = buffer_length - atom_header_length;
    auto end        = detail::offset_by(data, end_offset);

    if ( data->length < end_offset )
    {
        // • Free
        //
        auto free = detail::next(data);

        *free = {
            .length     = buffer_length - data->length - atom_header_length,
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
    }
    else
    {
        *end = {
            .length     = atom_header_length,
            .identifier = AtomID::end,
            .previous   = data->length,
            0
        };
    }

    return data;
}

} // namespace data

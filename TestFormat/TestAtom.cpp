//
//  TestAtom.cpp
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

#include <gmock/gmock.h>

#include <Data/Atom.hpp>

using namespace ::testing;
using namespace ::data;

//===------------------------------------------------------------------------===
//
// • Atom tests
//
//===------------------------------------------------------------------------===


TEST( atom, static_data )
{
    try
    {
        const auto contents = std::vector<uint8_t>{ {

            // Atom: length, identifier, previous, user_defined
            16,0,0,0,   'a','t','a','d',     0,0,0,0,   0,0,0,0,
            32,0,0,0,   'e','e','r','f',    16,0,0,0,   0,0,0,0,
                0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
            48,0,0,0,   'r','t','c','v',    32,0,0,0,   0,0,0,0,
                0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
                0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
            16,0,0,0,   ' ','d','n','e',    48,0,0,0,   0,0,0,0,
        } };

        const auto contents_length = static_cast<uint32_t>( contents.size() );

        EXPECT_TRUE( validate_layout(contents.data(), contents_length) );

        const auto data = reinterpret_cast<const Atom*>( contents.data() );

        EXPECT_EQ( data->identifier, AtomID::data );
        EXPECT_EQ( data->length, 16 );
        EXPECT_TRUE( detail::empty(data) );

        const auto free = detail::next(data);

        EXPECT_EQ( free->identifier, AtomID::free );
        EXPECT_EQ( free->length, 32 );
        EXPECT_EQ( free->previous, data->length );
        EXPECT_FALSE( detail::empty(free) );

        const auto vctr = detail::next(free);

        EXPECT_EQ( vctr->identifier, AtomID::vector );
        EXPECT_EQ( vctr->length, 48 );
        EXPECT_EQ( vctr->previous, 32 );
        EXPECT_EQ( vctr->previous, free->length );
        EXPECT_FALSE( detail::empty(vctr) );

        const auto end = end_atom( contents.data(), contents_length );

        EXPECT_EQ( detail::next(vctr), end );
        EXPECT_EQ( end->identifier, AtomID::end );
        EXPECT_EQ( end->length, atom_header_length );
        EXPECT_EQ( end->previous, vctr->length );
        EXPECT_TRUE( detail::empty(end) );

        EXPECT_EQ( data, detail::previous(free) );
        EXPECT_EQ( free, detail::previous(vctr) );
        EXPECT_EQ( vctr, detail::previous(end) );
    }
    catch ( ... )
    {
        FAIL();
    }
}

TEST( atom, default_layout )
{
    try
    {
        auto contents_length = uint32_t{ 1024 };
        auto contents        = std::make_unique<uint8_t[]>(contents_length);

        const auto data = format( contents.get(), contents_length );

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        EXPECT_EQ( data->identifier, AtomID::data );
        EXPECT_EQ( data->length, 16 );
        EXPECT_TRUE( detail::empty(data) );

        const auto free = detail::next(data);

        EXPECT_EQ( free->identifier, AtomID::free );
        EXPECT_EQ( free->length, contents_length - 2*atom_header_length );
        EXPECT_EQ( free->previous, data->length );
        EXPECT_FALSE( detail::empty(free) );

        const auto end = end_atom( contents.get(), contents_length );

        EXPECT_EQ( detail::next(free), end );
        EXPECT_EQ( end->identifier, AtomID::end );
        EXPECT_EQ( end->length, atom_header_length );
        EXPECT_EQ( end->previous, free->length );
        EXPECT_TRUE( detail::empty(end) );

        EXPECT_EQ( data, detail::previous(free) );
        EXPECT_EQ( free, detail::previous(end) );
    }
    catch ( ... )
    {
        FAIL();
    }
}

TEST( atom, non_empty_data_layout )
{
    try
    {
        auto contents_length = uint32_t{ 1024 };
        auto contents        = std::make_unique<uint8_t[]>(contents_length);

        const auto data = format( contents.get(), contents_length, 27 );

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        EXPECT_EQ( data->identifier, AtomID::data );
        EXPECT_EQ( data->length, 48 );
        EXPECT_FALSE( detail::empty(data) );

        const auto free = detail::next(data);

        EXPECT_EQ( free->identifier, AtomID::free );
        EXPECT_EQ( free->length, contents_length - 2*atom_header_length - 32 );
        EXPECT_EQ( free->previous, data->length );
        EXPECT_FALSE( detail::empty(free) );

        const auto end = end_atom( contents.get(), contents_length );

        EXPECT_EQ( detail::next(free), end );
        EXPECT_EQ( end->identifier, AtomID::end );
        EXPECT_EQ( end->length, atom_header_length );
        EXPECT_EQ( end->previous, free->length );
        EXPECT_TRUE( detail::empty(end) );

        EXPECT_EQ( data, detail::previous(free) );
        EXPECT_EQ( free, detail::previous(end) );
    }
    catch ( ... )
    {
        FAIL();
    }
}

TEST( atom, minimum_layout )
{
    try
    {
        auto contents_length = uint32_t{ 2*atom_header_length };
        auto contents        = std::make_unique<uint8_t[]>(contents_length);

        const auto data = format( contents.get(), contents_length );

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        EXPECT_EQ( data->identifier, AtomID::data );
        EXPECT_EQ( data->length, 16 );
        EXPECT_TRUE( detail::empty(data) );

        const auto end = end_atom( contents.get(), contents_length );

        EXPECT_EQ( detail::next(data), end );
        EXPECT_EQ( end->identifier, AtomID::end );
        EXPECT_EQ( end->length, atom_header_length );
        EXPECT_EQ( end->previous, data->length );
        EXPECT_TRUE( detail::empty(end) );

        EXPECT_EQ( data, detail::previous(end) );
    }
    catch ( ... )
    {
        FAIL();
    }
}

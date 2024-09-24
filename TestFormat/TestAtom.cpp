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

#include <Format/Atom.hpp>

using namespace ::testing;
using namespace ::format;

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
            48,0,0,0,   'c','o','l','a',    32,0,0,0,   0,0,0,0,
                0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
                0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
            16,0,0,0,   ' ','d','n','e',    48,0,0,0,   0,0,0,0,
        } };

        const auto contents_length = static_cast<uint32_t>( contents.size() );

        EXPECT_TRUE( validate_layout(contents.data(), contents_length) );

        auto dataIt = data_iterator(contents.data(), contents_length);

        EXPECT_EQ( dataIt->identifier, AtomID::data );
        EXPECT_EQ( dataIt->length, 16 );
        EXPECT_TRUE( dataIt.empty() );

        auto freeIt = std::next(dataIt);

        EXPECT_EQ( freeIt->identifier, AtomID::free );
        EXPECT_EQ( freeIt->length, 32 );
        EXPECT_EQ( freeIt->previous, dataIt->length );
        EXPECT_FALSE( freeIt.empty() );

        auto allocIt = std::next(freeIt);

        EXPECT_EQ( allocIt->identifier, AtomID::allocation );
        EXPECT_EQ( allocIt->length, 48 );
        EXPECT_EQ( allocIt->previous, 32 );
        EXPECT_EQ( allocIt->previous, freeIt->length );
        EXPECT_FALSE( allocIt.empty() );

        auto endIt = end_iterator(contents.data(), contents_length);

        EXPECT_EQ( std::next(allocIt), endIt );
        EXPECT_EQ( endIt->identifier, AtomID::end );
        EXPECT_EQ( endIt->length, atom_header_length );
        EXPECT_EQ( endIt->previous, allocIt->length );
        EXPECT_TRUE( endIt.empty() );

        EXPECT_EQ( dataIt, std::prev(freeIt) );
        EXPECT_EQ( freeIt, std::prev(allocIt) );
        EXPECT_EQ( allocIt, std::prev(endIt) );
    }
    catch ( ... )
    {
        EXPECT_TRUE( false );
    }
}

TEST( atom, default_layout )
{
    try
    {
        auto contents_length = uint32_t{ 1024 };
        auto contents        = std::make_unique<uint8_t[]>(contents_length);

        auto dataIt = prepare_layout(contents.get(), 0, contents_length);

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        EXPECT_EQ( dataIt->identifier, AtomID::data );
        EXPECT_EQ( dataIt->length, 16 );
        EXPECT_TRUE( dataIt.empty() );

        auto freeIt = std::next(dataIt);

        EXPECT_EQ( freeIt->identifier, AtomID::free );
        EXPECT_EQ( freeIt->length, contents_length - 2*atom_header_length );
        EXPECT_EQ( freeIt->previous, dataIt->length );
        EXPECT_FALSE( freeIt.empty() );

        auto endIt = end_iterator(contents.get(), contents_length);

        EXPECT_EQ( std::next(freeIt), endIt );
        EXPECT_EQ( endIt->identifier, AtomID::end );
        EXPECT_EQ( endIt->length, atom_header_length );
        EXPECT_EQ( endIt->previous, freeIt->length );
        EXPECT_TRUE( endIt.empty() );

        EXPECT_EQ( dataIt, std::prev(freeIt) );
        EXPECT_EQ( freeIt, std::prev(endIt) );
    }
    catch ( ... )
    {
        EXPECT_TRUE( false );
    }
}

TEST( atom, minimum_layout )
{
    try
    {
        auto contents_length = uint32_t{ 2*atom_header_length };
        auto contents        = std::make_unique<uint8_t[]>(contents_length);

        auto dataIt = prepare_layout(contents.get(), 0, contents_length);

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        EXPECT_EQ( dataIt->identifier, AtomID::data );
        EXPECT_EQ( dataIt->length, 16 );
        EXPECT_TRUE( dataIt.empty() );

        auto endIt = end_iterator(contents.get(), contents_length);

        EXPECT_EQ( std::next(dataIt), endIt );
        EXPECT_EQ( endIt->identifier, AtomID::end );
        EXPECT_EQ( endIt->length, atom_header_length );
        EXPECT_EQ( endIt->previous, dataIt->length );
        EXPECT_TRUE( endIt.empty() );

        EXPECT_EQ( dataIt, std::prev(endIt) );
    }
    catch ( ... )
    {
        EXPECT_TRUE( false );
    }
}

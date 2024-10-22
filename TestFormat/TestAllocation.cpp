//
//  TestAllocation.cpp
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

#include <Data/Allocation.hpp>

using namespace ::testing;
using namespace ::data;

//===------------------------------------------------------------------------===
//
// • Allocation tests
//
//===------------------------------------------------------------------------===

TEST( allocation, new_reservation )
{

    try
    {
        auto contents_length = uint32_t{ 1024 };
        auto contents        = std::make_unique<uint8_t[]>(contents_length);
        auto data            = format( contents.get(), contents_length );

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        // • First
        //
        auto alloc1 = detail::reserve(data, 34, AtomID::vector);

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        EXPECT_EQ( alloc1->identifier, AtomID::vector );
        EXPECT_EQ( detail::distance(data, alloc1), atom_header_length );
        EXPECT_EQ( detail::contents_size(alloc1), 48 ); // aligned_size(34) = 48
        EXPECT_EQ( detail::next(data), alloc1 );
        EXPECT_EQ( detail::next(alloc1)->identifier, AtomID::free );

        // • Second
        //
        auto alloc2 = detail::reserve(data, 512, AtomID::vector);

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        EXPECT_EQ( alloc2->identifier, AtomID::vector );
        EXPECT_EQ( alloc2->length, 528 );
        EXPECT_EQ( detail::distance(data, alloc2), 80 );
        EXPECT_EQ( detail::contents_size(alloc2), 512 );
        EXPECT_EQ( detail::next(alloc1), alloc2 );
        EXPECT_EQ( detail::next(alloc2)->identifier, AtomID::free );

        // • Deallocate first
        //
        auto free1 = detail::free(alloc1);

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        EXPECT_EQ( detail::next(data), free1 );
        EXPECT_EQ( free1->identifier, AtomID::free );
        EXPECT_EQ( free1->length, 64 );
        EXPECT_EQ( data, detail::previous(free1) );

        EXPECT_EQ( detail::next(free1), alloc2 );
        EXPECT_EQ( free1, detail::previous(alloc2) );

        // • Deallocate second
        //
        auto free2 = detail::free(alloc2);

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        EXPECT_EQ( detail::next(data), free2 );
        EXPECT_EQ( data, detail::previous(free2) );

        EXPECT_EQ( detail::next(free2)->identifier, AtomID::end );
    }
    catch ( ... )
    {
        FAIL();
    }
}

TEST( allocation, reallocation )
{
    try
    {
        auto contents_length = uint32_t{ 1024 };
        auto contents        = std::make_unique<uint8_t[]>(contents_length);
        auto data            = format( contents.get(), contents_length );

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        // • First reservation
        //
        auto alloc1 = detail::reserve(data, 34, AtomID::vector);

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        EXPECT_EQ( alloc1->length,  64 );
        EXPECT_EQ( detail::distance(data, alloc1), 16 );
        EXPECT_EQ( detail::contents_size(alloc1), 48 );

        // • Second reservation
        //
        auto alloc2 = detail::reserve(data, 512, AtomID::vector);

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        EXPECT_EQ( alloc2->length, 528 );
        EXPECT_EQ( detail::distance(data, alloc2), 80 );
        EXPECT_EQ( detail::contents_size(alloc2), 512 );

        // • Just for coverage, perform a same-size reallocation (no-op)
        //
        auto same_size = detail::reserve(data, alloc1, 42);

        EXPECT_EQ( same_size, alloc1 );
        EXPECT_EQ( detail::distance(data, same_size), 16 );
        EXPECT_EQ( detail::contents_size(same_size), 48 );

        // • First shrink the second
        //
        auto shrink2 = detail::reserve(data, alloc2, 480);

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        EXPECT_EQ( shrink2, alloc2 );
        EXPECT_EQ( detail::distance(data, shrink2), 80 );
        EXPECT_EQ( shrink2->length, atom_header_length + 480 );

        // • Reallocation of the second will extend it into the immediately following free space
        //
        auto realloc2 = detail::reserve(data, alloc2, 540);

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        EXPECT_EQ( realloc2, alloc2 );
        EXPECT_EQ( detail::distance(data, realloc2), 80 );
        EXPECT_EQ( realloc2->length, atom_header_length + 544 );
        EXPECT_EQ( alloc2->length, realloc2->length );

        // • Reallocation of the first will move it past the second
        //
        auto realloc1 = detail::reserve(data, alloc1, 120);

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        EXPECT_NE( realloc1, alloc1 );
        EXPECT_EQ( detail::distance(data, realloc1), 640 );
        EXPECT_EQ( detail::contents_size(realloc1), 128 );

        EXPECT_EQ( alloc1->identifier, AtomID::free );
    }
    catch ( ... )
    {
        FAIL();
    }
}

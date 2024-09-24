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

#include <Format/Allocation.hpp>

using namespace ::testing;
using namespace ::format;

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
        auto dataIt          = prepare_layout(contents.get(), 0, contents_length);

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        auto allocIt = detail::reserve(dataIt, 34);

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        EXPECT_EQ( allocIt->identifier, AtomID::allocation );
        EXPECT_EQ( allocIt.offset(), atom_header_length );
        EXPECT_EQ( allocIt.contents_size(), 48 ); // aligned_size(34) = 48
        EXPECT_EQ( std::next(dataIt), allocIt );

        auto alloc2It = detail::reserve(dataIt, 512);

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        EXPECT_EQ( alloc2It->identifier, AtomID::allocation );
        EXPECT_EQ( alloc2It->length, 528 );
        EXPECT_EQ( alloc2It.offset(), 80 );
        EXPECT_EQ( alloc2It.contents_size(), 512 );

        // • Test iterators
        //
        {
            auto testAlloc1It = std::next(dataIt);

            EXPECT_EQ( testAlloc1It->identifier, AtomID::allocation );
            EXPECT_EQ( testAlloc1It, allocIt );

            auto testAlloc2It = std::next(testAlloc1It);

            EXPECT_EQ( testAlloc2It->identifier, AtomID::allocation );
            EXPECT_EQ( testAlloc2It, alloc2It );

            auto freeIt = std::next(testAlloc2It);

            EXPECT_EQ( freeIt->identifier, AtomID::free );
            EXPECT_TRUE( std::next(freeIt).is_end() );
        }

        // • Deallocation
        //
        detail::free(allocIt);

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        // • Test iterators
        {
            auto free1It = std::next(dataIt);

            EXPECT_EQ( free1It->identifier, AtomID::free );
            EXPECT_EQ( free1It->length, 64 );
            EXPECT_EQ( free1It->previous, dataIt->length );

            auto alloc2It = std::next(free1It);

            EXPECT_EQ( alloc2It->identifier, AtomID::allocation );
            EXPECT_EQ( alloc2It->length, 528 );
            EXPECT_EQ( alloc2It->previous,   64 );

            auto free2It = std::next(alloc2It);

            EXPECT_EQ( free2It->identifier, AtomID::free );
            EXPECT_EQ( free2It->previous, 528 );
            EXPECT_TRUE( std::next(free2It).is_end() );
        }

        // • Deallocate second
        //
        detail::free(alloc2It);

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        // • Test iterators
        {
            auto coalescedFreeIt = std::next(dataIt);

            EXPECT_EQ( coalescedFreeIt->identifier, AtomID::free );
            EXPECT_EQ( coalescedFreeIt->length, contents_length - 2*atom_header_length );
            EXPECT_EQ( coalescedFreeIt->previous, dataIt->length );
            EXPECT_TRUE( std::next(coalescedFreeIt).is_end() );
        }
    }
    catch ( ... )
    {
        EXPECT_TRUE( false );
    }
}

TEST( allocation, reallocation )
{
    try
    {
        auto contents_length = uint32_t{ 1024 };
        auto contents        = std::make_unique<uint8_t[]>(contents_length);
        auto dataIt          = prepare_layout(contents.get(), 0, contents_length);

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        // • First reservation
        //
        auto allocIt = detail::reserve(dataIt, 34);

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        EXPECT_EQ( allocIt->length,  64 );
        EXPECT_EQ( allocIt.offset(), 16 );
        EXPECT_EQ( allocIt.contents_size(), 48 );

        // • Second reservation
        //
        auto alloc2It = detail::reserve(dataIt, 512);

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        EXPECT_EQ( alloc2It->length, 528 );
        EXPECT_EQ(alloc2It.offset(), 80 );
        EXPECT_EQ( alloc2It.contents_size(), 512 );

        // • Just for coverage, perform a same-size reallocation (no-op)
        //
        auto sameSizeIt = detail::reserve(dataIt, allocIt, 42);

        EXPECT_EQ( sameSizeIt, allocIt );
        EXPECT_EQ( sameSizeIt.offset(), 16 );
        EXPECT_EQ( sameSizeIt.contents_size(), 48 );

        // • First shrink the second
        //
        auto shrinkIt = detail::reserve(dataIt, alloc2It, 480);

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        EXPECT_EQ( shrinkIt, alloc2It );
        EXPECT_EQ( shrinkIt.offset(), 80 );
        EXPECT_EQ( shrinkIt->length, atom_header_length + 480 );

        // • Reallocation of the second will extend it into the immediately following free space
        //
        auto realloc2It = detail::reserve(dataIt, alloc2It, 540);

        EXPECT_EQ( realloc2It, alloc2It );
        EXPECT_EQ( realloc2It.offset(), 80 );
        EXPECT_EQ( alloc2It.offset(), 80 );
        EXPECT_EQ( realloc2It->length, atom_header_length + 544 );
        EXPECT_EQ( alloc2It->length, realloc2It->length );

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        // • Reallocation of the first will move it past the second
        //
        auto reallocIt = detail::reserve(dataIt, allocIt, 120);

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        EXPECT_NE( reallocIt, allocIt );
        EXPECT_EQ( reallocIt.offset(), 640 );
        EXPECT_EQ( reallocIt.contents_size(), 128 );
    }
    catch ( ... )
    {
        EXPECT_TRUE( false );
    }
}

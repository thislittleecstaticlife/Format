//
//  TextVector.cpp
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

#include <Data/Vector.hpp>

using namespace ::testing;
using namespace ::data;

//===------------------------------------------------------------------------===
//
// • Vector tests
//
//===------------------------------------------------------------------------===

TEST( vector, reservation )
{
    try
    {
        auto contents_length = uint32_t{ 1024 };
        auto contents        = std::make_unique<uint8_t[]>(contents_length);
        auto data            = data::format(contents.get(), contents_length);

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        auto ref    = VectorRef<int>{ 0 };
        auto vector = Vector<int>{ ref, data };

        EXPECT_EQ( vector.size(), 0 );
        EXPECT_TRUE( vector.empty() );
        EXPECT_EQ( ref.offset, 0 );
        EXPECT_EQ( ref.count, 0 );

        ASSERT_NO_THROW( vector.reserve(27) );

        auto expected_capacity = aligned_size<int>(27) / sizeof(int);

        EXPECT_EQ( vector.capacity(), expected_capacity );
        EXPECT_EQ( vector.available(), vector.capacity() );
        EXPECT_EQ( vector.size(), 0 );

        EXPECT_EQ( ref.offset, 2*atom_header_length );

        // • Reserving less than the current capacity is a no-op
        //
        ASSERT_NO_THROW( vector.reserve(1) );

        EXPECT_EQ( vector.capacity(), expected_capacity );
        EXPECT_EQ( vector.available(), vector.capacity() );
        EXPECT_EQ( vector.size(), 0 );

        EXPECT_EQ( ref.offset, 2*atom_header_length );
    }
    catch ( ... )
    {
        FAIL();
    }
}

TEST( vector, push_back )
{
    try
    {
        auto contents_length = uint32_t{ 1024 };
        auto contents        = std::make_unique<uint8_t[]>(contents_length);
        auto data            = data::format(contents.get(), contents_length);

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        auto ref    = VectorRef<int>{ };
        auto vector = Vector<int>{ ref, data };

        EXPECT_EQ( vector.size(), 0 );
        EXPECT_TRUE( vector.empty() );
        EXPECT_EQ( ref.offset, 0 );

        ASSERT_NO_THROW( vector.push_back(34) );

        EXPECT_EQ( vector.size(), 1 );
        EXPECT_EQ( vector[0], 34 );
        EXPECT_EQ( vector.at(0), 34 );
        EXPECT_EQ( vector.front(), 34 );
        EXPECT_EQ( vector.back(), 34 );

        for ( auto val : vector )
        {
            EXPECT_EQ( val, 34 );
        }

        auto eraseIt = vector.erase( vector.cbegin() );

        EXPECT_EQ( vector.size(), 0 );
        EXPECT_TRUE( vector.empty() );
        EXPECT_EQ( eraseIt, vector.cbegin() );
        EXPECT_EQ( eraseIt, vector.cend() );
    }
    catch ( ... )
    {
        FAIL();
    }
}

TEST( vector, assign )
{
    try
    {
        auto contents_length = uint32_t{ 1024 };
        auto contents        = std::make_unique<uint8_t[]>(contents_length);
        auto data            = data::format(contents.get(), contents_length);

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        auto ref    = VectorRef<int>{ };
        auto vector = Vector<int>{ ref, data };

        ASSERT_NO_THROW( vector.assign({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 }) );

        EXPECT_EQ( vector.size(), 17 );
        EXPECT_EQ( vector.capacity(), 20 );

        auto expected_value = 0;

        for ( auto val : vector )
        {
            EXPECT_EQ( val, expected_value++ );
        }

        vector.pop_back();

        EXPECT_EQ( vector.size(), 16 );
        EXPECT_EQ( vector.capacity(), 20 );

        expected_value = 0;

        for ( auto val : vector )
        {
            EXPECT_EQ( val, expected_value++ );
        }

        vector.erase( vector.cbegin() + 10 );

        EXPECT_EQ( vector.size(), 15 );
        EXPECT_EQ( vector[9], 9 );
        EXPECT_EQ( vector[10], 11 );

        vector.erase( vector.cbegin() + 5, vector.cbegin() + 12 );

        EXPECT_EQ( vector.size(), 8 );
        EXPECT_EQ( vector[4], 4 );
        EXPECT_EQ( vector[5], 13 );

        ASSERT_NO_THROW( vector.assign({ 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7 }) );

        EXPECT_EQ( vector.size(), 11 );

        expected_value = 17;

        for ( auto val : vector )
        {
            EXPECT_EQ( val, expected_value-- );
        }

        vector.assign({});

        EXPECT_TRUE( vector.empty() );
        EXPECT_EQ( vector.capacity(), 20 );
    }
    catch ( ... )
    {
        FAIL();
    }
}

TEST( vector, insert )
{
    try
    {
        auto contents_length = uint32_t{ 1024 };
        auto contents        = std::make_unique<uint8_t[]>(contents_length);
        auto data            = data::format(contents.get(), contents_length);

        EXPECT_TRUE( validate_layout(contents.get(), contents_length) );

        auto ref    = VectorRef<int>{ };
        auto vector = Vector<int>{ ref, data };

        ASSERT_NO_THROW( vector.assign({ 0, 1, 2, 3, 14, 15, 16 }) );

        EXPECT_EQ( vector.size(), 7 );
        EXPECT_EQ( vector.capacity(), 8 );

        auto insertIt = vector.end();

        EXPECT_NO_THROW( insertIt = vector.insert( vector.cbegin() + 4, { 4, 5, 6, 7, 8, 9, 10, 11, 12, 13 }) );

        EXPECT_EQ( insertIt, vector.begin() + 4 );
        EXPECT_EQ( vector.size(), 17 );
        EXPECT_EQ( vector.capacity(), 20 );

        auto expected_value = 0;

        for ( auto val : vector )
        {
            EXPECT_EQ( val, expected_value++ );
        }

        EXPECT_NO_THROW( insertIt = vector.insert(vector.cend(), 17) );

        EXPECT_EQ( insertIt, std::prev(vector.end()) );
        EXPECT_EQ( vector.size(), 18 );
        EXPECT_EQ( vector.capacity(), 20 );

        expected_value = 0;

        for ( auto val : vector )
        {
            EXPECT_EQ( val, expected_value++ );
        }

        EXPECT_NO_THROW( insertIt = vector.insert(vector.cbegin() + 3, {}) );

        EXPECT_EQ( insertIt, vector.begin() + 3 );
        EXPECT_EQ( vector.size(), 18 );
        EXPECT_EQ( vector.capacity(), 20 );

        expected_value = 0;

        for ( auto val : vector )
        {
            EXPECT_EQ( val, expected_value++ );
        }

        EXPECT_NO_THROW( insertIt = vector.insert(vector.cend(), 0, 18) );

        EXPECT_EQ( insertIt, vector.end() );
        EXPECT_EQ( vector.size(), 18 );
        EXPECT_EQ( vector.capacity(), 20 );

        expected_value = 0;

        for ( auto val : vector )
        {
            EXPECT_EQ( val, expected_value++ );
        }

        // • Test that erasing the end is a no-op
        //
        vector.erase( vector.cend() );

        EXPECT_EQ( vector.size(), 18 );
        EXPECT_EQ( vector.capacity(), 20 );

        expected_value = 0;

        for ( auto val : vector )
        {
            EXPECT_EQ( val, expected_value++ );
        }
    }
    catch ( ... )
    {
        FAIL();
    }
}

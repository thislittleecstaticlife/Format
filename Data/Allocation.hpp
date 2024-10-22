//
//  Allocation.hpp
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

#include <Data/Atom.hpp>

//===------------------------------------------------------------------------===
// • namespace data
//===------------------------------------------------------------------------===

namespace data
{

//===------------------------------------------------------------------------===
// • Allocation primitives
//===------------------------------------------------------------------------===

namespace detail
{

Atom* reserve(Atom* data, uint32_t requested_contents_size, AtomID identifier) noexcept(false);
Atom* reserve(Atom* data, Atom* curr_alloc, uint32_t requested_contents_size) noexcept(false);

Atom* free(Atom* dealloc) noexcept;

} // namespace detail

} // namespace data

//
//  Layout-Host.hpp
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

//===------------------------------------------------------------------------===
// • namespace data
//===------------------------------------------------------------------------===

namespace data
{

//===------------------------------------------------------------------------===
// • Concept names (Metal)
//===------------------------------------------------------------------------===

#define TRIVIAL_LAYOUT typename

//===------------------------------------------------------------------------===
// • Memory Layout Utilities (Metal)
//===------------------------------------------------------------------------===

template <TRIVIAL_LAYOUT Type_, TRIVIAL_LAYOUT Root_>
constant Type_* offset_by(constant Root_* root, uint32_t offset)
{
    return reinterpret_cast<constant Type_*>(reinterpret_cast<constant uint8_t*>(root) + offset);
}

template <TRIVIAL_LAYOUT Type_, TRIVIAL_LAYOUT Root_>
const device Type_* offset_by(const device Root_* root, uint32_t offset)
{
    return reinterpret_cast<const device Type_*>(reinterpret_cast<const device uint8_t*>(root) + offset);
}

template <TRIVIAL_LAYOUT Type_, TRIVIAL_LAYOUT Root_>
device Type_* offset_by(device Root_* root, uint32_t offset)
{
    return reinterpret_cast<device Type_*>(reinterpret_cast<device uint8_t*>(root) + offset);
}

} // namespace data

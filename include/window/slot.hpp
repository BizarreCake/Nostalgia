/*
 * Nostalgia - A custom Minecraft server.
 * Copyright (C) 2019  Jacob Zhitomirsky
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef NOSTALGIA_SLOT_HPP
#define NOSTALGIA_SLOT_HPP


/*!
 * \class slot
 * \brief Represents a window slot which stores an item and its associated data.
 */
class slot
{
  int item_id;
  unsigned char item_count;

 public:
  [[nodiscard]] inline auto id () const { return this->item_id; }
  [[nodiscard]] inline auto count () const { return this->item_count; }

  inline void set_id (int val) { this->item_id = val; }
  inline void set_count (unsigned char count) { this->item_count = count; }

  explicit slot (int item_id, unsigned char item_count = 1)
    : item_id (item_id), item_count (item_count)
  { }
};

#endif //NOSTALGIA_SLOT_HPP

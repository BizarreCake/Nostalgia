//
// Created by Jacob Zhitomirsky on 05-Jul-19.
//

#include "util/position.hpp"


chunk_pos::chunk_pos (const block_pos& bpos)
  : x (bpos.x / 16), z (bpos.z / 16)
{
  // nop
}

chunk_pos::chunk_pos (const player_pos& pos)
  : x ((int)(pos.x / 16.0)), z ((int)(pos.z / 16.0))
{
  // nop
}


block_pos::block_pos (const player_pos& pos)
  : x ((int)pos.x), y ((int)pos.y), z ((int)pos.z)
{
  // nop
}

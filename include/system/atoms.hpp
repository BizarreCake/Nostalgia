//
// Created by Jacob Zhitomirsky on 09-Sep-19.
//

#ifndef NOSTALGIA_ATOMS_HPP
#define NOSTALGIA_ATOMS_HPP

#include <caf/all.hpp>


// general atoms:
using stop_atom = caf::atom_constant<caf::atom ("0_1")>;
using init_atom = caf::atom_constant<caf::atom ("0_2")>;

// client atoms:
using broker_atom = caf::atom_constant<caf::atom ("1_1")>;
using packet_in_atom = caf::atom_constant<caf::atom ("1_2")>;
using packet_out_atom = caf::atom_constant<caf::atom ("1_3")>;
using message_atom = caf::atom_constant<caf::atom ("1_4")>;
using event_complete_atom = caf::atom_constant<caf::atom ("1_5")>;

// scripting engine atoms:
using run_command_atom = caf::atom_constant<caf::atom ("2_1")>;
using load_commands_atom = caf::atom_constant<caf::atom ("2_2")>;
using run_script_basic_atom = caf::atom_constant<caf::atom ("2_3")>;
using register_player_atom = caf::atom_constant<caf::atom ("2_4")>;
using unregister_player_atom = caf::atom_constant<caf::atom ("2_5")>;
using register_world_atom = caf::atom_constant<caf::atom ("2_6")>;
using unregister_world_atom = caf::atom_constant<caf::atom ("2_7")>;

// server atoms:
using add_client_atom = caf::atom_constant<caf::atom ("3_1")>;
using del_client_atom = caf::atom_constant<caf::atom ("3_2")>;
using get_client_atom = caf::atom_constant<caf::atom ("3_3")>;
using set_client_atom = caf::atom_constant<caf::atom ("3_4")>;
using get_world_atom = caf::atom_constant<caf::atom ("3_5")>;
using broadcast_packet_atom = caf::atom_constant<caf::atom ("3_6")>;
using global_message_atom = caf::atom_constant<caf::atom ("3_7")>;

// world generator atoms:
using generate_atom = caf::atom_constant<caf::atom ("4_1")>;

// world atoms:
using request_chunk_data_atom = caf::atom_constant<caf::atom ("5_1")>;
using set_block_atom = caf::atom_constant<caf::atom ("5_2")>;

// scripting request/response atoms:
using s_get_pos_atom = caf::atom_constant<caf::atom ("S_1")>;
using s_get_world_atom = caf::atom_constant<caf::atom ("S_2")>;

// scripting event triggers:
using s_evt_player_chat_atom = caf::atom_constant<caf::atom ("SE_1")>;
using s_evt_player_change_block_atom = caf::atom_constant<caf::atom ("SE_2")>;

#endif //NOSTALGIA_ATOMS_HPP

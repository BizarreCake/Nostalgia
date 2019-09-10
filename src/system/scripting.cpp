//
// Created by Jacob Zhitomirsky on 07-Sep-19.
//

#include "system/scripting.hpp"
#include "network/packets.hpp"
#include "util/position.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <lua.hpp>

using namespace std::chrono_literals;

static unsigned int player_table_magic = 0x13370001;

enum yield_code
{
  YC_NONE,
  YC_GET_POSITION,
};


//
// Exception types:
//

class command_not_found : public std::exception {};

class lua_exception : public std::runtime_error
{
 public:
  lua_exception (const std::string& str)
    : std::runtime_error (str)
  {}
};


//----

static bool
_is_player_object (lua_State *L, int index)
{
  if (!lua_istable (L, index))
    return false;

  lua_pushstring (L, "magic");
  lua_gettable (L, index - 1);
  auto value = (unsigned int)lua_tointeger (L,-1);
  lua_pop (L, 1);

  return value == player_table_magic;
}

static int
player_message (lua_State *L)
{
  if (lua_gettop (L) != 2)
    {
      // TODO: invalid argument count
      return 0;
    }

  // verify arguments
  if (!_is_player_object (L, -2) || !lua_isstring (L, -1))
    {
      // TODO: invalid arguments
      return 0;
    }

  const char *msg = lua_tostring (L, -1);

  // extract script state
  lua_pushvalue (L, -2); // push player table to top of stack
  lua_pushstring (L, "ctx");
  lua_gettable (L, -2); // get player.ctx
  auto& state = **static_cast<script_state **> (lua_touserdata (L, -1));
  lua_pop (L, 1); // pop ctx

  // send message packet to player
  auto packet = packets::play::make_chat_message_simple (msg, 0);
  state.self->send (state.client, packet_out_atom::value, packet.move_data ());

  return 0;
}

static int
player_get_position_cont (lua_State *L, int status, lua_KContext kctx)
{
  // the player's position is now at the top of the stack
  // just return it
  return 1;
}

static int
player_get_position (lua_State *L)
{
  if (lua_gettop (L) != 1)
    {
      // TODO: invalid argument count
      return 0;
    }

  // verify arguments
  if (!_is_player_object (L, -1))
    {
      // TODO: invalid arguments
      return 0;
    }

  lua_pushinteger (L, YC_GET_POSITION);
  lua_yieldk (L, 1, NULL, player_get_position_cont);
  return 0;
}


//----

scripting_actor::scripting_actor (caf::actor_config& cfg)
  : caf::blocking_actor (cfg)
{
  lua_State *L = luaL_newstate ();
  luaL_openlibs (L);
  this->vm = L;
}

scripting_actor::~scripting_actor ()
{
  auto L = static_cast<lua_State *> (this->vm);
  lua_close (L);
}


//! \brief Searches for the script state whose ID matches the specified argument.
script_state*
scripting_actor::find_state (int sid)
{
  // TODO: consider using an associative array for this or something
  auto itr = std::find_if (this->states.begin (), this->states.end (),
      [sid] (const script_state& state) { return state.id == sid; });
  if (itr == this->states.end ())
    return nullptr;
  return &*itr;
}


void
scripting_actor::act ()
{
  bool running = true;
  this->receive_while (running) (
    [&] (stop_atom) {
      running = false;
    },

    [&] (run_command_atom, const std::string& cmd, const std::string& msg, const caf::actor& client) {
      this->handle_runcmd (cmd, msg, client);
    },

    [&] (load_commands_atom , const std::string& path) {
      this->load_commands (path);
    },

    // response messages:

    [&] (s_get_pos_atom, int sid, player_pos pos, player_rot rot) {
      auto state = this->find_state (sid);
      if (!state) return;
      if (state->last_yield_code != YC_GET_POSITION) return; // shouldn't happen...

      // push result
      auto L = static_cast<lua_State *> (state->thread);
      lua_createtable (L, 0, 5);
      lua_pushstring (L, "x");
      lua_pushnumber (L, pos.x);
      lua_settable (L, -3);
      lua_pushstring (L, "y");
      lua_pushnumber (L, pos.y);
      lua_settable (L, -3);
      lua_pushstring (L, "z");
      lua_pushnumber (L, pos.z);
      lua_settable (L, -3);
      lua_pushstring (L, "yaw");
      lua_pushnumber (L, rot.yaw);
      lua_settable (L, -3);
      lua_pushstring (L, "pitch");
      lua_pushnumber (L, rot.pitch);
      lua_settable (L, -3);

      this->resume_script (*state, 1);
    },

    // timeout:
    caf::after (5ms) >> [&] () {
      this->handle_timeout ();
    }
  );
}

void
scripting_actor::handle_runcmd (const std::string& cmd, const std::string& msg, const caf::actor& client)
{
  auto L = static_cast<lua_State *> (this->vm);

  // create new state
  this->states.emplace_back ();
  auto& state = this->states.back ();
  state.vm = L;
  state.thread = lua_newthread (L);
  state.type = script_type::command;
  state.id = this->next_state_id ++;
  state.self = this;
  state.client = client;
  state.last_yield_code = YC_NONE;
  state.itr = this->states.end ();
  -- state.itr;

  try
    {
      this->run_command (state);
    }
  catch (const lua_exception& ex)
    {
      // TODO:
      caf::aout (this) << "Lua runtime error: " << ex.what () << std::endl;
      this->delete_state (state);
      return;
    }
}

void
scripting_actor::handle_timeout ()
{

}



void
scripting_actor::delete_state (script_state& state)
{
  this->states.erase (state.itr);
}


void
scripting_actor::load_command (const std::string& cmd_path)
{
  // open script file
  // TODO: sanitize command!
  // TODO: or alternatively, implement a trusted command list and use that.
  std::ifstream fs (cmd_path);
  if (!fs)
    throw command_not_found {};

  // read contents into string
  std::stringstream ss;
  ss << fs.rdbuf ();
  auto script = ss.str ();

  // load script in context of main VM
  auto L = static_cast<lua_State *> (this->vm);
  int err = luaL_loadstring (L, script.c_str ()) || lua_pcall (L, 0, 0, 0);
  if (err)
    {
      std::string err_msg = lua_tostring (L, -1);
      this->states.pop_back ();
      throw lua_exception (err_msg);
    }
}

void
scripting_actor::run_command (script_state& state)
{
  auto L = static_cast<lua_State *> (state.thread);
  if (!lua_getglobal (L, "do_command"))
    throw lua_exception ("Command script does not have a \"do_command\" function!");

  // create player table
  lua_createtable (L, 0, 0);

  // player.magic
  lua_pushstring (L, "magic");
  lua_pushinteger (L, player_table_magic);
  lua_settable (L, -3);

  // player.ctx (pointer to script_state)
  lua_pushstring (L, "ctx");
  auto ctx = static_cast<script_state **> (lua_newuserdata (L, sizeof (script_state *)));
  *ctx = &state;
  lua_settable (L, -3);

  // player.message
  lua_pushstring (L, "message");
  lua_pushcfunction (L, player_message);
  lua_settable (L, -3);

  // player.message
  lua_pushstring (L, "get_position");
  lua_pushcfunction (L, player_get_position);
  lua_settable (L, -3);

  this->resume_script (state, 1);
}

void
scripting_actor::load_commands (const std::string& path)
{
  caf::aout (this) << "Loading commands..." << std::endl;
  for (auto& entry : std::filesystem::directory_iterator (path))
    {
      auto& p = entry.path ();
      if (!entry.is_regular_file ())
        continue;
      if (p.extension () != ".lua")
        continue;

      try
        {
          caf::aout (this) << "    Loading command: " << p.filename ().string () << std::endl;
          this->load_command (p.string ());
        }
//      catch (const command_not_found&)
//        {
//          caf::aout (this) << "Command not found" << std::endl;
//        }
      catch (const lua_exception& ex)
        {
          // TODO:
          caf::aout (this) << "Lua error during load: " << ex.what () << std::endl;
        }
    }

  caf::aout (this) << "    Done" << std::endl;
}


void
scripting_actor::process_yield (script_state& state)
{
  auto L = static_cast<lua_State *> (state.thread);
  auto params = lua_gettop (L);
  if (params == 0)
    {
      this->delete_state (state);
      return;
    }

  auto yield_code = lua_tointeger (L, -params);
  state.last_yield_code = (int)yield_code;
  switch (yield_code)
    {
    case YC_GET_POSITION:
      this->send (state.client, s_get_pos_atom::value, state.id);
      break;

    default: ;
    }

  // clear stack
  lua_pop (L, params);
}

void
scripting_actor::resume_script (script_state& state, int num_args)
{
  auto L = static_cast<lua_State *> (state.thread);

  int status = lua_resume (L, NULL, num_args);
  if (status == LUA_OK)
    {
      // finished executing command
      this->delete_state (state);
    }
  else if (status == LUA_YIELD)
    {
      // script yielded
      this->process_yield (state);
    }
  else
    {
      // TODO: error
      throw lua_exception ("Error");
    }
}

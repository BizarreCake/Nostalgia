//
// Created by Jacob Zhitomirsky on 09-May-19.
//

#include "system/server.hpp"
#include "system/atoms.hpp"
#include "world/world.hpp"
#include "system/consts.hpp"
#include "world/generator_actor.hpp"
#include "network/packets.hpp"
#include "world/blocks.hpp"
#include "system/registries.hpp"
#include "system/scripting.hpp"


server_actor::server_actor (caf::actor_config& cfg, const caf::actor& script_eng)
    : caf::event_based_actor (cfg), script_eng (script_eng)
{
  // nop
}


void
server_actor::setup ()
{
  // load registries
  registries::initialize ();

  // load blocks
  block::initialize ();

  // load commands
  this->send (this->script_eng, load_commands_atom::value, "scripts/commands");

  // spawn world generator actor
  this->world_gen = this->system ().spawn<world_generator_actor> ();

  // spawn main world
  auto main_world = this->system ().spawn<world> (main_world_name, this->world_gen);
  world_info info = { main_world, main_world_name };
  this->worlds[main_world_name] = info;
}


caf::behavior
server_actor::make_behavior ()
{
  return {
      [this] (init_atom) {
        this->setup ();
        return true;
      },

      [this] (add_client_atom, caf::actor client, unsigned int client_id) {
        this->connected_clients[client_id] = client_info ();
        client_info& info = this->connected_clients[client_id];
        info.actor = client;
        info.id = client_id;
        info.uuid = uuid_t::random (this->rnd);
      },

      [this] (del_client_atom, unsigned int client_id) {
        this->connected_clients.erase (client_id);
      },

      [this] (get_client_atom, unsigned int client_id) {
        auto itr = this->connected_clients.find (client_id);
        if (itr == this->connected_clients.end ())
          return client_info {};
        return itr->second;
      },

      [this] (set_client_atom, unsigned int client_id,
              const client_info& info) {
        auto itr = this->connected_clients.find (client_id);
        if (itr != this->connected_clients.end ())
          {
            itr->second = info;
          }
      },

      [this] (get_world_atom, const std::string& world_name) {
        auto itr = this->worlds.find (world_name);
        if (itr == this->worlds.end ())
          return world_info {};
        return itr->second;
      },

      [this] (global_message_atom, const std::string& msg) {
        auto packet = packets::play::make_chat_message_simple (msg, 0);
        std::vector<char> packet_buf = packet.move_data ();
        for (auto& p : this->connected_clients)
          {
            auto& cl = p.second.actor;
            this->send (cl, packet_out_atom::value, packet_buf);
          }
      }
  };
}


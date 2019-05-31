//
// Created by Jacob Zhitomirsky on 09-May-19.
//

#include "server.hpp"
#include "world.hpp"
#include "consts.hpp"


server::server (caf::actor_config& cfg)
    : caf::event_based_actor (cfg)
{
  // nop
}


void
server::setup ()
{
  // spawn main world
  auto main_world = this->system ().spawn<world> (main_world_name);
  world_info info = { main_world, main_world_name };
  this->worlds[main_world_name] = info;
}


caf::behavior
server::make_behavior ()
{
  return {
      [this] (caf::atom_constant<caf::atom ("init")>) {
        this->setup ();
        return true;
      },

      [this] (caf::atom_constant<caf::atom ("addclient")>, const caf::actor& client,
              unsigned int client_id) {
        this->connected_clients[client_id] = client_info ();
        client_info& info = this->connected_clients[client_id];
        info.actor = client;
        info.id = client_id;
        info.uuid = uuid_t::random (this->rnd);
      },

      [this] (caf::atom_constant<caf::atom ("delclient")>, unsigned int client_id) {
        this->connected_clients.erase (client_id);
      },

      [this] (caf::atom_constant<caf::atom ("getclient")>, unsigned int client_id) {
        auto itr = this->connected_clients.find (client_id);
        if (itr == this->connected_clients.end ())
          return client_info {};
        return itr->second;
      },

      [this] (caf::atom_constant<caf::atom ("setclient")>, unsigned int client_id,
              const client_info& info) {
        auto itr = this->connected_clients.find (client_id);
        if (itr != this->connected_clients.end ())
          {
            itr->second = info;
          }
      },

      [this] (caf::atom_constant<caf::atom ("getworld")>, const std::string& world_name) {
        auto itr = this->worlds.find (world_name);
        if (itr == this->worlds.end ())
          return world_info {};
        return itr->second;
      }
  };
}

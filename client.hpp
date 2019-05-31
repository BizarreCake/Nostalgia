//
// Created by Jacob Zhitomirsky on 08-May-19.
//

#ifndef NOSTALGIA_CLIENT_HPP
#define NOSTALGIA_CLIENT_HPP

#include "consts.hpp"
#include "info.hpp"
#include "caf/all.hpp"

// forward decs:
class packet_reader;
class packet_writer;


/*!
 * \brief Thrown by functions called by handle_packet to force the client
 *        to be disconnected with a message.
 */
class disconnect : public std::exception
{
  std::string msg;

 public:
  explicit disconnect (const std::string& msg)
    : msg (msg)
  {}

  inline const std::string& message () const { return this->msg; }
};


class client : public caf::event_based_actor
{
  caf::actor srv;
  connection_state curr_state;
  caf::actor broker;
  client_info info;

  world_info curr_world;

 public:
  explicit client (caf::actor_config& cfg, const caf::actor& srv,
                         unsigned int client_id);

  caf::behavior make_behavior () override;

 private:
  void handle_packet (packet_reader& reader);

  void handle_handshake_state_packet (packet_reader& reader);
  void handle_play_state_packet (packet_reader& reader);
  void handle_status_state_packet (packet_reader& reader);
  void handle_login_state_packet (packet_reader& reader);

  void handle_handshake_packet (packet_reader& reader);

  void handle_login_start_packet (packet_reader& reader);


  /*!
   * \brief Sends the contents of the specified packet writer.
   * \param writer The packet writer that contains the packet to send.
   *
   * NOTE: This invalidates the packet writer as it moves its buffer.
   */
  void send_packet (packet_writer& writer);
  void send_packet (packet_writer&& writer);

  void join_world (const std::string& world_name);
};

#endif //NOSTALGIA_CLIENT_HPP
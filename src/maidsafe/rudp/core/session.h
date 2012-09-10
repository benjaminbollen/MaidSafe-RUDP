/*******************************************************************************
 *  Copyright 2012 MaidSafe.net limited                                        *
 *                                                                             *
 *  The following source code is property of MaidSafe.net limited and is not   *
 *  meant for external use.  The use of this code is governed by the licence   *
 *  file licence.txt found in the root of this directory and also on           *
 *  www.maidsafe.net.                                                          *
 *                                                                             *
 *  You are not free to copy, amend or otherwise use this source code without  *
 *  the explicit written permission of the board of directors of MaidSafe.net. *
 ******************************************************************************/
// Original author: Christopher M. Kohlhoff (chris at kohlhoff dot com)

#ifndef MAIDSAFE_RUDP_CORE_SESSION_H_
#define MAIDSAFE_RUDP_CORE_SESSION_H_

#include <mutex>
#include <cstdint>
#include "boost/date_time/posix_time/posix_time_types.hpp"
#include "boost/asio/ip/udp.hpp"
#include "boost/signals2/connection.hpp"
#include "boost/signals2/signal.hpp"
#include "maidsafe/common/rsa.h"
#include "maidsafe/rudp/packets/handshake_packet.h"
#include "maidsafe/rudp/nat_type.h"


namespace maidsafe {

namespace rudp {

namespace detail {

class Peer;
class TickTimer;

class Session {
 public:
  typedef boost::signals2::signal<void(const boost::asio::ip::udp::endpoint&,
                                       const boost::asio::ip::udp::endpoint&,
                                       uint16_t&)> OnNatDetectionRequested;

  enum Mode { kNormal, kBootstrapAndDrop, kBootstrapAndKeep };

  explicit Session(Peer& peer,                                                    // NOLINT (Fraser)
                   TickTimer& tick_timer,
                   boost::asio::ip::udp::endpoint& this_external_endpoint,
                   std::mutex& this_external_endpoint_mutex,
                   const boost::asio::ip::udp::endpoint& this_local_endpoint,
                   NatType& nat_type);

  // Open the session.
  void Open(uint32_t id,
            NodeId this_node_id,
            std::shared_ptr<asymm::PublicKey> this_public_key,
            uint32_t sequence_number,
            Mode mode,
            const OnNatDetectionRequested::slot_type& on_nat_detection_requested_slot);

  // Get whether the session is already open. May not be connected.
  bool IsOpen() const;

  // Get whether the session is currently connected to the peer.
  bool IsConnected() const;

  // Get the id assigned to the session.
  uint32_t Id() const;

  // Get the first sequence number for packets received.
  uint32_t ReceivingSequenceNumber() const;

  // Get the peer connection type.
  uint32_t PeerConnectionType() const;

  // Close the session. Clears the id.
  void Close();

  // Handle a handshake packet.
  void HandleHandshake(const HandshakePacket& packet);

  // Handle a tick in the system time.
  void HandleTick();

  // Changes mode_ to kNormal;
  void MakePermanent();

  Mode mode() const;

  boost::asio::ip::udp::endpoint RemoteNatDetectionEndpoint() const;

 private:
  // Disallow copying and assignment.
  Session(const Session&);
  Session& operator=(const Session&);

  bool CalculateEndpoint();

  // Helper functions to send the packets that make up the handshaking process.
  void SendPacket();
  void SendConnectionRequest();
  void SendCookie();

  // The peer with which we are communicating.
  Peer& peer_;

  // The timer used to generate tick events.
  TickTimer& tick_timer_;

  // This node's external endpoint as viewed by peer.  Object owned by multiplexer.
  boost::asio::ip::udp::endpoint& this_external_endpoint_;

  // Mutex protecting this_external_endpoint_.  Object owned by multiplexer.
  std::mutex &this_external_endpoint_mutex_;

  // This node's local endpoint.
  const boost::asio::ip::udp::endpoint kThisLocalEndpoint_;

  // This node's NAT type.  Object owned by ManagedConnections.
  NatType& nat_type_;

  // This node's NodeId
  NodeId this_node_id_;

  // This node's public key
  std::shared_ptr<asymm::PublicKey> this_public_key_;

  // The local socket id.
  uint32_t id_;

  // The initial sequence number for packets sent in this session.
  uint32_t sending_sequence_number_;

  // The initial sequence number for packets received in this session.
  uint32_t receiving_sequence_number_;

  // The peer's connection type.
  uint32_t peer_connection_type_;

  // Whether the peer requested another port to do NAT detection.
  bool peer_requested_nat_detection_port_;

  // Endpoint offered by peer for this node to perform NAT detection
  boost::asio::ip::udp::endpoint peer_nat_detection_endpoint_;

  // The open mode of the session.
  Mode mode_;

  // The state of the session.
  enum State { kClosed, kProbing, kHandshaking, kConnected } state_;

  OnNatDetectionRequested on_nat_detection_requested_;
  boost::signals2::connection signal_connection_;
};

}  // namespace detail

}  // namespace rudp

}  // namespace maidsafe

#endif  // MAIDSAFE_RUDP_CORE_SESSION_H_

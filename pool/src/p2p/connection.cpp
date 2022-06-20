/**
 * Copyright (c) 2021 braidpool developers (see AUTHORS)
 *
 * This file is part of braidpool.
 *
 * braidpool is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * braidpool is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with braidpool.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "p2p/connection.hpp"

#include <boost/asio/awaitable.hpp>
#include <boost/asio/detached.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <p2p/define.hpp>

#include "system.hpp"

using boost::asio::buffer;
using boost::asio::detached;
using boost::asio::use_awaitable;

namespace bp {
namespace p2p {

connection::connection(tcp::socket&& sock) : socket_(std::move(sock)) {
  start_receive_from_peer();
}

connection::~connection() {
  std::cerr << "Connection destroyed..." << std::endl;
  if (socket_.is_open()) {
    socket_.close();
  }
}

awaitable<void> connection::send_to_peer(std::string message) {
  try {
    std::cerr << "Sending: " << message << std::endl;
    co_await async_write(socket_, buffer(message), use_awaitable);
  } catch (const std::exception& e) {
    std::cerr << "failing to send to peer" << std::endl;
    std::cerr << e.what() << std::endl;
    socket_.close();
  }
}

void connection::start_receive_from_peer() {
  std::cerr << "calling receive..." << std::endl;
  co_spawn(socket_.get_executor(), receive_from_peer(), detached);
}

awaitable<void> connection::receive_from_peer() {
  try {
    for (std::string read_msg;;) {
      auto num_bytes_read = co_await boost::asio::async_read_until(
          socket_, boost::asio::dynamic_buffer(read_msg, 1024), "\r\n",
          use_awaitable);
      std::cerr << "Received: " << read_msg << std::endl;

      if (read_msg == "ping\r\n") {
        co_await send_to_peer("pong\r\n");
      }

      read_msg.erase(0, num_bytes_read);
    }
  } catch (const std::exception& e) {
    std::cerr << "failing to receive from peer" << std::endl;
    std::cerr << e.what() << std::endl;
    socket_.close();
  }
}

}  // namespace p2p
}  // namespace bp
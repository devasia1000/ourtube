#ifndef HTTP_HANDLER_HH
#define HTTP_HANDLER_HH

#include <fstream>
#include <utility>
#include "socket.hh"
#include "http_parser.hh"

class HTTPHandler
{
private:
  int pid_;
  std::fstream requests_fd_;
  std::fstream response_fd_;

  Socket client_socket_;
  int signal_fd_;

  void read_request( void );
  void connect_to_server( void );
  void two_way_connection( void );

  HTTPHeaderParser request_parser_;

  bool client_eof_, server_eof_;
  std::string pending_client_to_server_;

  Socket server_socket_;

  size_t client_body_so_far_;

public:
  HTTPHandler( const Socket & s_socket, const int s_signal_fd )
  : pid_(getpid()),
    requests_fd_(std::to_string(pid_)+std::string(".req"), std::ios::binary|std::ios::out),
    response_fd_(std::to_string(pid_)+std::string(".res"), std::ios::binary|std::ios::out),
    client_socket_( s_socket ),
    signal_fd_( s_signal_fd ),
    request_parser_(),
    client_eof_( false ), server_eof_( false ),
    pending_client_to_server_(),
    server_socket_(),
    client_body_so_far_( 0 )
  {}

  void handle_request( void );
};

#endif /* HTTP_HANDLER_HH */

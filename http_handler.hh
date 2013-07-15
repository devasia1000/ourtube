#ifndef HTTP_HANDLER_HH
#define HTTP_HANDLER_HH

#include "socket.hh"
#include "http_parser.hh"

class HTTPHandler
{
private:
  Socket client_socket_;
  int signal_fd_;

  void read_request_up_to_host_header( void );
  void connect_to_server( void );
  void two_way_connection( void );

  HTTPRequestParser parser_;

  bool client_eof_, server_eof_;
  std::string pending_client_to_server_;

  Socket server_socket_;

public:
  HTTPHandler( const Socket & s_socket, const int s_signal_fd )
  : client_socket_( s_socket ),
    signal_fd_( s_signal_fd ),
    parser_(),
    client_eof_( false ), server_eof_( false ),
    pending_client_to_server_(),
    server_socket_()
  {}

  void handle_request( void );
};

#endif /* HTTP_HANDLER_HH */

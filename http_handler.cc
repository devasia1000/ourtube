#include <poll.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <assert.h>

#include "exception.hh"
#include "http_handler.hh"

using namespace std;

void HTTPHandler::handle_request( void )
{
  try {
    /* parse up to the host header */
    read_request();

    if ( !request_parser_.has_header( "Host" ) ) {
      /* there was no host header */
      if ( !pending_client_to_server_.empty() ) {
	throw Exception( "HTTPHandler request is missing Host header", pending_client_to_server_ );
      } else {
	/* quit quietly */
	return;
      }
    }

    /* open connection to server */
    connect_to_server();

    assert( request_parser_.headers_parsed() );

    /* write pending data from client to server */
    server_socket_.write( pending_client_to_server_ );

    /* ferry data in both directions until EOF from either */
    two_way_connection();
  } catch ( const Exception &e ) {
    e.die();
  }
}

void HTTPHandler::read_request( void )
{
  while ( 1 ) {
    string buffer = client_socket_.read();
    requests_fd_.write(buffer.c_str(), buffer.size());
    requests_fd_.flush();

    if ( buffer.empty() ) { client_eof_ = true; return; }

    /* save this data for later replay to server */
    pending_client_to_server_ += buffer;
    if ( request_parser_.parse( buffer ) ) {
      /* found it */
      cout<<"Got initial request: "<<request_parser_.request_line()<<"\n";
      return;
    }
  }
}

void HTTPHandler::connect_to_server( void )
{
  /* find host and service */
  string header_value = request_parser_.get_header_value( "Host" );

  /* split value into host and (possible) port */
  size_t colon_location = header_value.find( ":" );

  string host, service;
  if ( colon_location == std::string::npos ) {
    host = header_value;
    service = "http";
  } else {
    host = header_value.substr( 0, colon_location );
    service = header_value.substr( colon_location + 1 );
  }

  Address server_addr( host, service );
  
  fprintf( stderr, "Opening connection to host %s @ %s\n",
	   host.c_str(), server_addr.str().c_str() );
 

  server_socket_.connect( server_addr );

  //  fprintf( stderr, "done.\n" );
}

void HTTPHandler::two_way_connection( void )
{
  struct pollfd pollfds[ 3 ];
  pollfds[ 0 ].fd = client_socket_.raw_fd();
  pollfds[ 1 ].fd = server_socket_.raw_fd();
  pollfds[ 2 ].fd = signal_fd_;

  /* Run until either client or server closes the underlying
     TCP connection running below the HTTP transactions */
  while ( 1 ) {
    if ( client_eof_ || server_eof_ ) {
      break;
    }

    pollfds[ 0 ].events = client_eof_ ? 0 : POLLIN;
    pollfds[ 1 ].events = server_eof_ ? 0 : POLLIN;
    pollfds[ 2 ].events = POLLIN;

    /* wait for data in either direction */
    if ( poll( pollfds, 3, -1 ) <= 0 ) {
      throw Exception( "poll" );
    }

    if ( pollfds[ 0 ].revents & POLLIN ) {
      /* data available from client */
      /* send to server */
      string buffer = client_socket_.read();
      requests_fd_.write(buffer.c_str(), buffer.size());
      requests_fd_.flush();

      if ( buffer.empty() ) {
	client_eof_ = true;
      } else {
	server_socket_.write( buffer );
      }

      /* parse body or header as appropriate */
      if ( request_parser_.parse( buffer ) ) {
        cout<<"Got continuation request: "<<request_parser_.request_line()<<"\n";
      }
    }

    if ( pollfds[ 1 ].revents & POLLIN ) {
      /* data available from server */
      /* send to client */
      string buffer = server_socket_.read();
      response_fd_.write(buffer.c_str(), buffer.size());
      response_fd_.flush();
      if ( buffer.empty() ) {
	server_eof_ = true;
      } else {
	client_socket_.write( buffer );
      }
    }

    if ( (pollfds[ 0 ].revents & POLLERR)
	 || (pollfds[ 1 ].revents & POLLERR)
	 || (pollfds[ 2 ].revents & POLLERR) ) {
      throw Exception( "poll error" );
    }

    if ( pollfds[ 2 ].revents & POLLIN ) {
      signalfd_siginfo delivered_signal;

      if ( read( signal_fd_, &delivered_signal, sizeof( signalfd_siginfo ) )
	   != sizeof( signalfd_siginfo ) ) {
	throw Exception( "read size mismatch" );
      }

      if ( delivered_signal.ssi_signo == SIGPIPE ) {
	return;
      }
    }
  }
}

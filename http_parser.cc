#include <string>
#include <assert.h>

#include "http_parser.hh"
#include "exception.hh"
#include "ezio.hh"

using namespace std;

/* parse a header line into a key and a value */
HTTPHeader::HTTPHeader( const string & buf )
  : key_(), value_()
{
  const string separator = ":";

  /* step 1: does buffer contain colon? */
  size_t colon_location = buf.find( separator );
  if ( colon_location == std::string::npos ) {
    fprintf( stderr, "Buffer: %s\n", buf.c_str() );
    throw Exception( "HTTPHeader", "buffer does not contain colon" ); 
  }

  /* step 2: split buffer */
  key_ = buf.substr( 0, colon_location );
  string value_temp = buf.substr( colon_location + separator.size() );

  /* strip whitespace */
  size_t first_nonspace = value_temp.find_first_not_of( " " );
  value_ = value_temp.substr( first_nonspace );

  /*
  fprintf( stderr, "Got header. key=[[%s]] value = [[%s]]\n",
	   key_.c_str(), value_.c_str() );
  */
}

bool HTTPHeaderParser::parse( const string & buf )
{
  /* step 1: append buf to internal buffer */
  internal_buffer_ += buf;

  /* are we looking for headers or for the body? */
  if ( headers_finished_ ) {
    /* looking for body */
    if ( body_left_ <= internal_buffer_.size() ) {
      /* reset me */
      internal_buffer_.replace( 0, body_left_, string() );
      request_line_.erase();
      headers_.clear();
      headers_finished_ = false;
      body_left_ = 0;
    } else {
      body_left_ -= internal_buffer_.size();
      internal_buffer_.erase();
    }
  }

  if ( headers_finished_ ) {
    return false;
  }

  const string crlf = "\r\n";

  /* step 2: parse all the lines */
  while ( 1 ) {
    /* step 2a: do we have a line? */
    size_t first_line_ending = internal_buffer_.find( crlf );
    if ( first_line_ending == std::string::npos ) {
      /* we don't have a full line yet */
      return false;
    }

    /* step 2b: yes, we have at least one full line */

    if ( request_line_.empty() ) { /* request line always comes first */
      request_line_ = internal_buffer_.substr( 0, first_line_ending );
    } else if ( first_line_ending == 0 ) { /* end of headers */
      headers_finished_ = true;
      body_left_ = body_len();
      internal_buffer_.replace( 0, first_line_ending + crlf.size(), string() );

      return true;
    } else { /* it's a header */
      headers_.emplace_back( internal_buffer_.substr( 0, first_line_ending ) );
    }

    /* step 2c: delete the parsed line from internal_buffer */
    internal_buffer_.replace( 0, first_line_ending + crlf.size(), string() );
  }
}

bool HTTPHeaderParser::has_header( const string & header_name ) const
{
  for ( const auto & header : headers_ ) {
    if ( header.key() == header_name ) {
      return true;
    }
  }

  return false;
}

string HTTPHeaderParser::get_header_value( const string & header_name ) const
{
  for ( const auto & header : headers_ ) {
    if ( header.key() == header_name ) {
      return header.value();
    }
  }

  throw Exception( "HTTPHeaderParser header not found", header_name );
}

size_t HTTPHeaderParser::body_len( void ) const
{
  assert( headers_parsed() );
  if ( request_line_.substr( 0, 4 ) == "GET " ) {
    return 0;
  } else if ( request_line_.substr( 0, 5 ) == "POST " ) {
    if ( !has_header( "Content-Length" ) ) {
      throw Exception( "HTTPHeaderParser does not support chunked requests or lowercase headers", "sorry" );
    }

    //    fprintf( stderr, "CONTENT-LENGTH is %s\n", get_header_value( "Content-Length" ).c_str() );
    return myatoi( get_header_value( "Content-Length" ) );
  } else {
    throw Exception( "Cannot handle HTTP method", request_line_ );
  }
}

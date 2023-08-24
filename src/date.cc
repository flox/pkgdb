/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#include <cmath>
#include <string>
#include <ctime>

#include "date.hh"

/* -------------------------------------------------------------------------- */

namespace datetime {

/* -------------------------------------------------------------------------- */

  std::pair<std::tm, std::string>
parseDate( std::string_view timestamp )
{
  static const char fmtYMD[] = "%Y-%m-%d";
  static const char fmtMDY[] = "%m-%d-%Y";
  std::tm       t {};
  std::string   s( timestamp );

  char * rest = strptime( s.c_str()
                        , ( timestamp.at( 4 ) == '-' ) ? fmtYMD : fmtMDY
                        , & t
                        );
  if ( rest == nullptr ) { return std::make_pair( t, "" ); }
  return std::make_pair( t, std::string( rest ) );
}


/* -------------------------------------------------------------------------- */

  unsigned long
parseDateToEpoch( std::string_view timestamp )
{
  std::tm t = parseDate( timestamp ).first;
  double  s = std::mktime( & t );
  return std::floor( s );
}


/* -------------------------------------------------------------------------- */

Date::Date( std::string_view timestamp )
{
  auto [time, rest] = parseDate( timestamp );
  this->time = std::move( time );
  this->rest = std::move( rest );
}

Date::operator std::time_t() const
{
  std::tm t = this->time;
  return std::mktime( & t );
}

  std::string
Date::to_string() const
{
  return this->stamp() + this->rest;
}


  std::string
Date::stamp() const
{
  char buffer[128];
  strftime( buffer, sizeof( buffer ), "%Y-%m-%d", & this->time );
  return std::string( buffer );
}

  unsigned long
Date::epoch() const
{
  return std::floor( (std::time_t) * this );
}

  char
Date::compare( const Date & other, bool ignoreRest ) const
{
  if ( this->time.tm_year < other.time.tm_year ) { return -1; }
  if ( other.time.tm_year < this->time.tm_year ) { return 1;  }

  if ( this->time.tm_mon < other.time.tm_mon ) { return -1; }
  if ( other.time.tm_mon < this->time.tm_mon ) { return 1;  }

  if ( this->time.tm_mday < other.time.tm_mday ) { return -1; }
  if ( other.time.tm_mday < this->time.tm_mday ) { return 1;  }

  if ( ignoreRest ) { return 0; }
  return ( this->rest < other.rest ) ? -1 :
         ( other.rest < this->rest ) ?  1 : 0;
}

  bool
Date::isBefore( const Date & other, bool ignoreRest ) const
{
  return this->compare( other, ignoreRest ) <= 0;
}


/* -------------------------------------------------------------------------- */

}  /* End Namespace `datetime' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */

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
  static const char fmtDMY[] = "%d-%m-%Y";

  std::tm       t;
  std::string   s( timestamp );
  char        * rest = strptime( s.c_str(), fmtYMD, & t );
  if ( rest == s.c_str() ) { rest = strptime( s.c_str(), fmtDMY, & t ); }
  if ( rest == nullptr )   { return std::make_pair( t, "" ); }
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

  bool
dateBefore( const std::tm & before, const std::tm & time )
{
  std::tm b = before;
  std::tm t = time;
  return std::mktime( & t ) <= std::mktime( & b );
}

  bool
dateBefore( std::string_view before, std::string_view timestamp )
{
  return dateBefore( parseDate( before ).first
                   , parseDate( timestamp ).first
                   );
}

  bool
dateBefore( const std::tm & before, std::string_view timestamp )
{
  return dateBefore( before, parseDate( timestamp ).first );
}

  bool
dateBefore( std::string_view before, const std::tm & time )
{
  return dateBefore( parseDate( before ).first, time );
}


/* -------------------------------------------------------------------------- */

  char
compareDates( const std::tm & a, const std::tm & b )
{
  std::tm _a = a;
  std::tm _b = b;
  auto    _ta = std::mktime( & _a );
  auto    _tb = std::mktime( & _b );
  return ( _ta < _tb ) ? -1 : ( _ta = _tb ) ? 0 : 1;
}

  char
compareDates( std::string_view a, std::string_view b )
{
  return compareDates( parseDate( a ).first, parseDate( b ).first );
}

  char
compareDates( const std::tm & a, std::string_view b )
{
  return compareDates( a, parseDate( b ).first );
}

  char
compareDates( std::string_view a, const std::tm & b )
{
  return compareDates( parseDate( a ).first, b );
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
  return std::floor( (std::time_t) this );
}

  bool
Date::isBefore( const Date & before, bool ignoreRest ) const
{
  if ( ignoreRest ) { return dateBefore( before.time, this->time ); }
  char cmp = compareDates( before.time, this->time );
  if ( cmp == 0 ) { return before.rest <= this->rest; }
  return cmp < 0;
}

  char
Date::compare( const Date & other, bool ignoreRest ) const
{
  long long epc = this->epoch() - other.epoch();
  if ( epc == 0 )
    {
      if ( ignoreRest ) { return 0; }
      return ( this->rest < other.rest ) ? -1 :
             ( other.rest < this->rest ) ?  1 : 0;
    }
  return ( epc < 0 ) ? -1 : 1;
}


/* -------------------------------------------------------------------------- */

}  /* End Namespace `datetime' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */

/* ========================================================================== *
 *
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include <string>
#include <ctime>
#include <functional>


/* -------------------------------------------------------------------------- */

namespace datetime {

/* -------------------------------------------------------------------------- */

/**
 * Example Dates
 *   2022-06-29
 *   %Y-%m-%d
 * or
 *   2022-06-29-pre
 *   %Y-%m-%d-(<CAPTURED>)
 * or
 *   06-29-2022
 *   %m-%d-%Y
 * or
 *   06-29-2022-pre
 *   %m-%d-%Y-(<CAPTURED>)
 * @return A parsed date struct, and any remaining unparsed parts of the string.
 */
std::pair<std::tm, std::string> parseDate( std::string_view timestamp );

unsigned long parseDateToEpoch( std::string_view timestamp );


/* -------------------------------------------------------------------------- */

struct Date {

  std::tm     time;
  std::string rest;

  Date( std::string_view timestamp );

  Date( const time_t secondsSinceEpoch )
    : time( * gmtime( & secondsSinceEpoch ) ) {}

  Date( const unsigned long secondsSinceEpoch )
    : Date( (time_t) secondsSinceEpoch ) {}

  Date( const std::tm & time ) : time( time ) {};

  std::string   to_string() const;
  std::string   stamp()     const;
  unsigned long epoch()     const;

  /**
   * @param ignoreRest Whether @a rest should be used to break ties by
   *                   lexicographical comparison with `<=`.
   * @return `true` iff @a this is <= @a other.
   */
  bool isBefore( const Date & other, bool ignoreRest = true ) const;

  /**
   * @param ignoreRest Whether @a rest should be used to break ties by
   *                   lexicographical comparison.
   * @return `-1` iff @a this < @a other, `0` iff @a this = @a other,
   *         `1` iff @a other < @a this.
   */
  char compare(const Date & other,  bool ignoreRest = true ) const;

  operator unsigned long() const { return this->epoch(); }
  operator std::time_t()   const;

};


/* -------------------------------------------------------------------------- */

}  /* End Namespace `datetime' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */

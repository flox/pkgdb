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

/** @return `true` iff @a time is <= @a before. */
bool dateBefore( const std::tm & before, const std::tm & time );
/** @return `true` iff @a time is <= @a before. */
bool dateBefore( std::string_view before, std::string_view timestamp );
/** @return `true` iff @a time is <= @a before. */
bool dateBefore( const std::tm & before, std::string_view timestamp );
/** @return `true` iff @a time is <= @a before. */
bool dateBefore( std::string_view before, const std::tm & time );


/* -------------------------------------------------------------------------- */

/** @return `-1` iff @a a < @a b, `0` iff @a a = @a b, `1`iff @a b < @a a. */
char compareDates( const std::tm & a, const std::tm & b );
/** @return `-1` iff @a a < @a b, `0` iff @a a = @a b, `1`iff @a b < @a a. */
char compareDates( std::string_view a, std::string_view b );
/** @return `-1` iff @a a < @a b, `0` iff @a a = @a b, `1`iff @a b < @a a. */
char compareDates( const std::tm & a, std::string_view b );
/** @return `-1` iff @a a < @a b, `0` iff @a a = @a b, `1`iff @a b < @a a. */
char compareDates( std::string_view a, const std::tm & b );


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
   * @return `true` iff @a this is <= @a before.
   */
  bool isBefore( const Date & before, bool ignoreRest = true ) const;

  /**
   * @param ignoreRest Whether @a rest should be used to break ties by
   *                   lexicographical comparison.
   * @return `-1` iff @a a < @a b, `0` iff @a a = @a b, `1`iff @a b < @a a.
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

/* ========================================================================== *
 *
 * @file compat/concepts.hh
 *
 * @brief Provides backports of some `concepts` features that are missing in
 *        Clang v11.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#if defined( __clang__ ) && ( __clang_major__ < 15 )

/* -------------------------------------------------------------------------- */

#include <concepts>


/* -------------------------------------------------------------------------- */

namespace std {

/* -------------------------------------------------------------------------- */

/**
 * The concept `destructible` specifies the concept of all types whose
 * instances can safely be destroyed at the end of their lifetime
 * ( including reference types ).
 */
  template <class T>
concept destructible = std::is_nothrow_destructible_v<T>;


/* -------------------------------------------------------------------------- */

/**
 * The `constructible_from` concept specifies that a variable of type @a T can
 * be initialized with the given set of argument types @a Args....
 */
  template <class T, class... Args>
concept constructible_from =
  std::destructible<T> && std::is_constructible_v<T, Args...>;


/* -------------------------------------------------------------------------- */

/**
 * The concept `convertible_to<From, To>` specifies that an expression of the
 * same type and value category as those of `std::declval<From>()` can be
 * implicitly and explicitly converted to the type @a To, and the two forms of
 * conversion are equivalent.
 */
  template <class From, class To>
concept convertible_to =
  std::is_convertible_v<From, To> &&
  requires { static_cast<To>( std::declval<From>() ); };


/* -------------------------------------------------------------------------- */

}  /* End namespace `std' */

/* -------------------------------------------------------------------------- */

#endif  // defined( __clang__ ) && ( __clang_major__ < 15 )


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */

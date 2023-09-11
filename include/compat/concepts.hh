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

/* Yeah, I know: "iT's IlLeGaL tO eXtEnD `std::*'".
 * This backports definitions exactly as written, so it's fine. */
namespace std {

/* -------------------------------------------------------------------------- */

#if 0
namespace detail {
  template<class T, class U> concept SameHelper = std::is_same_v<T, U>;
}  /* End namespace `std::detail' */

/**
 * The concept `same_as<T, U>` is satisfied if and only if @a T and @a U denote
 * the same type.
 * `std::same_as<T, U>` subsumes `std::same_as<U, T>` and vice versa.
 */
  template<class T, class U>
concept same_as = detail::SameHelper<T, U> && detail::SameHelper<U, T>;
#endif


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

/**
 * The concept `derived_from<Derived, Base>` is satisfied if and only if
 * @a Base is a class type that is either @a Derived or a public and unambiguous
 * base of @a Derived, ignoring cv-qualifiers.
 * Note that this behavior is different to `std::is_base_of` when @a Base is a
 * private or protected base of @a Derived.
 */
  template<class Derived, class Base>
concept derived_from =
  std::is_base_of_v<Base, Derived> &&
  std::is_convertible_v<const volatile Derived *, const volatile Base *>;


/* -------------------------------------------------------------------------- */

}  /* End namespace `std' */

/* -------------------------------------------------------------------------- */

#endif  // defined( __clang__ ) && ( __clang_major__ < 15 )


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */

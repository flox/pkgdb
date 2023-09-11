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

/**
 * The concept `common_reference_with<T, U>` specifies that two types @a T and
 * @a U share a common reference type
 * ( as computed by `std::common_reference_t` ) to which both can be converted.
 */
  template<class T, class U>
concept common_reference_with =
  std::same_as<std::common_reference_t<T, U>, std::common_reference_t<U, T>> &&
  std::convertible_to<T, std::common_reference_t<T, U>> &&
  std::convertible_to<U, std::common_reference_t<T, U>>;


/* -------------------------------------------------------------------------- */

/**
 * The concept `assignable_from<LHS, RHS>` specifies that an expression of the
 * type and value category specified by @a RHS can be assigned to an lvalue
 * expression whose type is specified by @a LHS.
 */
  template<class LHS, class RHS>
concept assignable_from =
  std::is_lvalue_reference_v<LHS> &&
  std::common_reference_with<
    const std::remove_reference_t<LHS>&,
    const std::remove_reference_t<RHS>&> &&
  requires( LHS lhs, RHS && rhs ) {
    { lhs = std::forward<RHS>(rhs) } -> std::same_as<LHS>;
  };


/* -------------------------------------------------------------------------- */

/**
 * The concept `common_with<T, U>` specifies that two types @a T and @a U share
 * a common type ( as computed by `std::common_type_t` ) to which both can
 * be converted.
 */
  template <class T, class U>
concept common_with =
  std::same_as<std::common_type_t<T, U>, std::common_type_t<U, T>> &&
  requires {
    static_cast<std::common_type_t<T, U>>( std::declval<T>() );
    static_cast<std::common_type_t<T, U>>( std::declval<U>() );
  } &&
  std::common_reference_with<
    std::add_lvalue_reference_t<const T>,
    std::add_lvalue_reference_t<const U>> &&
  std::common_reference_with<
    std::add_lvalue_reference_t<std::common_type_t<T, U>>,
    std::common_reference_t<
      std::add_lvalue_reference_t<const T>,
      std::add_lvalue_reference_t<const U>>>;


/* -------------------------------------------------------------------------- */

}  /* End namespace `std' */

/* -------------------------------------------------------------------------- */

#endif  // defined( __clang__ ) && ( __clang_major__ < 15 )


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */

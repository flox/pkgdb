#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ( [2.71] )
AC_INIT([pkgdb], m4_include([version]), [alex.ameen@floxdev.com])

AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_AUX_DIR([build-aux])

AC_CONFIG_SRCDIR([src/main.cc])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.
AC_CHECK_LIB(
  [nixfetchers]dnl
, translit( [_ZN3nix8fetchers11attrsToJSONERKSt3mapINSt7]
            [__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt7variantIJS7]
            [_mNS_8ExplicitIbEEEESt4lessIS7_ESaISt4pairIKS7_SB_EEE]
          , ' '
          )
)

# Checks for header files.
AC_FUNC_ALLOCA
AC_CHECK_HEADERS([unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_MODE_T
AC_FUNC_OBSTACK
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_SSIZE_T
AC_STRUCT_TIMEZONE
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_FSEEKO
AC_FUNC_MALLOC
AC_FUNC_MBRTOWC
AC_HEADER_MAJOR
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_FUNC_STRTOD
AC_CHECK_FUNCS([
  atexit 
  btowc 
  clock_gettime
  getdelim
  isascii
  iswprint
  localeconv
  localtime_r
  mblen
  mbrlen
  memset
  putenv
  realpath
  rpmatch
  select
  setenv
  setlocale
  strtol
  strtoul
  strtoull
  tzset
  wcwidth
])

PKG_CHECK_MODULES([NIX], [nix-store nix-main nix-cmd nix-expr])
PKG_CHECK_MODULES([NLJSON], [nlohmann_json])
PKG_CHECK_MODULES([SQLITE3], [sqlite3])
PKG_CHECK_MODULES([SQLITE3PP], [sqlite3pp])
PKG_CHECK_MODULES([YAML], [yaml-cpp])
PKG_CHECK_MODULES([ARGPARSE], [argparse])

AC_CONFIG_FILES([config.mk])
AC_OUTPUT

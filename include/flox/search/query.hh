#pragma once

#include "flox/pkgdb/pkg-query.hh"

namespace flox::search {
/** @brief Package query parser. */
struct PkgQueryMixin
{

  pkgdb::PkgQuery query;

  /**
   * @brief Add `query` argument to any parser to construct a
   *        @a flox::pkgdb::PkgQuery.
   */
  argparse::Argument &
  addQueryArgs( argparse::ArgumentParser & parser );

  /**
   * @brief Run query on a @a pkgdb::PkgDbReadOnly database.
   *
   * Any scraping should be performed before invoking this function.
   */
  std::vector<pkgdb::row_id>
  queryDb( pkgdb::PkgDbReadOnly & pdb ) const;


}; /* End struct `PkgQueryMixin' */
}  // namespace flox::search

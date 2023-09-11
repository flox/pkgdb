/* ========================================================================== *
 *
 * @file flox/resolver/command.hh
 *
 * @brief Executable command helpers, argument parsers, etc.
 *
 *
 * -------------------------------------------------------------------------- */

#pragma once

#include "flox/search/command.hh"
#include "flox/resolver/params.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

/**
 * @brief Resolve a set of package requirements to a set of
 *        satisfactory installables.
 */
struct ResolveCommand
  : public pkgdb::PkgDbRegistryMixin
  , public search::PkgQueryMixin
{

  private:

    ResolveOneParams params;

  /**
   * @brief Add argument to any parser to construct
   *        a @a flox::resolver::ResolveOneParams.
   */
    argparse::Argument &
  addResolveParamArgs( argparse::ArgumentParser & parser );


  protected:

      [[nodiscard]]
      virtual RegistryRaw
    getRegistryRaw() override { return this->params.registry; }

      [[nodiscard]]
      virtual std::vector<std::string> &
    getSystems() override { return this->params.systems; }


  public:

    command::VerboseParser parser;

    ResolveCommand();

    /** @brief Display a single row from the given @a input. */
      void
    showRow( pkgdb::PkgDbInput & input, pkgdb::row_id row )
    {
      std::cout << input.getRowJSON( row ).dump() << std::endl;
    }

    /**
     * @brief Execute the `resolve` routine.
     *
     * @return `EXIT_SUCCESS` or `EXIT_FAILURE`.
     */
    int run();

};  /* End struct `ResolveCommand' */


/* -------------------------------------------------------------------------- */

}  /* End namespaces `flox::resolver' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */

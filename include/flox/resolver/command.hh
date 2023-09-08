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

struct ResolveCommand
  : public search::PkgDbRegistryMixin
  , public search::PkgQueryMixin
{

  private:

    ResolveOneParams params;

  /** Add argument to any parser to construct a @a ResolveOneParams. */
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

    /** Display a single row from the given @a input. */
    void showRow( std::string_view    inputName
                , pkgdb::PkgDbInput & input
                , pkgdb::row_id       row
                );

    /**
     * Execute the `resolve` routine.
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

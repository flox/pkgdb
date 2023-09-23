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
#include "flox/resolver/state.hh"


/* -------------------------------------------------------------------------- */

namespace flox::resolver {

/* -------------------------------------------------------------------------- */

/**
 * @brief Resolve a set of package requirements to a set of
 *        satisfactory installables.
 */
struct ResolveCommand : public pkgdb::PkgDbRegistryMixin<> {

  private:

    ResolveOneParams params;

    /**
     * @brief Add argument to any parser to construct
     *        a @a flox::resolver::ResolveOneParams.
     */
      argparse::Argument &
    addResolveParamArgs( argparse::ArgumentParser & parser );

    [[nodiscard]] ResolverState getResolverState() const;

    [[nodiscard]]
    PkgDescriptorRaw getQuery() const { return this->params.query; }


  protected:

      [[nodiscard]]
      virtual RegistryRaw
    getRegistryRaw() override
    {
      return this->params.registry;
    }

      [[nodiscard]]
      virtual std::vector<std::string> &
    getSystems() override
    {
      return this->params.systems;
    }


  public:

    command::VerboseParser parser;

    ResolveCommand();

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

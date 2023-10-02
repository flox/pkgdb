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
struct ResolveCommand : public pkgdb::PkgDbRegistryMixin {

  private:

    ResolveOneParams       params;  /**< Query arguments and inputs */
    command::VerboseParser parser;  /**< Query arguments and inputs parser */

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
    RegistryRaw getRegistryRaw() override { return this->params.registry; }

      [[nodiscard]]
      std::vector<std::string> &
    getSystems() override
    {
      return this->params.systems;
    }


  public:

    ResolveCommand();

    [[nodiscard]] command::VerboseParser & getParser() { return this->parser; }

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

/* ========================================================================== *
 *
 * @file repl.hh
 *
 * @brief Executable command helpers, argument parsers, etc.
 *
 *
 * -------------------------------------------------------------------------- */

#include <nix/repl.hh>

#include "flox/repl.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

ReplCommand::ReplCommand() : parser( "repl" )
{
  this->parser.add_description(
    "Run an interactive `nix` REPL with extensions" );
}


/* -------------------------------------------------------------------------- */

int
ReplCommand::run()
{
  auto repl = nix::AbstractNixRepl::create(
    nix::SearchPath(),
    this->getStore(),
    this->getState(),
    [&]() { return nix::AbstractNixRepl::AnnotatedValues(); } );
  repl->initEnv();
  repl->mainLoop();
  return EXIT_SUCCESS;
}


/* -------------------------------------------------------------------------- */

}  // namespace flox


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */

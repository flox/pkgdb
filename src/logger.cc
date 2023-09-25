/* ========================================================================== *
 *
 * @file logger.cc
 *
 * @brief Custom `nix::Logger` implementation used to filter some messages.
 *
 *
 * -------------------------------------------------------------------------- */

#include <nix/logging.hh>
#include <nix/util.hh>

#include "flox/core/nix-state.hh"


/* -------------------------------------------------------------------------- */

namespace flox {

/* -------------------------------------------------------------------------- */

/**
 * @brief Custom `nix::Logger` implementation used to filter some messages.
 *
 * This is an exact copy of `nix::SimpleLogger` with the addition of filtering
 * in the `log` routine.
 */
class FilteredLogger : public nix::Logger {

  protected:

    /** @brief Detect ignored warnings. */
      bool
    shouldIgnoreWarning( const std::string & str )
    {
      if ( str.find( " has an override for a non-existent input " ) !=
           std::string::npos
         )
        {
          return true;
        }

      return false;
    }


    /** @brief Detect ignored messages. */
      bool
    shouldIgnoreMsg( std::string_view str )
    {
      (void) str;
      return false;
    }


  public:

    bool systemd;         /**< Whether we should emit `systemd` style logs. */
    bool tty;             /**< Whether we should emit TTY colors in logs. */
    bool printBuildLogs;  /**< Whether we should emit build logs. */

    FilteredLogger( bool printBuildLogs )
      : systemd( nix::getEnv( "IN_SYSTEMD" ) == "1" )
      , tty( nix::shouldANSI() )
      , printBuildLogs( printBuildLogs )
    {}


    /** @brief Whether the logger prints the whole build log. */
    bool isVerbose() override { return this->printBuildLogs; }


    /** @brief Emit a log message with a colored "warning:" prefix. */
      void
    warn( const std::string & msg ) override
    {
      if ( ! this->shouldIgnoreWarning( msg ) )
        {
          /* NOTE: The `nix' definitions of `ANSI_WARNING' and `ANSI_NORMAL'
           *       use `\e###` escapes, but `gcc' will gripe at you for not
           *       following ISO standard.
           *       We use equivalent `\033###' sequences instead.' */
          this->log( nix::lvlWarn
                   , /* ANSI_WARNING */ "\033[35;1m"
                     "warning:"
                     /* ANSI_NORMAL */ "\033[0m"
                     " " + msg
                   );
        }
    }


    /**
     * @brief Emit a log line depending on verbosity setting.
     * @param lvl Minimum required verbosity level to emit the message.
     * @param str The message to emit.
     */
      void
    log( nix::Verbosity lvl, std::string_view str ) override
    {
      if ( ( nix::verbosity < lvl ) || this->shouldIgnoreMsg( str ) )
        {
          return;
        }

      /* Handle `systemd' style log level prefixes. */
      std::string prefix;
      if ( systemd )
        {
          char levelChar;
          switch ( lvl )
            {
              case nix::lvlError:     levelChar = '3'; break;

              case nix::lvlWarn:      levelChar = '4'; break;

              case nix::lvlNotice:
              case nix::lvlInfo:      levelChar = '5'; break;

              case nix::lvlTalkative:
              case nix::lvlChatty:    levelChar = '6'; break;

              case nix::lvlDebug:
              case nix::lvlVomit:     levelChar = '7'; break;

              /* Should not happen, and missing enum case is reported
               * by `-Werror=switch-enum' */
              default: levelChar = '7'; break;
            }
          prefix = std::string( "<" ) + levelChar + ">";
        }

      nix::writeToStderr(
        prefix + nix::filterANSIEscapes( str, ! tty ) + "\n"
      );
    }


    /** @brief Emit error information. */
      void
    logEI( const nix::ErrorInfo & einfo ) override
    {
      std::stringstream oss;
      /* From `nix/error.hh' */
      showErrorInfo( oss, einfo, nix::loggerSettings.showTrace.get() );

      this->log( einfo.level, oss.str() );
    }


    /** @brief Begin an activity block. */
      void
    startActivity(
            nix::ActivityId     /* act ( unused ) */
    ,       nix::Verbosity      lvl
    ,       nix::ActivityType   /* type ( unused ) */
    , const std::string       & str
    , const Fields            & /* fields ( unused ) */
    ,       nix::ActivityId     /* parent ( unused ) */
    ) override
    {
      if ( ( lvl <= nix::verbosity ) && ( ! str.empty() ) )
        {
          this->log( lvl, str + "..." );
        }
    }


    /** @brief Report the result of an RPC call with a remote `nix` store. */
      void
    result(       nix::ActivityId   /* act ( unused ) */
          ,       nix::ResultType   type
          , const Fields          & fields
          ) override
    {
      if ( ! this->printBuildLogs ) { return; }
      if ( type == nix::resBuildLogLine )
        {
          this->log( nix::lvlError, fields[0].s );
        }
      else if ( type == nix::resPostBuildLogLine )
        {
          this->log( nix::lvlError, "post-build-hook: " + fields[0].s );
        }
    }


};  /* End class `FilteredLogger' */


/* -------------------------------------------------------------------------- */

  nix::Logger *
makeFilteredLogger( bool printBuildLogs )
{
  return new FilteredLogger( printBuildLogs );
}


/* -------------------------------------------------------------------------- */


}  /* End namespace `flox' */


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */

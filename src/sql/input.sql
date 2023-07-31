-- ========================================================================== --
--
-- Holds metadata information about the flake a given database is
-- associated with.
--
-- This table should only contain a single row.
--
--
-- -------------------------------------------------------------------------- --

CREATE TABLE IF NOT EXISTS LockedFlake (
  fingerprint  TEXT  PRIMARY KEY
, string       TEXT  NOT NULL
, attrs        JSON  NOT NULL
);


-- -------------------------------------------------------------------------- --
--
--
--
-- ========================================================================== --


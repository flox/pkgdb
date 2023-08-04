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

-- Limit the table to a single row.
CREATE TRIGGER IF NOT EXISTS IT_LockedFlake AFTER INSERT ON LockedFlake
  WHEN ( 1 < ( SELECT COUNT( fingerprint ) FROM LockedFlake ) )
  BEGIN
    SELECT RAISE( ABORT, 'Cannot write conflicting LockedFlake info.' );
  END;


-- -------------------------------------------------------------------------- --
--
--
--
-- ========================================================================== --


-- ========================================================================== --
--
-- Snippet to be used with `sqlite> .read ./tmp-paths.sql' or
-- `sqlite3_exec(...)' which creates a temporary table and view for processing
-- attribute paths.
--
-- This aligns with the schema used by the `PackageSets' table, and can be used
-- to extract special path parts without creating redundant views.
--
--
-- -------------------------------------------------------------------------- --

CREATE TEMPORARY TABLE TmpPaths (
  pathId  INTEGER PRIMARY KEY
, path    JSON    NOT NULL UNIQUE
);

-- -------------------------------------------------------------------------- --

CREATE VIEW IF NOT EXISTS temp.v_TmpPaths_Parts AS SELECT
    pathId
  , subtree
  , system
  , iif( subtree = 'catalog', json_extract( rest, '$[0]' ), NULL )
    AS stability
  , iif( subtree = 'catalog', json_remove( rest, '$[0]' ), rest ) AS rest
  FROM (
    SELECT   pathId
           , json_extract( path, '$[0]' )        AS subtree
           , json_extract( path, '$[1]' )        AS system
           , json_remove( path, '$[0]', '$[0]' ) AS rest
    FROM temp.TmpPaths
  );


-- -------------------------------------------------------------------------- --
--
--
--
-- ========================================================================== --

-- ========================================================================== --
--
--
--
-- -------------------------------------------------------------------------- --

CREATE TABLE IF NOT EXISTS PackageSets (
  pathId  INTEGER PRIMARY KEY
, path    JSON    NOT NULL UNIQUE
);

CREATE UNIQUE INDEX IF NOT EXISTS idx_PackageSets ON PackageSets ( path );


-- -------------------------------------------------------------------------- --

INSERT OR IGNORE INTO PackageSets ( path ) VALUES
-- Standard
  ( '["packages","x86_64-linux"]'   )
, ( '["packages","x86_64-darwin"]'  )
, ( '["packages","aarch64-linux"]'  )
, ( '["packages","aarch64-darwin"]' )
-- Legacy
, ( '["legacyPackages","x86_64-linux"]'   )
, ( '["legacyPackages","x86_64-darwin"]'  )
, ( '["legacyPackages","aarch64-linux"]'  )
, ( '["legacyPackages","aarch64-darwin"]' )
-- Catalogs
, ( '["catalog","x86_64-linux","stable"]'     )
, ( '["catalog","x86_64-linux","staging"]'    )
, ( '["catalog","x86_64-linux","unstable"]'   )
, ( '["catalog","x86_64-darwin","stable"]'    )
, ( '["catalog","x86_64-darwin","staging"]'   )
, ( '["catalog","x86_64-darwin","unstable"]'  )
, ( '["catalog","aarch64-linux","stable"]'    )
, ( '["catalog","aarch64-linux","staging"]'   )
, ( '["catalog","aarch64-linux","unstable"]'  )
, ( '["catalog","aarch64-darwin","stable"]'   )
, ( '["catalog","aarch64-darwin","staging"]'  )
, ( '["catalog","aarch64-darwin","unstable"]' )
;


-- -------------------------------------------------------------------------- --

CREATE VIEW IF NOT EXISTS v_PackageSets_PathParts AS SELECT
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
    FROM PackageSets
  );


-- -------------------------------------------------------------------------- --
--
--
--
-- ========================================================================== --

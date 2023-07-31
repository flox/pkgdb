-- ========================================================================== --
--
--
--
-- -------------------------------------------------------------------------- --

.read ./package-sets.sql

-- -------------------------------------------------------------------------- --

CREATE TABLE IF NOT EXISTS Descriptions (
  id           INTEGER PRIMARY KEY
, description  TEXT    NOT NULL UNIQUE
);

CREATE UNIQUE INDEX IF NOT EXISTS idx_Descriptions
  ON Descriptions ( description );


-- -------------------------------------------------------------------------- --

CREATE TABLE IF NOT EXISTS Packages (
  id                INTEGER PRIMARY KEY
, parentId          INTEGER        NOT NULL
, attrName          VARCHAR( 255 ) NOT NULL
, name              VARCHAR( 255 ) NOT NULL
, pname             VARCHAR( 255 )
, version           VARCHAR( 127 )
, semver            VARCHAR( 127 )
, license           VARCHAR( 255 )
, outputs           JSON           NOT NULL
, outputsToInstall  JSON
, broken            BOOL
, unfree            BOOL
, descriptionId     INTEGER

, FOREIGN KEY ( parentId      ) REFERENCES PackageSets  ( pathId )
, FOREIGN KEY ( descriptionId ) REFERENCES Descriptions ( id     )
);

CREATE UNIQUE INDEX IF NOT EXISTS idx_Packages
  ON Packages ( parentId, attrName );


-- -------------------------------------------------------------------------- --

-- A list of all attribute paths to packages with their `id'.
CREATE VIEW IF NOT EXISTS v_Packages_Paths AS SELECT 
    id
  , json_insert( path, '$[#]', Packages.attrName ) AS path
  FROM Packages
    INNER JOIN PackageSets ON Packages.parentId = PackageSets.pathId;


-- -------------------------------------------------------------------------- --

-- A view intended for performing searches with package metadata.
CREATE VIEW IF NOT EXISTS v_Packages_SearchBasic AS SELECT
    Packages.id
  , subtree
  , system
  , stability
  , json_insert( rest, '$[#]', attrName ) as RelPath
  , name
  , pname
  , version
  , semver
  , license
  , broken
  , unfree
  , description
  FROM Packages
    INNER JOIN v_PackageSets_PathParts
      ON Packages.parentId = v_PackageSets_PathParts.pathId
    INNER JOIN Descriptions ON Packages.descriptionId = Descriptions.id;


-- -------------------------------------------------------------------------- --
--
--
--
-- ========================================================================== --

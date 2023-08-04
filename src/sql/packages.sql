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

, FOREIGN KEY ( parentId      ) REFERENCES AttrSets  ( id )
, FOREIGN KEY ( descriptionId ) REFERENCES Descriptions ( id     )
, CONSTRAINT UC_Packages UNIQUE ( parentId, attrName )
);

CREATE UNIQUE INDEX IF NOT EXISTS idx_Packages
  ON Packages ( parentId, attrName );


-- -------------------------------------------------------------------------- --
--
--
--
-- ========================================================================== --

/* ========================================================================== *
 *
 * @file pkgdb/schemas.hh
 *
 * @brief SQL Schemas to initialize a package database.
 *
 *
 * -------------------------------------------------------------------------- */

/* Holds metadata information about schema versions. */


static const char * sql_versions = R"SQL(
CREATE TABLE IF NOT EXISTS DbVersions (
  name     TEXT NOT NULL PRIMARY KEY
, version  TEXT NOT NULL
)
)SQL";


/* -------------------------------------------------------------------------- */

static const char * sql_input = R"SQL(
CREATE TABLE IF NOT EXISTS LockedFlake (
  fingerprint  TEXT  PRIMARY KEY
, string       TEXT  NOT NULL
, attrs        JSON  NOT NULL
);

CREATE TRIGGER IF NOT EXISTS IT_LockedFlake AFTER INSERT ON LockedFlake
  WHEN ( 1 < ( SELECT COUNT( fingerprint ) FROM LockedFlake ) )
  BEGIN
    SELECT RAISE( ABORT, 'Cannot write conflicting LockedFlake info.' );
  END
)SQL";


/* -------------------------------------------------------------------------- */

static const char * sql_attrSets = R"SQL(
CREATE TABLE IF NOT EXISTS AttrSets (
  id        INTEGER       PRIMARY KEY
, parent    INTEGER
, attrName  VARCHAR( 255) NOT NULL
, CONSTRAINT  UC_AttrSets UNIQUE ( id, parent )
);

CREATE UNIQUE INDEX IF NOT EXISTS idx_AttrSets ON AttrSets ( id, parent );

CREATE TRIGGER IF NOT EXISTS IT_AttrSets AFTER INSERT ON AttrSets
  WHEN
    ( NEW.id = NEW.parent ) OR
    ( ( SELECT NEW.parent != 0 ) AND
      ( ( SELECT COUNT( id ) FROM AttrSets WHERE ( NEW.parent = AttrSets.id ) )
        < 1
      )
    )
  BEGIN
    SELECT RAISE( ABORT, 'No such AttrSets.id for parent.' );
  END
)SQL";


/* -------------------------------------------------------------------------- */

static const char * sql_packages = R"SQL(
CREATE TABLE IF NOT EXISTS Descriptions (
  id           INTEGER PRIMARY KEY
, description  TEXT    NOT NULL UNIQUE
);

CREATE UNIQUE INDEX IF NOT EXISTS idx_Descriptions
  ON Descriptions ( description );

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
  ON Packages ( parentId, attrName )
)SQL";


/* -------------------------------------------------------------------------- */

static const char * sql_view_packages = R"SQL(
CREATE VIEW IF NOT EXISTS v_PackagesSearch (
  Packages.id
, Packages.parentId
, Packages.attrName
, Packages.name
, Packages.pname
, Packages.version
, Packages.semver
, Packages.license
, Packages.broken
, Packages.unfree
, Descriptions.description
) FROM Packages JOIN Descriptions ON Packages.descriptionId = Descriptions.id
)SQL";


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */

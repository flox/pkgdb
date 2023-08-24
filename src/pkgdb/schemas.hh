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

static const char * sql_views = R"SQL(
CREATE VIEW IF NOT EXISTS v_AttrPaths AS
  WITH Tree ( id, parent, attrName, subtree, system, stability, path ) AS
  (
    SELECT id, parent, attrName
         , attrName                     AS subtree
         , NULL                         AS system
         , NULL                         AS stability
         , ( '["' || attrName || '"]' ) AS path
    FROM AttrSets WHERE ( parent = 0 )
    UNION ALL SELECT O.id, O.parent
                   , O.attrName
                   , Parent.subtree
                   , iif( ( Parent.system IS NULL ), O.attrName, Parent.system )
                     AS system
                   , iif( ( Parent.subtree = 'catalog' )
                        , iif( ( ( Parent.stability IS NULL ) AND
                                 ( Parent.system IS NOT NULL )
                               )
                             , O.attrName
                             , NULL
                             )
                        , NULL
                        )
                     AS stability
                   , json_insert( Parent.path, '$[#]', O.attrName ) AS path
    FROM AttrSets O INNER JOIN Tree as Parent ON ( Parent.id = O.parent )
  ) SELECT * FROM Tree;

CREATE VIEW IF NOT EXISTS v_Semvers AS SELECT
  semver
, major
, minor
, ( iif( ( length( mPatch ) < 1 ), rest, mPatch ) ) AS patch
, ( iif( ( length( mPatch ) < 1 ), NULL, rest ) )   AS preTag
FROM (
  SELECT semver
       , major
       , minor
       , ( substr( rest, 0, instr( rest, '-' ) ) )  AS mPatch
       , ( substr( rest, instr( rest, '-' ) + 1 ) ) AS rest
  FROM (
    SELECT semver
         , major
         , ( substr( rest, 0, instr( rest, '.' ) ) )  AS minor
         , ( substr( rest, instr( rest, '.' ) + 1 ) ) AS rest
    FROM (
      SELECT semver
           , ( substr( semver, 0, instr( semver, '.' ) ) )  AS major
           , ( substr( semver, instr( semver, '.' ) + 1 ) ) AS rest
      FROM ( SELECT DISTINCT semver FROM Packages )
    )
  )
) ORDER BY major, minor, patch, preTag DESC NULLS FIRST;

CREATE VIEW IF NOT EXISTS v_PackagesSearch AS SELECT
  Packages.id
, v_AttrPaths.subtree
, v_AttrPaths.system
, v_AttrPaths.stability
, json_insert( v_AttrPaths.path, '$[#]', Packages.attrName ) AS path
, Packages.name
, Packages.pname
, Packages.version
, Packages.semver
, Packages.license
, Packages.broken
, Packages.unfree
, Descriptions.description
FROM Packages
JOIN Descriptions         ON ( Packages.descriptionId = Descriptions.id  )
JOIN v_AttrPaths          ON ( Packages.parentId      = v_AttrPaths.id   )
FULL OUTER JOIN v_Semvers ON ( Packages.semver        = v_Semvers.semver )
)SQL";


/* -------------------------------------------------------------------------- *
 *
 *
 *
 * ========================================================================== */

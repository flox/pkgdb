-- ========================================================================== --
--
--
--
-- -------------------------------------------------------------------------- --

CREATE TABLE IF NOT EXISTS AttrSets (
  id        INTEGER       PRIMARY KEY
, parent    INTEGER
, attrName  VARCHAR( 255) NOT NULL UNIQUE

, CONSTRAINT UC_AttrSets UNIQUE ( id, parent )
);

CREATE UNIQUE INDEX IF NOT EXISTS idx_AttrSets ON AttrSets ( id, parent );


-- -------------------------------------------------------------------------- --
--
--
--
-- ========================================================================== --

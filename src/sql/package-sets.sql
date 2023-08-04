-- ========================================================================== --
--
--
--
-- -------------------------------------------------------------------------- --

CREATE TABLE IF NOT EXISTS AttrSets (
  id        INTEGER       PRIMARY KEY
, parent    INTEGER
, attrName  VARCHAR( 255) NOT NULL

, CONSTRAINT  UC_AttrSets UNIQUE ( id, parent )
);

CREATE UNIQUE INDEX IF NOT EXISTS idx_AttrSets ON AttrSets ( id, parent );


-- -------------------------------------------------------------------------- --

-- Ensure `parent' is an existing `AttrSets.id' or 0.
-- Also ensure `NEW.id' != `NEW.parent'  ( recursive reference )
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
  END;


-- -------------------------------------------------------------------------- --
--
--
--
-- ========================================================================== --

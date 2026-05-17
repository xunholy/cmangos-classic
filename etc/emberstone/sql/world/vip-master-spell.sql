-- Wayfarer's Boon (spell 91200) — the VIP master spell.
--
-- mangos reads spell data from spell_template at world load. This row
-- defines a single instant self-cast spell with Effect1 = SPELL_EFFECT_DUMMY
-- (3) — mangos treats it as a valid cast that does nothing on the engine
-- side. The VipModule::OnCast hook detects spell.Id == 91200 and cascades
-- into Vip.BundledSpellIds (default: 10 vanilla world/consumable buffs).
--
-- We only set the columns we care about; spell_template has 158 columns
-- with DEFAULT 0 so omitted columns just take the default.
--
-- Idempotent: DELETE+INSERT pattern, safe to re-run.

DELETE FROM `spell_template` WHERE `Id` = 91200;

INSERT INTO `spell_template`
  (`Id`, `SpellName`,
   `Attributes`, `AttributesEx`,
   `Targets`,
   `CastingTimeIndex`, `RangeIndex`,
   `Effect1`, `EffectImplicitTargetA1`,
   `SpellVisual`, `SpellIconID`)
VALUES
  (91200, "Wayfarer''s Boon",
   0, 0,
   0,
   1,    -- CastingTimeIndex 1 = instant cast
   13,   -- RangeIndex 13 = self range
   3, 1, -- Effect1 = SPELL_EFFECT_DUMMY, target SELF
   4230, -- SpellVisual = Rallying Cry's golden flash
   1548  -- SpellIconID = generic buff scroll icon
  );

DROP TABLE IF EXISTS `custom_attunement_player_config`;
DROP TABLE IF EXISTS `custom_attunement_account_boost`;
DROP TABLE IF EXISTS `custom_hardcore_loot_gameobjects`;
DROP TABLE IF EXISTS `custom_hardcore_loot_tables`;
DROP TABLE IF EXISTS `custom_hardcore_grave_gameobjects`;
DROP TABLE IF EXISTS `custom_hardcore_player_config`;
DROP TABLE IF EXISTS `custom_hardcore_player_deathlog`;

-- Dualspec uninstall: tables NOT dropped automatically — they hold
-- per-player action bars + talent assignments + spec names that are
-- not reconstructable from the standard character_action /
-- character_spell tables. Drop manually if you want a clean wipe:
--   DROP TABLE IF EXISTS `custom_dualspec_talent`;
--   DROP TABLE IF EXISTS `custom_dualspec_talent_name`;
--   DROP TABLE IF EXISTS `custom_dualspec_action`;
--   DROP TABLE IF EXISTS `custom_dualspec_characters`;

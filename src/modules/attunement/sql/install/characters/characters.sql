-- ============================================================
-- Attunement: per-player option storage
-- ============================================================
-- Generic key/value table so future opt-ins can slot in without a
-- schema change. Today the only key is 'xp_rate' (FLOAT).
-- ============================================================

CREATE TABLE IF NOT EXISTS `custom_attunement_player_config` (
  `guid` int(11) unsigned NOT NULL COMMENT 'Character GUID',
  `option_key` varchar(64) NOT NULL COMMENT 'Option name (e.g. xp_rate)',
  `value` float NOT NULL DEFAULT 0 COMMENT 'Option value',
  `updated_at` datetime DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`guid`, `option_key`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin COMMENT='Attunement per-player options';

-- ============================================================
-- Attunement: account-level boost ledger
-- ============================================================
-- The level-60 boost is one-time PER ACCOUNT, not per character.
-- This table records which account already consumed its boost so
-- the gossip option is hidden for every other character on the
-- same account, even after the boosted character is deleted.
--
-- boosted_guid + boosted_name are stored for audit only — they are
-- not used by the runtime gate (the PK existing is enough).
-- ============================================================

CREATE TABLE IF NOT EXISTS `custom_attunement_account_boost` (
  `account_id` int(11) unsigned NOT NULL COMMENT 'Account identifier (one boost per account)',
  `boosted_guid` int(11) unsigned NOT NULL COMMENT 'GUID of the character that consumed the boost',
  `boosted_name` varchar(12) NOT NULL DEFAULT '' COMMENT 'Character name at boost time (audit only — may go stale on rename/delete)',
  `boosted_at` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'When the boost was used',
  PRIMARY KEY (`account_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin COMMENT='Attunement one-boost-per-account ledger';

-- ============================================================
-- Hardcore: loot drops, graves, per-player challenge state, death log
-- (amalgamated from the former cmangos-hardcore module)
-- ============================================================

DROP TABLE IF EXISTS `custom_hardcore_loot_gameobjects`;
CREATE TABLE `custom_hardcore_loot_gameobjects` (
  `id` int(11) unsigned NOT NULL,
  `player` int(11) unsigned NOT NULL COMMENT 'Player identifier',
  `loot_id` int(11) unsigned NOT NULL COMMENT 'The loot group this gameobject is part of',
  `loot_table` int(11) unsigned NOT NULL COMMENT 'custom_hardcore_loot_tables identifier',
  `money` int(11) unsigned NOT NULL DEFAULT '0',
  `position_x` float NOT NULL DEFAULT '0',
  `position_y` float NOT NULL DEFAULT '0',
  `position_z` float NOT NULL DEFAULT '0',
  `orientation` float NOT NULL DEFAULT '0',
  `map` int(11) NOT NULL DEFAULT '0' COMMENT 'Map identifier',
  `phase_mask` int(11) NOT NULL DEFAULT '0' COMMENT 'Phase mask identifier',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;

DROP TABLE IF EXISTS `custom_hardcore_loot_tables`;
CREATE TABLE `custom_hardcore_loot_tables` (
  `id` int(11) unsigned NOT NULL,
  `item` int(11) unsigned NOT NULL COMMENT 'Item identifier',
  `amount` tinyint(3) unsigned NOT NULL DEFAULT '1' COMMENT 'Amount of items',
  `random_property_id` smallint(5) NOT NULL DEFAULT '0' COMMENT 'The property of the item (e.g. ... of the Hawk, ... of the Monkey)',
  `durability` int(5) unsigned NOT NULL DEFAULT '0',
  `enchantments` text,
  PRIMARY KEY (`id`, `item`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;

DROP TABLE IF EXISTS `custom_hardcore_grave_gameobjects`;
CREATE TABLE `custom_hardcore_grave_gameobjects` (
  `id` int(11) unsigned NOT NULL,
  `player` int(11) unsigned NOT NULL COMMENT 'Player identifier',
  `gameobject_template` int(11) unsigned NOT NULL COMMENT 'gameobject_template entry',
  `position_x` float NOT NULL DEFAULT '0',
  `position_y` float NOT NULL DEFAULT '0',
  `position_z` float NOT NULL DEFAULT '0',
  `orientation` float NOT NULL DEFAULT '0',
  `map` int(11) NOT NULL DEFAULT '0' COMMENT 'Map identifier',
  `phase_mask` int(11) NOT NULL DEFAULT '0' COMMENT 'Phase mask identifier',
  PRIMARY KEY (`id`, `player`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;

-- xp_rate column dropped relative to the legacy hardcore schema —
-- Attunement owns per-player XP rate now (custom_attunement_player_config).
DROP TABLE IF EXISTS `custom_hardcore_player_config`;
CREATE TABLE `custom_hardcore_player_config` (
  `id` int(11) unsigned NOT NULL,
  `revive_disabled` boolean DEFAULT FALSE,
  `drop_loot_on_death` boolean DEFAULT FALSE,
  `lose_xp_on_death` boolean DEFAULT FALSE,
  `pvp_disabled` boolean DEFAULT FALSE,
  `self_found` boolean DEFAULT FALSE,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;

DROP TABLE IF EXISTS `custom_hardcore_player_deathlog`;
CREATE TABLE `custom_hardcore_player_deathlog` (
  `id` int(11) unsigned NOT NULL AUTO_INCREMENT,
  `player` int(11) unsigned NOT NULL,
  `account` int(11) unsigned NOT NULL,
  `name` char(100) NOT NULL,
  `level` int(11) NOT NULL,
  `zone` int(11) unsigned NOT NULL,
  `area` int(11) unsigned NOT NULL,
  `map` int(11) unsigned NOT NULL,
  `killer` int(11) unsigned NOT NULL,
  `killer_name` char(100) NOT NULL,
  `reason` int(11) unsigned NOT NULL,
  `date` datetime NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin;

-- ============================================================
-- Dualspec: per-player talent / action / name / status tables
-- (amalgamated from the former cmangos-dualspec module)
-- ============================================================
-- IMPORTANT: these tables MAY ALREADY EXIST on live with data
-- (e.g. Emberstone live ran the dualspec module for a long time
-- and every existing character has rows here). All statements
-- are CREATE TABLE IF NOT EXISTS + idempotent backfills so this
-- script is safe to re-apply.
-- ============================================================

CREATE TABLE IF NOT EXISTS `custom_dualspec_talent` (
  `guid` int(11) unsigned NOT NULL DEFAULT '0',
  `spell` int(11) unsigned NOT NULL DEFAULT '0',
  `spec` tinyint(3) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`spell`,`spec`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `custom_dualspec_talent_name` (
  `guid` int(11) unsigned NOT NULL DEFAULT '0',
  `spec` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `name` varchar(255) NOT NULL DEFAULT '',
  PRIMARY KEY (`guid`,`spec`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `custom_dualspec_action` (
  `guid` int(11) unsigned NOT NULL DEFAULT '0',
  `button` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `action` int(11) unsigned NOT NULL DEFAULT '0',
  `type` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `spec` tinyint(3) UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`,`spec`,`button`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Backfill spec-0 action bar from character_action for any character
-- that doesn't yet have one. Idempotent — the PK collision swallows
-- existing rows.
INSERT IGNORE INTO `custom_dualspec_action` (`guid`, `spec`, `button`, `action`, `type`)
  SELECT `guid`, 0 AS `spec`, `button`, `action`, `type` FROM `character_action`;

CREATE TABLE IF NOT EXISTS `custom_dualspec_characters` (
  `guid` int(11) unsigned NOT NULL DEFAULT '0',
  `spec_count` TINYINT(3) UNSIGNED NOT NULL DEFAULT '1',
  `active_spec` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;

-- Backfill a row per character so OnPreLoadFromDB can read defaults.
INSERT IGNORE INTO `custom_dualspec_characters` (`guid`)
  SELECT `guid` FROM `characters`;

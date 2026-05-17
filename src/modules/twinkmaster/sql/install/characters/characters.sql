CREATE TABLE IF NOT EXISTS `custom_twinkmaster_player_config` (
  `guid` int(11) unsigned NOT NULL COMMENT 'Character GUID',
  `xp_locked` boolean DEFAULT FALSE COMMENT 'Whether XP gain is blocked',
  `level_set` boolean DEFAULT FALSE COMMENT 'Whether level was set by Twink Master',
  `created_at` datetime DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_bin COMMENT='Twink Master player configuration';

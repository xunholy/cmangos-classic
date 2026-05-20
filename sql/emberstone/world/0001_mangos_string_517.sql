-- mangos_string row 517 (LANG_GO_MIXED_LIST_CHAT) drifted away from what
-- the C++ call site at Level2.cpp expects. The drifted format had an extra
-- "%d" in the Hgameobject:%d:%d link and put SpawnGroup outside the |h|r
-- link tags, giving 11 placeholders against 10 args. vsnprintf then walked
-- past the supplied varargs and ran strlen on uninitialised stack bits —
-- repeatable SIGSEGV on any .gobject near / delete / target from a GM
-- near gameobjects.
--
-- Three cores on 2026-05-18 (12:56, 12:58, 13:02) all had identical
-- stacks: __strlen_avx2 → vfprintf → PSendSysMessage.
--
-- Canonical format matches upstream sql/base/mangos.sql:3865. The
-- corresponding C++ fix (commit 180ec272) hardcodes the format string at
-- both call sites so this row no longer matters to the worldserver — keep
-- this migration anyway so any future DB rebuild starts from a sane row.
UPDATE `mangos_string`
   SET `content_default` = '%d%s, Entry %d - |cffffffff|Hgameobject:%d|h[%s X:%f Y:%f Z:%f MapId:%d]SpawnGroup:%u|h|r'
 WHERE `entry` = 517;

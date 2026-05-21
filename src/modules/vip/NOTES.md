# Module: vip

Originated in this fork. There is no separate upstream repository to track — changes land directly on `main` here.

## Why it lives in-tree

The module piggybacks on a vanilla `spell_template` row (`18282 Dummy Spell`) and registers an `OnCast` hook on it. The technique is intentionally cmangos-specific (relies on the modules framework's hook surface and on the fact that the chosen spell ID exists in the 1.12 client DBC). A separate repo would just add ceremony.

## If you fork

Drop the module by:

1. Removing the `src/modules/vip/` directory.
2. Removing the `VIP=in-tree` line from `src/modules/modules/modules.conf`.
3. Building without `-DBUILD_MODULE_VIP=ON`.

Optional DB cleanup if you previously enabled the module:

* Players still flagged as VIP will keep the flag (harmless without the module loaded — nothing reads it).
* If you want the master spell unlearned from existing VIPs, run `.unlearn 18282` per character, or `DELETE FROM character_spell WHERE spell = 18282` against the characters DB before next login.

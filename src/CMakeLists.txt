#
# This file is part of the CMaNGOS Project. See AUTHORS file for Copyright information
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

if(BUILD_GAME_SERVER OR BUILD_LOGIN_SERVER OR BUILD_EXTRACTORS)
  add_subdirectory(framework)
  add_subdirectory(shared)
endif()

# Playerbots module — vendored in src/modules/PlayerBots/. Provenance
# (upstream SHA + import date) is recorded in PlayerBots/NOTES.md.
# Previously fetched at configure time from cmangos/playerbots@master via
# FetchContent; switched to a vendored copy so this fork can carry local
# patches (e.g. the EquippedItemInventoryTypeMask guard in
# UpdateGearAction::EnchantItem) and so two builds at the same fork SHA
# produce the same binary.
if(BUILD_PLAYERBOTS)
  if(NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/modules/PlayerBots/CMakeLists.txt)
    message(FATAL_ERROR
      "BUILD_PLAYERBOTS=ON but src/modules/PlayerBots/ is missing. "
      "PlayerBots is now vendored in-tree; if you stripped it, re-import "
      "from cmangos/playerbots (see src/modules/PlayerBots/NOTES.md).")
  endif()
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/modules/PlayerBots)
endif()

if(BUILD_GAME_SERVER)
  add_subdirectory(game)
  add_subdirectory(mangosd)
endif()

if(BUILD_LOGIN_SERVER)
  add_subdirectory(realmd)
endif()

# Emberstone modules — vendored under src/modules/. The canonical list and
# per-module BUILD_MODULE_<NAME> options live in cmake/options.cmake. The
# 'modules' framework library is built whenever any module is enabled — it
# provides the Module base class and ModuleMgr that each module inherits from.
set(_any_module_enabled FALSE)
foreach(mod ${EMBERSTONE_MODULES})
  string(TOUPPER ${mod} _MOD)
  if(BUILD_MODULE_${_MOD})
    add_subdirectory(modules/${mod})
    set(_any_module_enabled TRUE)
  endif()
endforeach()
if(_any_module_enabled)
  add_subdirectory(modules/modules)
endif()

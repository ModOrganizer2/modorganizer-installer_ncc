/*	Better Oblivion Sorting Software

  A "one-click" program for users that quickly optimises and avoids
  detrimental conflicts in their TES IV: Oblivion, Nehrim - At Fate's Edge,
  TES V: Skyrim, Fallout 3 and Fallout: New Vegas mod load orders.

    Copyright (C) 2011    BOSS Development Team.

  This file is part of Better Oblivion Sorting Software.

    Better Oblivion Sorting Software is free software: you can redistribute
  it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of
  the License, or (at your option) any later version.

    Better Oblivion Sorting Software is distributed in the hope that it will
  be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Better Oblivion Sorting Software.  If not, see
  <http://www.gnu.org/licenses/>.

  $Revision: 2188 $, $Date: 2011-01-20 10:05:16 +0000 (Thu, 20 Jan 2011) $
*/

// Modifications by Sebastian Herbord

#ifndef BOSSDUMMY_H
#define BOSSDUMMY_H


#include <cstdint>


#ifdef __cplusplus
extern "C"
{
#endif

////////////////////////
// Types
////////////////////////

typedef struct {
  uint32_t dummy;
} boss_db_int, *boss_db;

// BashTag structure gives the Unique ID number (UID) for each Bash Tag and
// the corresponding Tag name string.
typedef struct {
    uint32_t id;
    const uint8_t *name;  // don't use char for utf-8 since char can be signed
} BashTag;


__declspec(dllexport) bool IsCompatibleVersion(const uint32_t bossVersionMajor, const uint32_t bossVersionMinor, const uint32_t bossVersionPatch);
__declspec(dllexport) uint32_t GetVersionString(const uint8_t **bossVersionStr);
__declspec(dllexport) uint32_t GetLastErrorDetails(const uint8_t **err);
__declspec(dllexport) uint32_t CreateBossDb(boss_db *db);
__declspec(dllexport) void DestroyBossDb(boss_db db);
__declspec(dllexport) uint32_t Load(boss_db db, const uint8_t *masterlistPath, const uint8_t *userlistPath, const uint8_t *dataPath, const uint32_t clientGame);
__declspec(dllexport) uint32_t EvalConditionals(boss_db db, const uint8_t *dataPath);
__declspec(dllexport) uint32_t UpdateMasterlist(const uint32_t clientGame, const uint8_t  masterlistPath);
__declspec(dllexport) uint32_t SortMods(boss_db db);
__declspec(dllexport) uint32_t GetLoadOrder(boss_db db, uint8_t ***plugins, size_t *numPlugins);
__declspec(dllexport) uint32_t SetLoadOrder(boss_db db, uint8_t **plugins, const size_t numPlugins);
__declspec(dllexport) uint32_t GetActivePlugins(boss_db db, uint8_t ***plugins, size_t *numPlugins);
__declspec(dllexport) uint32_t SetActivePlugins(boss_db db, uint8_t **plugins, const size_t numPlugins);

__declspec(dllexport) uint32_t GetPluginLoadOrder(boss_db db, const uint8_t *plugin, size_t *index);
__declspec(dllexport) uint32_t SetPluginLoadOrder(boss_db db, const uint8_t *plugin, size_t index);
__declspec(dllexport) uint32_t GetIndexedPlugin(boss_db db, const size_t index, uint8_t **plugin);
__declspec(dllexport) uint32_t SetPluginActive(boss_db db, const uint8_t *plugin, const bool active);
__declspec(dllexport) uint32_t IsPluginActive(boss_db db, const uint8_t *plugin, bool *isActive);
__declspec(dllexport) uint32_t IsPluginMaster(boss_db db, const uint8_t *plugin, bool * sMaster);


#ifdef __cplusplus
}
#endif

#endif // BOSSDUMMY_H

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

#include "bossdummy.h"


__declspec(dllexport) bool IsCompatibleVersion(const uint32_t, const uint32_t, const uint32_t)
{
  return true;
}


__declspec(dllexport) uint32_t GetLastErrorDetails(const uint8_t **err)
{
  //Check for valid args.
  if (err == 0)
    return 32;

  *err = reinterpret_cast<const uint8_t *>("fake_boss");
  return 0;
}

__declspec(dllexport) uint32_t GetVersionString(const uint8_t **bossVersionStr)
{
  //Check for valid args.
  if (bossVersionStr == 0)
    return 32;

  *bossVersionStr = reinterpret_cast<const uint8_t *>("fake_boss");
  return 0;
}

__declspec(dllexport) uint32_t CreateBossDb(boss_db *db)
{
  *db = new boss_db_int;
  return 0;
}

__declspec(dllexport) void DestroyBossDb(boss_db db)
{
  delete db;
}

__declspec(dllexport) uint32_t Load(boss_db, const uint8_t*, const uint8_t*, const uint8_t*, const uint32_t)
{
  return 0;
}


__declspec(dllexport) uint32_t EvalConditionals(boss_db, const uint8_t*)
{
  return 0;
}

__declspec(dllexport) uint32_t UpdateMasterlist(const uint32_t, const uint8_t)
{
  return 0;
}

__declspec(dllexport) uint32_t SortMods(boss_db)
{
  return 0;
}

__declspec(dllexport) uint32_t GetLoadOrder(boss_db , uint8_t ***plugins, size_t *numPlugins)
{
  *numPlugins = 0;
  *plugins = 0;
  return 0;
}


__declspec(dllexport) uint32_t SetLoadOrder(boss_db , uint8_t**, const size_t)
{
  return 0;
}


__declspec(dllexport) uint32_t GetActivePlugins(boss_db , uint8_t ***plugins, size_t *numPlugins)
{
  *numPlugins = 0;
  *plugins = 0;
  return 0;
}

__declspec(dllexport) uint32_t SetActivePlugins(boss_db , uint8_t**, const size_t)
{
  return 0;
}


__declspec(dllexport) uint32_t GetPluginLoadOrder(boss_db, const uint8_t*, size_t *index)
{
  *index = 0;
  return 0;
}

__declspec(dllexport) uint32_t SetPluginLoadOrder(boss_db, const uint8_t*, size_t)
{
  return 0;
}

__declspec(dllexport) uint32_t GetIndexedPlugin(boss_db, const size_t, uint8_t**)
{
  return 32;
}

__declspec(dllexport) uint32_t SetPluginActive(boss_db, const uint8_t*, const bool)
{
  return 0;
}

__declspec(dllexport) uint32_t IsPluginActive(boss_db, const uint8_t*, bool *isActive)
{
  *isActive = false;
  return 0;
}

__declspec(dllexport) uint32_t IsPluginMaster(boss_db, const uint8_t*, bool *isMaster)
{
  *isMaster = false;
  return 0;
}


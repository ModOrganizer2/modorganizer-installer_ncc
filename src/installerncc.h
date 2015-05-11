/*
Copyright (C) 2012 Sebastian Herbord. All rights reserved.

This file is part of Mod Organizer.

Mod Organizer is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Mod Organizer is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Mod Organizer.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef INSTALLERNCC_H
#define INSTALLERNCC_H


#include <iplugininstallercustom.h>
#include <iplugindiagnose.h>
#include <igameinfo.h>
#include <QXmlStreamReader>


class InstallerNCC : public MOBase::IPluginInstallerCustom, public MOBase::IPluginDiagnose
{
  Q_OBJECT
  Q_INTERFACES(MOBase::IPlugin MOBase::IPluginInstaller MOBase::IPluginInstallerCustom MOBase::IPluginDiagnose)
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
  Q_PLUGIN_METADATA(IID "org.tannin.InstallerNCC" FILE "installerncc.json")
#endif

public:

  InstallerNCC();

public: // IPlugin

  virtual bool init(MOBase::IOrganizer *moInfo);
  virtual QString name() const;
  virtual QString author() const;
  virtual QString description() const;
  virtual MOBase::VersionInfo version() const;
  virtual bool isActive() const;
  virtual QList<MOBase::PluginSetting> settings() const;

public: // IPluginInstaller

  virtual unsigned int priority() const;
  virtual bool isManualInstaller() const;
  virtual bool isArchiveSupported(const MOBase::DirectoryTree &tree) const;

public: // IPluginInstallerCustom

  virtual std::set<QString> supportedExtensions() const;
  virtual bool isArchiveSupported(const QString &archiveName) const;
  virtual EInstallResult install(MOBase::GuessedValue<QString> &modName, const QString &archiveName,
                                 const QString &version, int modID);

public: // IPluginDiagnose

  virtual std::vector<unsigned int> activeProblems() const;
  virtual QString shortDescription(unsigned int key) const;
  virtual QString fullDescription(unsigned int key) const;
  virtual bool hasGuidedFix(unsigned int key) const;
  virtual void startGuidedFix(unsigned int key) const;

private:

  const wchar_t *gameShortName(MOBase::IGameInfo::Type gameType) const;
  bool isNCCInstalled() const;
  bool isNCCCompatible() const;
  bool isDotNetInstalled() const;
  QString nccPath() const;
  std::wstring getSEVersion();

  IPluginInstaller::EInstallResult invokeNCC(MOBase::IModInterface *modInterface, const QString &archiveName);

private:

  static const unsigned int COMPATIBLE_MAJOR_VERSION = 0x03;

  static const unsigned int PROBLEM_NCCMISSING = 1;
  static const unsigned int PROBLEM_NCCINCOMPATIBLE = 2;
  static const unsigned int PROBLEM_DOTNETINSTALLED = 3;

private:

  MOBase::IOrganizer *m_MOInfo;

};

#endif // INSTALLERNCC_H

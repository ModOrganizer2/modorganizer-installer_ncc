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

#include "installerncc.h"

#include <utility.h>
#include <report.h>
#include <scopeguard.h>

#include <boost/assign.hpp>
#include <boost/scoped_array.hpp>

#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QCoreApplication>
#include <QProgressDialog>
#include <QtPlugin>


using namespace MOBase;


InstallerNCC::InstallerNCC()
{
}

bool InstallerNCC::init(IOrganizer *moInfo)
{
  m_MOInfo = moInfo;
  return true;
}

QString InstallerNCC::name() const
{
  return tr("Fomod Installer (external)");
}

QString InstallerNCC::author() const
{
  return "Tannin";
}

QString InstallerNCC::description() const
{
  return tr("Installer for all fomod archives. Requires NCC to be installed");
}

VersionInfo InstallerNCC::version() const
{
  return VersionInfo(1, 0, 0, VersionInfo::RELEASE_FINAL);
}

bool InstallerNCC::isActive() const
{
  return isNCCInstalled();
}

QList<PluginSetting> InstallerNCC::settings() const
{
  return QList<PluginSetting>();
}

unsigned int InstallerNCC::priority() const
{
  return 100;
}


bool InstallerNCC::isManualInstaller() const
{
  return false;
}


std::set<QString> InstallerNCC::supportedExtensions() const
{
  return boost::assign::list_of("zip")("7z")("rar")("fomod");
}


bool InstallerNCC::isArchiveSupported(const DirectoryTree &tree) const
{
  for (DirectoryTree::const_node_iterator iter = tree.nodesBegin();
       iter != tree.nodesEnd(); ++iter) {
    const QString &dirName = (*iter)->getData().name;
    if (dirName.compare("fomod", Qt::CaseInsensitive) == 0) {
      return true;
    }
  }

  // recurse into single directories
  if ((tree.numNodes() == 1) && (tree.numLeafs() == 0)) {
    DirectoryTree::Node *node = *tree.nodesBegin();
    return isArchiveSupported(*node);
  } else {
    return false;
  }
}


bool InstallerNCC::isArchiveSupported(const QString &archiveName) const
{
  return archiveName.endsWith(".fomod", Qt::CaseInsensitive) ||
         archiveName.endsWith(".omod", Qt::CaseInsensitive);
}


const wchar_t *InstallerNCC::gameShortName(IGameInfo::Type gameType) const
{
  switch (gameType) {
    case IGameInfo::TYPE_OBLIVION: return L"Oblivion";
    case IGameInfo::TYPE_SKYRIM: return L"Skyrim";
    case IGameInfo::TYPE_FALLOUT3: return L"Fallout3";
    case IGameInfo::TYPE_FALLOUTNV: return L"FalloutNV";
    default: throw IncompatibilityException(tr("game not supported by NCC Installer"));
  }
}


IPluginInstaller::EInstallResult InstallerNCC::install(GuessedValue<QString> &modName, const QString &archiveName)
{
  IModInterface *modInterface = m_MOInfo->createMod(modName);

  wchar_t binary[MAX_PATH];
  wchar_t parameters[1024]; // maximum: 2xMAX_PATH + approx 20 characters
  wchar_t currentDirectory[MAX_PATH];

  _snwprintf(binary, MAX_PATH, L"%ls", ToWString(QDir::toNativeSeparators(nccPath())).c_str());
  _snwprintf(parameters, 1024, L"-g %ls -p \"%ls\" -i \"%ls\" \"%ls\"",
             gameShortName(m_MOInfo->gameInfo().type()),
             ToWString(QDir::toNativeSeparators(QDir::cleanPath(m_MOInfo->profilePath() + "/plugins.txt"))).c_str(),
             ToWString(QDir::toNativeSeparators(archiveName)).c_str(),
             ToWString(QDir::toNativeSeparators(modInterface->absolutePath())).c_str());
  _snwprintf(currentDirectory, MAX_PATH, L"%ls", ToWString(QFileInfo(nccPath()).absolutePath()).c_str());

  // NCC assumes the installation directory is the game directory and may try to access the binary to determine version information
  QString binaryDestination = modInterface->absolutePath() + "/" + m_MOInfo->gameInfo().binaryName();
  QFile::copy(m_MOInfo->gameInfo().path() + "/" + m_MOInfo->gameInfo().binaryName(), binaryDestination);
  ON_BLOCK_EXIT([&binaryDestination] { QFile::remove(binaryDestination); } );

  SHELLEXECUTEINFOW execInfo = {0};
  execInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
  execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
  execInfo.hwnd = NULL;
  execInfo.lpVerb = L"open";
  execInfo.lpFile = binary;
  execInfo.lpParameters = parameters;
  execInfo.lpDirectory = currentDirectory;
  execInfo.nShow = SW_SHOW;

  if (!::ShellExecuteExW(&execInfo)) {
    reportError(tr("failed to start %1").arg(nccPath()));
    return RESULT_FAILED;
  }

  QProgressDialog busyDialog(tr("Running external installer.\nNote: This installer will not be aware of other installed mods!"),
                             tr("Force Close"), 0, 0, parentWidget());
  busyDialog.setWindowModality(Qt::WindowModal);
  bool confirmCancel = false;
  busyDialog.show();
  bool finished = false;
  while (true) {
    QCoreApplication::processEvents();
    DWORD res = ::WaitForSingleObject(execInfo.hProcess, 100);
    if (res == WAIT_OBJECT_0) {
      finished = true;
      break;
    } else if ((busyDialog.wasCanceled()) || (res != WAIT_TIMEOUT)) {
      if (!confirmCancel) {
        confirmCancel = true;
        busyDialog.hide();
        busyDialog.reset();
        busyDialog.show();
        busyDialog.setCancelButtonText(tr("Confirm"));
      } else {
        break;
      }
    }
  }

  if (!finished) {
    ::TerminateProcess(execInfo.hProcess, 1);
    return RESULT_FAILED;
  }

  DWORD exitCode = 128;
  ::GetExitCodeProcess(execInfo.hProcess, &exitCode);

  ::CloseHandle(execInfo.hProcess);

  if ((exitCode == 0) || (exitCode == 10)) { // 0 = success, 10 = incomplete installation
    bool errorOccured = false;
    { // move all installed files from the data directory one directory up
      QDir targetDir(modInterface->absolutePath());
      QDirIterator dirIter(targetDir.absoluteFilePath("Data"), QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);

      bool hasFiles = false;

      while (dirIter.hasNext()) {
        dirIter.next();
        QFileInfo fileInfo = dirIter.fileInfo();
        QString newName = targetDir.absoluteFilePath(fileInfo.fileName());
        if (fileInfo.isFile() && QFile::exists(newName)) {
          if (!QFile::remove(newName)) {
            qCritical("failed to overwrite %s", qPrintable(newName));
            errorOccured = true;
          }
        } // if it's a directory and the target exists that isn't really a problem

        if (!QFile::rename(fileInfo.absoluteFilePath(), newName)) {
          // moving doesn't work when merging
          if (!copyDir(fileInfo.absoluteFilePath(), newName, true)) {
            qCritical("failed to move %s to %s", qPrintable(fileInfo.absoluteFilePath()), qPrintable(newName));
            errorOccured = true;
          }
        }
        hasFiles = true;
      }
      // recognition of canceled installation in the external installer is broken so we assume the installation was
      // canceled if no files were installed
      if (!hasFiles) {
        exitCode = 11;
      }
    }

    QString dataDir = modInterface->absolutePath() + "/Data";
    if (!removeDir(dataDir)) {
      qCritical("failed to remove data directory from %s", qPrintable(dataDir));
      errorOccured = true;
    }
    if (errorOccured) {
      reportError(tr("Finalization of the installation failed. The mod may or may not work correctly. See mo_interface.log for details"));
    }
  } else if (exitCode != 11) { // 11 = manually canceled
    reportError(tr("installation failed (errorcode %1)").arg(exitCode));
  }

  if ((exitCode == 0) || (exitCode == 10)) {
    return RESULT_SUCCESS;
  } else {
    if (!m_MOInfo->removeMod(modInterface)) {
      qCritical("failed to remove empty mod %s", qPrintable(modInterface->absolutePath()));
    }
    return RESULT_FAILED;
  }
}

bool InstallerNCC::isNCCInstalled() const
{
  return QFile::exists(nccPath());
}

bool InstallerNCC::isNCCCompatible() const
{
  std::wstring nccNameW = ToWString(QDir::toNativeSeparators(nccPath()));

  DWORD size = ::GetFileVersionInfoSizeW(nccNameW.c_str(), NULL);
  if (size == 0) {
    qCritical("failed to determine file version info size");
    return false;
  }

  boost::scoped_array<char> buffer(new char[size]);

  if (!::GetFileVersionInfoW(nccNameW.c_str(), 0UL, size, buffer.get())) {
    qCritical("failed to determine file version info");
    return false;
  }

  void *versionInfoPtr = NULL;
  UINT versionInfoLength = 0;
  if (!::VerQueryValue(buffer.get(), L"\\", &versionInfoPtr, &versionInfoLength)) {
    qCritical("failed to determine file version");
    return false;
  }

  VS_FIXEDFILEINFO* temp = (VS_FIXEDFILEINFO*)versionInfoPtr;

  return (temp->dwFileVersionMS & 0xFFFFFF) == COMPATIBLE_MAJOR_VERSION;
}

QString InstallerNCC::nccPath() const
{
  return QCoreApplication::applicationDirPath() + "/NCC/NexusClientCLI.exe";
}


std::vector<unsigned int> InstallerNCC::activeProblems() const
{
  std::vector<unsigned int> result;

  if (!isNCCInstalled()) {
    result.push_back(PROBLEM_NCCMISSING);
  } else if (!isNCCCompatible()) {
    result.push_back(PROBLEM_NCCINCOMPATIBLE);
  }

  return result;
}

QString InstallerNCC::shortDescription(unsigned int key) const
{
  switch (key) {
    case PROBLEM_NCCMISSING:
      return tr("NCC is not installed. You won't be able to install some scripted mod-installers.");
    case PROBLEM_NCCINCOMPATIBLE:
      return tr("NCC Vrsion may be incompatible.");
    default:
      throw MyException(tr("invalid problem key %1").arg(key));
  }
}

QString InstallerNCC::fullDescription(unsigned int key) const
{
  switch (key) {
    case PROBLEM_NCCMISSING:
      return tr("NCC is not installed. You won't be able to install some scripted mod-installers."
                "Get NCC from <a href=\"http://skyrim.nexusmods.com/downloads/file.php?id=1334\">the MO page on nexus</a>");
    case PROBLEM_NCCINCOMPATIBLE:
      return tr("NCC version may be incompatible, expected version %1.x.x.x.").arg(COMPATIBLE_MAJOR_VERSION);
    default:
      throw MyException(tr("invalid problem key %1").arg(key));
  }
}

bool InstallerNCC::hasGuidedFix(unsigned int) const
{
  return false;
}

void InstallerNCC::startGuidedFix(unsigned int key) const
{
  throw MyException(tr("invalid problem key %1").arg(key));
}


Q_EXPORT_PLUGIN2(installerNCC, InstallerNCC)

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
#include <QSettings>
#include <QtPlugin>


using namespace MOBase;


InstallerNCC::InstallerNCC()
  : m_MOInfo(NULL)
{
}

bool InstallerNCC::init(IOrganizer *moInfo)
{
  m_MOInfo = moInfo;
  return true;
}

QString InstallerNCC::name() const
{
  return "Fomod Installer (external)";
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
  return VersionInfo(1, 0, 1, VersionInfo::RELEASE_FINAL);
}

bool InstallerNCC::isActive() const
{
  return isDotNetInstalled() && isNCCInstalled();
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
      for (DirectoryTree::const_leaf_iterator fileIter = (*iter)->leafsBegin();
           fileIter != (*iter)->leafsEnd(); ++fileIter) {
        if ((fileIter->getName().compare("ModuleConfig.xml", Qt::CaseInsensitive) == 0) ||
            (fileIter->getName().compare("script.cs", Qt::CaseInsensitive) == 0)) {
          return true;
        }
      }
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


// http://www.shloemi.com/2012/09/solved-setforegroundwindow-win32-api-not-always-works/
static void ForceWindowVisible(HWND hwnd)
{
  DWORD foregroundThread = ::GetWindowThreadProcessId(::GetForegroundWindow(), NULL);
  DWORD currentThread = ::GetCurrentThreadId();

  if (foregroundThread != currentThread) {
    ::AttachThreadInput(foregroundThread, currentThread, true);
    ::BringWindowToTop(hwnd);
    ::ShowWindow(hwnd, SW_SHOW);
    ::AttachThreadInput(foregroundThread, currentThread, false);
  } else {
    ::BringWindowToTop(hwnd);
    ::ShowWindow(hwnd, SW_SHOW);
  }
}


static BOOL CALLBACK BringToFront(HWND hwnd, LPARAM lParam)
{
  DWORD procid;

  GetWindowThreadProcessId(hwnd, &procid);
  ::SetLastError(ERROR_HANDLE_EOF);
  if ((procid == static_cast<DWORD>(lParam)) && IsWindowVisible(hwnd)) {
    ForceWindowVisible(hwnd);
    ::SetLastError(NOERROR);
    return false;
  }
  return TRUE;
}


IPluginInstaller::EInstallResult InstallerNCC::invokeNCC(IModInterface *modInterface, const QString &archiveName)
{
  wchar_t binary[MAX_PATH];
  wchar_t parameters[1024]; // maximum: 2xMAX_PATH + approx 20 characters
  wchar_t currentDirectory[MAX_PATH];
#pragma warning( push )
#pragma warning( disable : 4996 )
  _snwprintf(binary, MAX_PATH, L"%ls", ToWString(QDir::toNativeSeparators(nccPath())).c_str());
  _snwprintf(parameters, 1024, L"-g %ls -p \"%ls\" -i \"%ls\" \"%ls\"",
             gameShortName(m_MOInfo->gameInfo().type()),
             ToWString(QDir::toNativeSeparators(QDir::cleanPath(m_MOInfo->profilePath() + "/plugins.txt"))).c_str(),
             ToWString(QDir::toNativeSeparators(archiveName)).c_str(),
             ToWString(QDir::toNativeSeparators(modInterface->absolutePath())).c_str());
  _snwprintf(currentDirectory, MAX_PATH, L"%ls", ToWString(QFileInfo(nccPath()).absolutePath()).c_str());
#pragma warning( pop )

  // NCC assumes the installation directory is the game directory and may try to access the binary to determine version information
  QStringList copiedFiles;
  QStringList patterns;
  patterns.append(QDir::fromNativeSeparators(m_MOInfo->gameInfo().binaryName()));
  patterns.append("*se_loader.exe");
  QDirIterator iter(QDir::fromNativeSeparators(m_MOInfo->gameInfo().path()), patterns);
  QDir modDir(modInterface->absolutePath());
  while (iter.hasNext()) {
    iter.next();
    QString destination = modDir.absoluteFilePath(iter.fileInfo().fileName());
    if (QFile::copy(iter.fileInfo().absoluteFilePath(), destination)) {
      copiedFiles.append(destination);
    }
  }
  ON_BLOCK_EXIT([&copiedFiles] {
    if (!shellDelete(copiedFiles)) {
      reportError(QString("Failed to clean up after NCC installation, you may find some files "
                     "unrelated to the mod in the newly created mod directory: %1").arg(windowsErrorString(::GetLastError())));
    }
  });

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

  QProgressDialog busyDialog(tr("Running external installer.\n"
                                "Note: This installer will not be aware of other installed mods!\n\n"
                                "Based on Nexus Mod Manager by Black Tree Gaming Ltd.\n"),
                             tr("Force Close"), 0, 0, parentWidget());
  busyDialog.setWindowFlags(busyDialog.windowFlags() & ~Qt::WindowContextHelpButtonHint);
  busyDialog.setWindowModality(Qt::WindowModal);
  bool confirmCancel = false;
  busyDialog.show();

  bool finished = false;
  DWORD procid = ::GetProcessId(execInfo.hProcess);
  bool inFront = false;
  while (!finished) {
    QCoreApplication::processEvents();
    if (!inFront) {
      if (!::EnumWindows(BringToFront, procid) && (::GetLastError() == NOERROR)) {
        qDebug("brought window to front");
        inFront = true;
      }
    }
    DWORD res = ::WaitForSingleObject(execInfo.hProcess, 100);
    if (res == WAIT_OBJECT_0) {
      finished = true;
    } else if (busyDialog.wasCanceled() || (res != WAIT_TIMEOUT)) {
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
        qDebug("no files in installed mod");
        exitCode = 11;
      }
    }

    QString dataDir = modInterface->absolutePath() + "/Data";
    if (!shellDelete(QStringList(dataDir), false, parentWidget())) {
      qCritical("failed to remove data directory from %s", qPrintable(dataDir));
      errorOccured = true;
    }
    if (errorOccured) {
      reportError(tr("Finalization of the installation failed. The mod may or may not work correctly. See mo_interface.log for details"));
    }
  } else if (exitCode != 11) { // 11 = manually canceled
    reportError(tr("installation failed (errorcode %1)").arg(exitCode));
  }

  return ((exitCode == 0) || (exitCode == 10)) ? RESULT_SUCCESS : RESULT_FAILED;
}


IPluginInstaller::EInstallResult InstallerNCC::install(GuessedValue<QString> &modName, const QString &archiveName,
                                                       const QString &version, int modID)
{
  IModInterface *modInterface = m_MOInfo->createMod(modName);
  if (modInterface == NULL) {
    return RESULT_CANCELED;
  }
  modInterface->setVersion(version);
  modInterface->setNexusID(modID);

  EInstallResult res = invokeNCC(modInterface, archiveName);

  if (res == RESULT_SUCCESS) {
    // post process mod directory
    QFile file(modInterface->absolutePath() + "/__installInfo.txt");
    if (file.open(QIODevice::ReadOnly)) {
      QStringList data = QString(file.readAll()).split("\n");
      file.close();
      QFile::remove(modInterface->absolutePath() + "/__installInfo.txt");
      if (data.count() == 3) {
        modName.update(data.at(0), GUESS_META);
        QString newName = modName;
        if ((QString::compare(modName, modInterface->name(), Qt::CaseInsensitive) != 0) &&
            (m_MOInfo->getMod(newName) == NULL)) {
          modInterface->setName(modName);
        }
        if (data.at(1).length() > 0) {
          modInterface->setVersion(data.at(1));
        }
        if (data.at(2).length() > 0) {
          modInterface->setNexusID(data.at(2).toInt());
        }
      }
    }

  } else {
    if (!modInterface->remove()) {
      qCritical("failed to remove empty mod %s", qPrintable(modInterface->absolutePath()));
    }
  }

  return res;
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

bool InstallerNCC::isDotNetInstalled() const
{
  return QSettings("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\NET Framework Setup\\NDP\\v3.5",
                   QSettings::NativeFormat).value("Install", 0) == 1;
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
  } else if (!isDotNetInstalled()) {
    result.push_back(PROBLEM_DOTNETINSTALLED);
  }

  return result;
}

QString InstallerNCC::shortDescription(unsigned int key) const
{
  switch (key) {
    case PROBLEM_NCCMISSING:
      return tr("NCC is not installed.");
    case PROBLEM_NCCINCOMPATIBLE:
      return tr("NCC Version may be incompatible.");
    case PROBLEM_DOTNETINSTALLED:
      return tr("dotNet is not installed or outdated.");
    default:
      throw MyException(tr("invalid problem key %1").arg(key));
  }
}

QString InstallerNCC::fullDescription(unsigned int key) const
{
  switch (key) {
    case PROBLEM_NCCMISSING:
      return tr("NCC is not installed. You won't be able to install some scripted mod-installers. "
                "Get NCC from <a href=\"http://www.nexusmods.com/skyrim/mods/1334\">the MO page on nexus</a>.");
    case PROBLEM_NCCINCOMPATIBLE:
      return tr("NCC version may be incompatible, expected version 0.%1.x.x.").arg(COMPATIBLE_MAJOR_VERSION);
    case PROBLEM_DOTNETINSTALLED: {
      QString dotNetUrl = "http://www.microsoft.com/en-us/download/details.aspx?id=17851";
      return tr("<li>dotNet is not installed or the wrong version. This is required to use NCC. "
                "Get it from here: <a href=\"%1\">%1</a></li>").arg(dotNetUrl);
    } break;
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

#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
Q_EXPORT_PLUGIN2(installerNCC, InstallerNCC)
#endif

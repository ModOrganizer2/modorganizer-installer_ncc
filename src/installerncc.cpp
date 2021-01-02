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
#include <imodinterface.h>
#include "iplugingame.h"
#include "scriptextender.h"
#include "nccinstalldialog.h"

#include <boost/assign.hpp>
#include <boost/scoped_array.hpp>
#include <boost/format.hpp>

#include <QFile>
#include <QDir>
#include <QDirIterator>
#include <QCoreApplication>
#include <QProgressDialog>
#include <QSettings>
#include <QtPlugin>
#include <QInputDialog>


using namespace MOBase;




VS_FIXEDFILEINFO getFileVersionInfo(const QString &path)
{
  std::wstring nameW = QDir::toNativeSeparators(path).toStdWString();

  DWORD size = ::GetFileVersionInfoSizeW(nameW.c_str(), NULL);
  if (size == 0) {
    throw std::runtime_error("failed to determine file version info size");
  }

  boost::scoped_array<char> buffer(new char[size]);

  if (!::GetFileVersionInfoW(nameW.c_str(), 0UL, size, buffer.get())) {
    throw std::runtime_error("failed to determine file version info");
  }

  void *versionInfoPtr = NULL;
  UINT versionInfoLength = 0;
  if (!::VerQueryValue(buffer.get(), L"\\", &versionInfoPtr, &versionInfoLength)) {
    throw std::runtime_error("failed to determine file version");
  }
  return *(VS_FIXEDFILEINFO*)versionInfoPtr;
}



InstallerNCC::InstallerNCC()
  : m_MOInfo(nullptr)
{
}

bool InstallerNCC::init(IOrganizer *moInfo)
{
  m_MOInfo = moInfo;
  pluginShouldWork = isDotNetInstalled() && isNCCInstalled();
  return true;
}

QString InstallerNCC::name() const
{
  return "Fomod Installer (external)";
}

QString InstallerNCC::localizedName() const
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
  return VersionInfo(1, 7, 0, VersionInfo::RELEASE_FINAL);
}

std::vector<std::shared_ptr<const MOBase::IPluginRequirement>> InstallerNCC::requirements() const
{
  return { Requirements::diagnose(this) };
}

QList<PluginSetting> InstallerNCC::settings() const
{
  return {};
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


bool InstallerNCC::isArchiveSupported(std::shared_ptr<const IFileTree> tree) const
{
  if (!pluginShouldWork) {
    return false;
  }

  if (tree->empty()) {
    return false;
  }

  for (auto entry : *tree) {
    if (entry->isDir() && entry->compare("fomod") == 0) {
      for (auto childEntry : *entry->astree()) {
        if (childEntry->compare("ModuleConfig.xml") == 0
          || childEntry->compare("script.cs") == 0) {
          return true;
        }
      }
    }
  }

  // If there is no subdirectory, invalid:
  if (!tree->at(0)->isDir()) {
    return false;
  }

  // If there is two or more directory, invalid:
  if (tree->size() > 1 && tree->at(1)->isDir()) {
    return false;
  }

  return isArchiveSupported(tree->at(0)->astree());
}


bool InstallerNCC::isArchiveSupported(const QString &archiveName) const
{
  if (!pluginShouldWork) {
    return false;
  }

  return archiveName.endsWith(".fomod", Qt::CaseInsensitive) ||
         archiveName.endsWith(".omod", Qt::CaseInsensitive);
}

// http://www.shloemi.com/2012/09/solved-setforegroundwindow-win32-api-not-always-works/
static void ForceWindowVisible(HWND hwnd)
{
  DWORD foregroundThread = ::GetWindowThreadProcessId(::GetForegroundWindow(), nullptr);
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


std::wstring InstallerNCC::getSEVersion(QString const &seloader)
{
  VS_FIXEDFILEINFO version = getFileVersionInfo(seloader);
  return (boost::basic_format<wchar_t>(L"%d.%d.%d")
          % (int)(version.dwFileVersionMS & 0xFFFF)
          % (int)(version.dwFileVersionLS >> 16)
          % (int)(version.dwFileVersionLS & 0xFFFF)).str();
}


IPluginInstaller::EInstallResult InstallerNCC::invokeNCC(IModInterface *modInterface, const IPluginGame *game, const QString &archiveName)
{
  wchar_t binary[MAX_PATH];
  wchar_t parameters[1024]; // maximum: 2xMAX_PATH + approx 20 characters
  wchar_t currentDirectory[MAX_PATH];
#pragma warning( push )
#pragma warning( disable : 4996 )
  _snwprintf(binary, MAX_PATH, L"%ls", ToWString(QDir::toNativeSeparators(nccPath())).c_str());


  std::wstring seString;
  ScriptExtender *extender = m_MOInfo->managedGame()->feature<ScriptExtender>();
  if (extender != nullptr && extender->isInstalled())
  {
    std::wstring seVersion = getSEVersion(extender->loaderPath());
    if (!seVersion.empty()) {
      seString = std::wstring() + L"-se \"" + seVersion + L"\"";
    }
  }

  // NCC doesn't always clean up the TEMP folder correctly so this force
  // deletes it before every install
  shellDelete(QStringList() << QDir::temp().absoluteFilePath("NMMCLI"));

  _snwprintf(parameters, sizeof(parameters), L"-g %ls -p \"%ls\" -gd \"%ls\" -d \"%ls\" %ls -i \"%ls\" \"%ls\"",
             game->gameShortName().toStdWString().c_str(),
             QDir::toNativeSeparators(QDir::cleanPath(m_MOInfo->profilePath())).toStdWString().c_str(),
             QDir::toNativeSeparators(QDir::cleanPath(m_MOInfo->managedGame()->gameDirectory().absolutePath())).toStdWString().c_str(),
             QDir::toNativeSeparators(QDir::cleanPath(m_MOInfo->overwritePath())).toStdWString().c_str(),
             seString.c_str(),
             QDir::toNativeSeparators(archiveName).toStdWString().c_str(),
             QDir::toNativeSeparators(modInterface->absolutePath()).toStdWString().c_str());
  _snwprintf(currentDirectory, MAX_PATH, L"%ls", ToWString(QFileInfo(nccPath()).absolutePath()).c_str());
#pragma warning( pop )

  // NCC assumes the installation directory is the game directory and may try to
  // access the binary to determine version information. So we have to copy the
  // executable and script extender in.
  QStringList filesToCopy;
  filesToCopy << m_MOInfo->managedGame()->binaryName();
  if (extender != nullptr && extender->isInstalled()) {
    filesToCopy << extender->loaderName();
  }

  QStringList copiedFiles;
  QDir modDir(modInterface->absolutePath());

  for (QString file : filesToCopy) {
    QString destination = modDir.absoluteFilePath(file);
    if (QFile::copy(m_MOInfo->managedGame()->gameDirectory().absoluteFilePath(file), destination)) {
      copiedFiles.append(destination);
    }
  }
  ON_BLOCK_EXIT([&copiedFiles] {
    if (!shellDelete(copiedFiles)) {
      reportError(QString("Failed to clean up after NCC installation, you may find some files "
                     "unrelated to the mod in the newly created mod directory: %1").arg(windowsErrorString(::GetLastError())));
    }
  });

  qDebug("running %ls %ls", binary, parameters);

  SHELLEXECUTEINFOW execInfo = {0};
  execInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
  execInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
  execInfo.hwnd = nullptr;
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
    // cleanup
    shellDelete(QStringList(modInterface->absolutePath() + "/Data"));
    shellDelete(QStringList(modInterface->absolutePath() + "/temp"));
    shellDelete(QStringList(modInterface->absolutePath() + "/NexusClientCLI.log"));
  } else if (exitCode != 11) { // 11 = manually canceled
    reportError(tr("installation failed (errorcode %1)").arg(exitCode));
  }

  return ((exitCode == 0) || (exitCode == 10)) ? RESULT_SUCCESS : RESULT_FAILED;
}


IPluginInstaller::EInstallResult InstallerNCC::install(GuessedValue<QString> &modName, QString gameName, const QString &archiveName,
                                                       const QString &version, int modID)
{
  if (!pluginShouldWork) {
    return RESULT_FAILED;
  }

  auto dialog = NccInstallDialog(modName, parentWidget());
  if (dialog.exec() == QDialog::Rejected) {
    if (dialog.manualRequested()) {
      modName.update(dialog.getName(), GUESS_USER);
      return RESULT_MANUALREQUESTED;
    }
    else {
      return RESULT_CANCELED;
    }
  }
  modName.update(dialog.getName().trimmed());

  const IPluginGame *game = m_MOInfo->getGame(gameName);
  if (game == nullptr) {
    game = m_MOInfo->managedGame();
    QStringList sources = game->primarySources();
    if (sources.size()) {
      game = m_MOInfo->getGame(sources.at(0));
      gameName = game->gameShortName();
    }
  }

  IModInterface *modInterface = m_MOInfo->createMod(modName);
  if (modInterface == nullptr) {
    return RESULT_CANCELED;
  }
  QFileInfo archiveInfo(archiveName);
  if (archiveInfo.dir() == QDir(m_MOInfo->downloadsPath())) {
    modInterface->setInstallationFile(QFileInfo(archiveName).fileName());
  } else {
    modInterface->setInstallationFile(archiveName);
  }
  modInterface->setVersion(version);
  modInterface->setNexusID(modID);
  modInterface->setGameName(gameName);

  EInstallResult res = invokeNCC(modInterface, game, archiveName);

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
            (m_MOInfo->modList()->getMod(newName) == nullptr)) {
          modInterface = m_MOInfo->modList()->renameMod(modInterface, modName);
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
    if (!m_MOInfo->modList()->removeMod(modInterface)) {
      qCritical("failed to remove empty mod %s", qUtf8Printable(modInterface->absolutePath()));
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
  try {
    VS_FIXEDFILEINFO temp = getFileVersionInfo(nccPath());
    return (temp.dwFileVersionMS & 0xFFFFFF) == COMPATIBLE_MAJOR_VERSION;
  } catch (const std::runtime_error &ex) {
    qCritical("%s", ex.what());
    return false;
  }
}

bool InstallerNCC::isDotNetInstalled() const
{
  return QSettings("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\NET Framework Setup\\NDP\\v4\\Full",
                   QSettings::NativeFormat).value("Release", 0) >= 393295;
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
      return tr(".NET 4.6 or greater is required.");
    default:
      throw MyException(tr("invalid problem key %1").arg(key));
  }
}

QString InstallerNCC::fullDescription(unsigned int key) const
{
  switch (key) {
    case PROBLEM_NCCMISSING:
      return tr("NCC is not installed. You won't be able to install some scripted mod-installers. "
                "There may have been problems when installing MO.");
    case PROBLEM_NCCINCOMPATIBLE:
      return tr("NCC version may be incompatible, expected version 0.%1.x.x.").arg(COMPATIBLE_MAJOR_VERSION);
    case PROBLEM_DOTNETINSTALLED: {
      QString dotNetUrl = "https://www.microsoft.com/en-us/download/details.aspx?id=48130";
      return tr("<li>.NET version 4.6 or greater is required to use NCC. "
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


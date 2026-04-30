//
// Copyright (C) 2018-2026 University of Amsterdam
//
// This program is free to redistribute and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// If not, see <http://www.gnu.org/licenses/>.
//

#include "jaspexporter.h"

#include <sys/stat.h>

#include <ios>
#include <archive.h>
#include <archive_entry.h>
#include <json/json.h>
#include <fstream>
#include "data/jaspencryptiondata.h"
#include "data/jaspencrypt.h"
#include "version.h"
#include "tempfiles.h"
#include "log.h"
#include "utilenums.h"
#include "utilities/qutils.h"
#include <fstream>
#include "appinfo.h"
#include "gui/preferencesmodel.h"
#include "data/asyncloader.h"

#include <utilities/desktopcommunicator.h>


const Version JASPExporter::jaspArchiveVersion = Version("5.0.0");
time_t JASPExporter::_now;
std::string JASPExporter::_globalWorkspaceSnapshot = "";
std::mutex JASPExporter::_snapshotMutex;

JASPExporter::JASPExporter()
{
	_defaultFileType = Utils::FileType::jasp;
	_allowedFileTypes.push_back(Utils::FileType::jasp);
}

void JASPExporter::setGlobalWorkspaceSnapshot(const std::string &path)
{
	std::lock_guard<std::mutex> lock(_snapshotMutex);
	_globalWorkspaceSnapshot = path;
}

std::string JASPExporter::getGlobalWorkspaceSnapshot()
{
	std::lock_guard<std::mutex> lock(_snapshotMutex);
	return _globalWorkspaceSnapshot;
}

void JASPExporter::saveDataSet(const std::string &path, std::function<void(int)> progressCallback)
{
	struct archive *a;

	_now = time(nullptr); //Give all files same timestamp
	
	std::string sourceDir = getGlobalWorkspaceSnapshot();
	
	std::filesystem::path tmpPath = path;
	bool encrypt = JaspEncryptionData::getInstance()->encryptionActive();
	if(encrypt) {
		if(!JaspEncryptionData::getInstance()->paramsSet())
		{
			if (!DesktopCommunicator::singleton()->queryEncryptionSettings())
				throw LoaderException("Query encryption settings is cancelled", true); // Cancelled
		}
		
		if(!JaspEncryptionData::getInstance()->paramsSet())
			throw LoaderException(DesktopCommunicator::tr("No password given!").toStdString());
		
		tmpPath = std::filesystem::temp_directory_path() / ("_tmp_unlock_" + std::filesystem::path(path).filename().generic_string());
	}

	a = archive_write_new();
	archive_write_set_format_zip(a);
	

#ifdef _WIN32
	if (archive_write_open_filename_w(a, QString(tmpPath.c_str()).toStdWString().c_str()) != ARCHIVE_OK)
#else
	if (archive_write_open_filename(a, tmpPath.c_str()) != ARCHIVE_OK)
#endif
		throw LoaderException(std::string("File could not be opened because of ") + archive_error_string(a));

	saveManifest(a, sourceDir);    progressCallback(10);
	saveAnalyses(a, sourceDir);    progressCallback(30);
	saveResults(a, sourceDir);     progressCallback(70);
	saveDatabase(a, sourceDir);    progressCallback(100);
	
	if (archive_write_close(a) != ARCHIVE_OK)
		throw LoaderException("File could not be closed.");

	archive_write_free(a);
	if(encrypt) {
		Json::Value root;
		try {
            auto privKey = JaspEncryptionData::getInstance()->getPrivatekey();
            if(privKey.length()) //check if user want to use privkey or password to encrypt
                JASPEncrypt::encrypt(tmpPath, path, privKey, root, JaspEncryptionData::getInstance()->getPublicKeyResponse(), JaspEncryptionData::getInstance()->getPasswordSaltResponse(), true);
            else
                JASPEncrypt::encrypt(tmpPath, path, JaspEncryptionData::getInstance()->getPassword(), root, JaspEncryptionData::getInstance()->getPublicKeyResponse());
		} catch (std::exception& e) {
			Log::log() << "Encryption failed: " << e.what() << std::endl;
			throw LoaderException("Encryption failed. Click 'Save As' and save as normal Jasp File. \n\n" + std::string(" Technical Reason: ") + std::string(e.what()));
		}
		std::filesystem::remove(tmpPath);
	}

	//Make sure it is now always considered "loading" in DataSetPackage
	DataSetPackage::pkg()->setLoaded(true);
}

void JASPExporter::saveManifest(archive * a, const std::string &sourceDir)
{
    Json::Value manifest = Json::objectValue;

    manifest["jaspArchiveVersion"]	= jaspArchiveVersion.asString();
    manifest["jaspVersion"]			= AppInfo::version.asString();

    makeEntry(a, "manifest.json", manifest.toStyledString(), sourceDir);
}

void JASPExporter::saveResults(archive * a, const std::string &sourceDir)
{
	DataSetPackage::pkg()->waitForExportResultsReady();

	makeEntry(a, "index.html", fq(DataSetPackage::pkg()->analysesHTML()), sourceDir);
}

void JASPExporter::saveTempFile(archive *a, const std::string & filePath, const std::string &sourceDir)
{
	// std::ios::ate seeks to the end of stream immediately after open
	std::string effectivePath = sourceDir.empty() ? filePath : sourceDir + "/" + filePath;
	std::ifstream   readTempFile(effectivePath, std::ios::ate | std::ios::binary);
	char            fileBuff[8192];

	if (readTempFile.is_open())
	{
		archive_entry * entry       = archive_entry_new();

		archive_entry_set_pathname( entry,  filePath.c_str());
		archive_entry_set_size(		entry,	readTempFile.tellg()); // get size from curpos after ios::ate seek
		archive_entry_set_filetype(	entry,	AE_IFREG);
		archive_entry_set_birthtime(entry,  _now, 0);
		archive_entry_set_ctime(    entry,  _now, 0);
		archive_entry_set_mtime(    entry,  _now, 0);
		archive_entry_set_atime(    entry,  _now, 0);
		archive_entry_set_perm(		entry,	0644); //set some read write permissions

		archive_write_header(		a,		entry);

		readTempFile.seekg(0, std::ios::beg);	// move back to begin
		while (!readTempFile.eof())
		{
			readTempFile.read(fileBuff, sizeof(fileBuff));
			archive_write_block(a, &fileBuff, readTempFile.gcount());
		}
		archive_entry_free(entry);
	}
	else
	{
		Log::log() << "JASP Export: cannot find/open file " << filePath << " in " << sourceDir << std::endl;;
#ifdef JASP_DEBUG
		//If we are building jasp ourselves or debugging it might be helpful to know stuff is not getting written.
		//A normal user should however not be forced to endure a crash for that. Because half a jasp file could be better than nothing
		throw LoaderException("JASP Export: cannot find/open file " + filePath);
#endif
	}
	readTempFile.close();
}

void JASPExporter::saveAnalyses(archive *a, const std::string &sourceDir)
{
	const Json::Value analysesJson = DataSetPackage::pkg()->analysesData();

	makeEntry(a, "analyses.json", analysesJson.toStyledString(), sourceDir);

	const Json::Value & analysesDataList = analysesJson.isArray() ? analysesJson : analysesJson["analyses"];
	
	for (const Json::Value & analysisJson : analysesDataList)
		for (const std::string & path : TempFiles::retrieveList(analysisJson["id"].asInt()))
			if(!stringUtils::endsWith(path, "/state") || (PreferencesModel::prefs()->storeStateEtc() && analysisJson.get("saveState", "default").asString() != "never") || analysisJson.get("saveState", "default").asString() == "always")
				saveTempFile(a, path, sourceDir);
}

void JASPExporter::saveDatabase(archive * a, const std::string &sourceDir)
{
	saveTempFile(a, DatabaseInterface::singleton()->dbFile(true), sourceDir);
	//saveTempFile(a, DatabaseInterface::singleton()->dbFile(true)+"-shm", sourceDir);
	//saveTempFile(a, DatabaseInterface::singleton()->dbFile(true)+"-wal", sourceDir);
}

void JASPExporter::makeEntry(archive * a, const std::string & filename, const std::string & data, const std::string &sourceDir)
{
	archive_entry *entry = archive_entry_new();

	archive_entry_set_pathname(	entry, filename.c_str());
	archive_entry_set_size(		entry, int(data.size()));
	archive_entry_set_birthtime(entry,  _now, 0);
	archive_entry_set_ctime(    entry,  _now, 0);
	archive_entry_set_mtime(    entry,  _now, 0);
	archive_entry_set_atime(    entry,  _now, 0);
	archive_entry_set_filetype(	entry,	AE_IFREG);
	archive_entry_set_perm(		entry,	0644); //set some read write permissions

	archive_write_header(               a,	entry);
	size_t written = archive_write_data(a,  data.c_str(), data.size());

	if (written != data.size())
		throw LoaderException("File could not be written.");

	archive_entry_free(entry);
}

void JASPExporter::saveTempFile(archive *a, const std::string & filePath, const std::string &sourceDir)
{
	// std::ios::ate seeks to the end of stream immediately after open
	std::ifstream   readTempFile(TempFiles::sessionDirName() + "/" + filePath, std::ios::ate | std::ios::binary);
	char            fileBuff[8192];

	if (readTempFile.is_open())
	{
		archive_entry * entry       = archive_entry_new();

		archive_entry_set_pathname( entry,  filePath.c_str());
		archive_entry_set_size(		entry,	readTempFile.tellg()); // get size from curpos after ios::ate seek
		archive_entry_set_filetype(	entry,	AE_IFREG);
		archive_entry_set_birthtime(entry,  _now, 0);
		archive_entry_set_ctime(    entry,  _now, 0);
		archive_entry_set_mtime(    entry,  _now, 0);
		archive_entry_set_atime(    entry,  _now, 0);
		archive_entry_set_perm(		entry,	0644); //set some read write permissions

		archive_write_header(		a,		entry);

		readTempFile.seekg(0, std::ios::beg);	// move back to begin
		while (!readTempFile.eof())
		{
			readTempFile.read(fileBuff, sizeof(fileBuff));
			archive_write_block(a, &fileBuff, readTempFile.gcount());
		}
		archive_entry_free(entry);
	}
	else
	{
		Log::log() << "JASP Export: cannot find/open file " << filePath << " in " << sourceDir << std::endl;;
#ifdef JASP_DEBUG
		//If we are building jasp ourselves or debugging it might be helpful to know stuff is not getting written.
		//A normal user should however not be forced to endure a crash for that. Because half a jasp file could be better than nothing
		throw LoaderException("JASP Export: cannot find/open file " + filePath);
#endif
	}
	readTempFile.close();
}

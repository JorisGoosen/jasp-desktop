//
// Copyright (C) 2018 University of Amsterdam
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#ifndef JASPEXPORTER_H
#define JASPEXPORTER_H

#include "exporter.h"
#include <archive.h>

class JASPExporter : public Exporter
{
public:
	JASPExporter();
	virtual void saveDataSet(const std::string &path, std::function<void(int)> progressCallback) override;

	static void setGlobalWorkspaceSnapshot(const std::string &path);
	static std::string getGlobalWorkspaceSnapshot();

private:
	void saveManifest(archive * a, const std::string &sourceDir);
	void saveAnalyses(archive * a, const std::string &sourceDir);
	void saveResults(archive * a, const std::string &sourceDir);
	void saveDatabase(archive * a, const std::string &sourceDir);
	void saveTempFile(archive *a, const std::string & filePath, const std::string &sourceDir);
	void makeEntry(archive * a, const std::string & filename, const std::string & data, const std::string &sourceDir);

	static Version jaspArchiveVersion;
	static time_t _now;

	static std::string _globalWorkspaceSnapshot;
	static std::mutex _snapshotMutex;
};

#endif // JASPEXPORTER_H


#include "mysql_fs_layer.hpp"
#include <dbpool/db_pool.h>
#include <dbpool/db_pool_single.h>
#include <dbpool/statement.h>
#include <dbpool/handle_var.h>
#include <yandex/logger.h>
#include <algorithm>
#include <string>
#include <boost/utility.hpp>
#include <boost/filesystem.hpp>
#include <map>

using namespace std;
using namespace mypp;
using namespace boost;

#define db_pool_ (theDBPool::Instance())

namespace DPS {

	MysqlLayer::MysqlLayer() : 
		time_out_(theConfig::Instance().getValue("DB_POOL_TIMEOUT").asInt()), has_slave_(false)
	{
		std::string read_dns_config = theConfig::Instance().getValue("DB_READ_DSNS").asString();
		std::string write_dns_config = theConfig::Instance().getValue("DB_WRITE_DSNS").asString();
		read_dsn_ = CreateFromString(read_dns_config);
		write_dsn_ = CreateFromString(write_dns_config);
		Driver::load_driver(read_dsn_.begin()->driver);

		if (read_dns_config != write_dns_config) {
			Handle_var sqlh(db_pool_, read_dsn_, time_out_);
			Statement stm(sqlh, "select 1 from DUAL");
			if (stm.Execute()) {
				has_slave_ = true;
				LOG(LogLevel::INFO, "Slave detected: " << read_dns_config << endl);
			}
		}
	}

	MysqlLayer::~MysqlLayer()
	{
	}

	string ConstructReadFileQuery(const string version, bool forceRemoved) {

		auto lowerVersion = version;
		transform(lowerVersion.begin(), lowerVersion.end(), lowerVersion.begin(), ::tolower);
		auto stableVersionRequested = (lowerVersion	== "stable");
		auto latestVersionRequested = (lowerVersion	== "latest");
		auto versionRequested = (!stableVersionRequested && !latestVersionRequested);

		auto requestedVersionId = -1;
		if (versionRequested)
			requestedVersionId = stol(version);

		auto query = 
			(string) 
			"SELECT f.FileID as ID, \n"
			"		'File' as Type, \n"
			"		f.Title as Name, \n"
			"		f.CatalogID as ParentCatalogID, \n"
			"		f.CreationTime, \n"
			"		f.ModificationTime, \n"
			"		f.IsDeleted, \n"
			"		f.IsLink, \n"
			"		CONCAT(c.FullName, '/', f.Title) as FullName \n"
			",		vd.FileData as Content \n" 
			",		vd.VersionID as SelectedVersionID \n"  
			 + ( stableVersionRequested ? ",		fp.PropValue as StableVersionID \n" : ",    Null as StableVersionID \n" ) +  
			"FROM Catalogs as c \n"
			"STRAIGHT_JOIN Files as f on c.CatalogID = f.CatalogID \n"
            + ( stableVersionRequested ?  "STRAIGHT_JOIN FileProperties as fp on fp.FileID = f.FileID AND PropName = 'stable' \n" : "" ) +
			"STRAIGHT_JOIN VersionedData as vd on vd.FileID = f.FileID \n" 
			+ ( latestVersionRequested ? 
					"	AND vd.VersionID = (SELECT MAX(VersionID) FROM VersionedData WHERE FileID = f.FileID) \n" : "" )
			+ ( stableVersionRequested ? 
					"	AND vd.VersionID = fp.PropValue \n" : "")
			+ ( versionRequested ? 
					"	AND vd.VersionID = " + to_string(requestedVersionId) + "\n" : "" ) +
			"WHERE"
            + ( forceRemoved ? "       " : "       f.IsDeleted = 'no' \nAND    " ) +
            "c.FullName LIKE ? \n"
            "AND    f.Title LIKE ? \n";
		return query;
	}

	string ConstructReadDirQuery(const string version, bool fetchContent, bool fetchDirs, bool forceRemoved) {

		auto lowerVersion = version;
		transform(lowerVersion.begin(), lowerVersion.end(), lowerVersion.begin(), ::tolower);
		auto stableVersionRequested = (lowerVersion	== "stable");
		auto latestVersionRequested = (lowerVersion	== "latest");
		auto versionRequested = (!stableVersionRequested && !latestVersionRequested);

		auto requestedVersionId = -1;
		if (versionRequested)
			requestedVersionId = stol(version);

		auto query = 
			(string)
            "SELECT CatalogID as ID, \n"
            "       'dir' as Type, \n"
            "       Title as Name, \n"
            "       ParentID as ParentCatalogID, \n"
            "       CreationTime, \n"
            "       ModificationTime, \n"
            "       IsDeleted, \n"
            "       IsLink, \n"
            "       FullName, \n"
            "       Null as Content, \n"
            "       Null as SelectedVersionID, \n"
            "       Null as StableVersionID \n" 
            "FROM Catalogs \n"
            "WHERE \n"
            + ( forceRemoved ? "       " : "       IsDeleted = 'no' \nAND    " ) +
            "( FullName LIKE ?" +
            ( fetchDirs ? "OR     ParentID IN (SELECT CatalogID FROM Catalogs WHERE (FullName LIKE ? OR FullName LIKE ?))) \n" : ")\n" ) + 
            "UNION ALL\n" + 
			"SELECT f.FileID as ID, \n"
			"		'file' as Type, \n"
			"		f.Title as Name, \n"
			"		f.CatalogID as ParentCatalogID, \n"
			"		f.CreationTime, \n"
			"		f.ModificationTime, \n"
			"		f.IsDeleted, \n"
			"		f.IsLink, \n"
			"		CONCAT(c.FullName, '/', f.Title) as FullName \n"
			+ ( fetchContent ? 
			",		vd.FileData as Content \n" 
			",		vd.VersionID as SelectedVersionID \n" : 
			",		Null as Content \n"
			",		Null as SelectedVersionID \n" ) +
			+ ( fetchContent && stableVersionRequested ? ",		fp.PropValue as StableVersionID \n" : ",    Null as StableVersionID \n" ) +  
			"FROM Catalogs as c \n"
			"STRAIGHT_JOIN Files as f on c.CatalogID = f.CatalogID \n"
            + ( fetchContent && stableVersionRequested ? "STRAIGHT_JOIN FileProperties as fp on fp.FileID = f.FileID AND fp.PropName = 'stable' \n" : "" ) +

			+ ( fetchContent ?  "STRAIGHT_JOIN VersionedData as vd on vd.FileID = f.FileID \n" : "") + 
			+ ( fetchContent && latestVersionRequested ? "	AND vd.VersionID = (SELECT MAX(VersionID) FROM VersionedData WHERE FileID = f.FileID) \n" : "" )
			+ ( fetchContent && stableVersionRequested ? "	AND vd.VersionID = fp.PropValue \n" : "")
			+ ( fetchContent && versionRequested ? "	AND vd.VersionID = " + to_string(requestedVersionId) + "\n" : "" ) +
			
            "WHERE"
            + ( forceRemoved ? "       " : "       f.IsDeleted = 'no' \nAND    " ) +
            "(c.FullName LIKE ? OR c.FullName LIKE ?) \n";
		return query;
	}

	string trimSlash(const string& str) {
		if (str.back() == '/')
			return str.substr(0, str.size()-1);
		return str;
	}

	Entry MysqlLayer::ReadFile(const string& path, const string& version, const bool& forceRemoved) const
	{
		auto query = ConstructReadFileQuery(version, forceRemoved);
		Handle_var sqlh(db_pool_, write_dsn_, time_out_);
		Statement stm(sqlh, query);

		filesystem::path entryPath(path);
		auto root = trimSlash(entryPath.branch_path().string());
		auto leaf = entryPath.leaf().string();
	    // LOG(LogLevel::ERROR, "ReadFile query: " << endl << QueryException(query, root, leaf).what() << endl);	
		try {
			if (!stm.Execute(tie(
							root,								// Directory part for files select 
							leaf								// Filename part for files select
							))) throw QueryException(query, root, leaf);
		} catch (...) { 
			throw QueryException(query, root, leaf);
		}
		Row row; 
		if (!stm.Fetch(row)) {
			throw NotFoundException(path);
		}

		auto entry = Entry(
				row[0].asString(),		// id 
				row[1].asString(),		// type
				row[2].asString(),		// name
				row[3].asString(),		// parentid
				row[4].asString(),		// ctime
				row[5].asString(),		// mtime
				row[6].asString(),		// isdeleted 
				row[7].asString(),		// islink 
				row[8].asString(),		// fullname 
				row[9].isNull() ? "" : row[9].asString(),		// content
				row[10].isNull() ? "-1" : row[10].asString(),		// selectedversion
				row[11].isNull() ? "-1" : row[11].asString());		// stableversion

		if (!entry.IsFile())
			return entry;

		auto propQuery = "SELECT FileID, PropName, PropValue FROM FileProperties WHERE FileID = ?";
		try {
			Statement propertiesStatement(sqlh, propQuery);
			if (!propertiesStatement.Execute(tie(entry.id))) 
				return entry;

			for (Row row : propertiesStatement) {
				entry.properties.push_back(Property(row[1].asString(), row[2].asString()));
			}
		} catch (...) {
			throw QueryException(propQuery, to_string(entry.id));
		}

		return entry;
	};

    Revisions MysqlLayer::GetRevisions(const std::string& path) const {
        auto query = (string) "SELECT VersionID, CreationDate FROM VersionedData WHERE FileID = (" +
            "SELECT FileID FROM Files WHERE Title = ? AND CatalogID = (" +
            "SELECT CatalogID FROM Catalogs WHERE FullName = ? " + 
            ") ) ORDER BY VersionID";

		Handle_var sqlh(db_pool_, write_dsn_, time_out_);
		Statement stm(sqlh, query);

        auto fullpath = trimSlash(path);
		filesystem::path entryPath(fullpath);
		auto title = entryPath.leaf().string();
        auto fullname = trimSlash(entryPath.branch_path().string());
	    //LOG(LogLevel::ERROR, "GetRevisions query: " << endl << QueryException(query, title, fullname).what() << endl);	
        if (!stm.Execute(tie(
                        title,
                        fullname
                        ))) throw QueryException(query, title, fullname);

        Revisions revisions;

		for (Row row : stm) {
            revisions.push_back(Revision(
                row[0].asString(),		// VersionID
                row[1].asString()		// CreationDate
            ));
        }

        return revisions;
    };

    Properties MysqlLayer::GetProperties(const std::string& path) const {
        auto query = (string) "SELECT PropName, PropValue FROM FileProperties WHERE FileID = (" +
            "SELECT FileID FROM Files WHERE Title = ? AND CatalogID = (" +
            "SELECT CatalogID FROM Catalogs WHERE FullName = ? " + 
            ") )";

		Handle_var sqlh(db_pool_, write_dsn_, time_out_);
		Statement stm(sqlh, query);

        auto fullpath = trimSlash(path);
		filesystem::path entryPath(fullpath);
		auto title = entryPath.leaf().string();
        auto fullname = trimSlash(entryPath.branch_path().string());
	    // LOG(LogLevel::ERROR, "GetProperties query: " << endl << QueryException(query, title, fullname).what() << endl);	
        if (!stm.Execute(tie(
                        title,
                        fullname
                        ))) throw QueryException(query, title, fullname);

        Properties properties;

		for (Row row : stm) {
            properties.push_back(Property(
                row[0].asString(),		// PropName
                row[1].asString()		// PropValue
            ));
        }

        return properties;
    };

	Entries MysqlLayer::ReadDir(const std::string& path, const string& version, const bool& recursive, const bool& fetchContent, const bool& properties, const bool& fetchDirs, const bool& forceRemoved) const
	{
		auto query = ConstructReadDirQuery(version, fetchContent, fetchDirs, forceRemoved);
		Handle_var sqlh(db_pool_, write_dsn_, time_out_);
		Statement stm(sqlh, query);

		filesystem::path entryPath(path);
		auto fullpath = entryPath.string();
        fullpath = trimSlash(fullpath);
        auto fullpathRecursive = fullpath;
		if (recursive) { 
            fullpathRecursive += "/%";
        }
        if (fetchDirs) {
            if (!stm.Execute(tie(
                            fullpath,
                            fullpath,
                            fullpathRecursive,
                            fullpath,
                            fullpathRecursive
                            ))) throw QueryException(query, fullpath, fullpathRecursive, fullpath, fullpathRecursive);	
        } else {
            if (!stm.Execute(tie(
                            fullpath,
                            fullpath,
                            fullpathRecursive
                            ))) throw QueryException(query, fullpath, fullpath, fullpathRecursive);	
        }

		string propertiesQuery = "SELECT FileID, PropName, PropValue FROM FileProperties WHERE FileID IN (";

		Entries entries;
		map<ID_t, Entry*> entriesMap;
		for (Row row : stm) {
			entries.push_back(Entry(
						row[0].asString(),		// id 
						row[1].asString(),		// type
						row[2].asString(),		// name
						row[3].asString(),		// parentid
						row[4].asString(),		// ctime
						row[5].asString(),		// mtime
						row[6].asString(),		// isdeleted 
						row[7].asString(),		// islink 
						row[8].asString(),		// fullname 
						row[9].isNull() ? "" : row[9].asString(),		// content
						row[10].isNull() ? "-1" : row[10].asString(),		// selectedversion
						row[11].isNull() ? "-1" : row[11].asString())		// stableversion
					);
			Entry* entry = &*prev(entries.end());
			if (entry->IsFile()) {
				propertiesQuery += row[0].asString() + ", ";
				entriesMap[entry->id] = entry;
			}
		}

		if (!properties || !entriesMap.size())
			return entries;

		propertiesQuery.erase(propertiesQuery.length() - 2);
		propertiesQuery += ")";
		try {
			Statement propertiesStatement(sqlh, propertiesQuery);
			if (!propertiesStatement.Execute()) 
				return entries;

			for (Row row : propertiesStatement) {
				entriesMap[row[0].asLong()]->properties.push_back(Property(row[1].asString(), row[2].asString()));
			}
		} catch (...) {
			throw QueryException(propertiesQuery);
		}
		return entries;
	};

}

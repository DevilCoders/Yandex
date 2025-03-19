#ifndef MYSQL_FS_LAYER_H
#define MYSQL_FS_LAYER_H

#include "singleton_holder.hpp"
#include "types.hpp"
#include <dbpool/db_pool.h>
#include <dbpool/handle_var.h>

namespace DPS {

	class MysqlLayer : public boost::noncopyable
	{
		public:
			typedef unsigned long IDType;

			bool HasSlave() const;

			Entry ReadFile(const std::string& path, const string& version, const bool& forceRemoved) const;
			Entries ReadDir(const std::string& path, const string& version, const bool& recursive, const bool& fetchContent, const bool& properties, const bool& fetchDirs, const bool& forceRemoved) const;
            Revisions GetRevisions(const std::string& path) const; 
            Properties GetProperties(const std::string& path) const; 

		private:
			MysqlLayer();
			~MysqlLayer();

			friend class Yandex::DPS::CreateUsingNew<MysqlLayer>;

			mypp::DSN_list read_dsn_;
			mypp::DSN_list write_dsn_;

			int time_out_;
			bool has_slave_;
	};

	typedef Yandex::DPS::SingletonHolder<MysqlLayer> SingletonMysqlLayer;

}

#endif

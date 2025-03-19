#ifndef INDEX_HANDLER_HPP_INCLUDED
#define INDEX_HANDLER_HPP_INCLUDED

#include <fastcgi2/handler.h>
#include <fastcgi2/request.h>
#include <fastcgi2/stream.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <string>
#include <time.h>
#include "../mysql_fs_layer.hpp"
#include "../writers/abstract.hpp"
#include "../writers/xml.hpp"
#include "../writers/json.hpp"
#include "../writers/msgpack.hpp"

using namespace std;

namespace DPS {
	namespace Handle {
		class Index {
			public:
				Index(fastcgi::Request *request, fastcgi::HandlerContext *context);	

			private:
				void process();
				void processDirectory();
				void createItem(xmlTextWriterPtr writer, Entry& entry); 
				string file;
				string dir;
				string version;
				bool forceRemoved;
				bool fetchContent;
				bool fetchDirs;
				bool recursive;
                bool properties;
				string format;
				fastcgi::Request* req;
				fastcgi::HandlerContext* ctx;
		};
	}
}

#endif

#ifndef DOCUMENT_METADATA_HANDLER_HPP_INCLUDED
#define DOCUMENT_METADATA_HANDLER_HPP_INCLUDED

#include <fastcgi2/handler.h>
#include <fastcgi2/request.h>
#include <fastcgi2/stream.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <string>
#include "../mysql_fs_layer.hpp"

using namespace std;
using namespace boost::posix_time;

namespace DPS {
	namespace Handle {
		class DocumentMetadata {
			public:
				DocumentMetadata(fastcgi::Request *request, fastcgi::HandlerContext *context);	

			private:
				void process();
				string file;
                bool properties;
                bool revisions;
				fastcgi::Request* req;
				fastcgi::HandlerContext* ctx;
		};
	}
}

#endif

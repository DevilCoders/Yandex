#ifndef PING_HANDLER_HPP_INCLUDED
#define PING_HANDLER_HPP_INCLUDED

#include <fastcgi2/handler.h>
#include <fastcgi2/request.h>
#include <fastcgi2/stream.h>
#include <string>

using namespace std;

namespace DPS {
	namespace Handle {
		class Ping {
			public:
				Ping(fastcgi::Request *request, fastcgi::HandlerContext *context);	
		};
	}
}

#endif

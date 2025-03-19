#include "ping.hpp"

namespace DPS {
	namespace Handle {
		
		Ping::Ping(fastcgi::Request *request, fastcgi::HandlerContext *context) {
			request->setStatus(200);
			fastcgi::RequestStream stream(request);
			stream << "pong";
		};

	}
}

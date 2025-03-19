#ifndef _DPS_PROXY_HPP_INCLUDED_
#define _DPS_PROXY_HPP_INCLUDED_

#include <fastcgi2/component.h>
#include <fastcgi2/handler.h>
#include <fastcgi2/logger.h>
#include <fastcgi2/request.h>
#include <exception>
#include "mysql_fs_layer.hpp"

namespace DPS {

	class Proxy : virtual public fastcgi::Component, virtual public fastcgi::Handler {

		public:
			Proxy(fastcgi::ComponentContext *context);
			virtual ~Proxy();

		public:
			virtual void onLoad();
			virtual void onUnload();
			virtual void handleRequest(fastcgi::Request *request, fastcgi::HandlerContext *context);

		private:
			fastcgi::Logger *logger_;
	};

} // namespace dps

#endif // _DPS_PROXY_HPP_INCLUDED_


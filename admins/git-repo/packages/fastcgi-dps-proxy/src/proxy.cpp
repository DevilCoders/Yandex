#include <fastcgi2/config.h>
#include <fastcgi2/handler.h>
#include <fastcgi2/request.h>
#include <fastcgi2/component.h>
#include <fastcgi2/component_factory.h>
#include <fastcgi2/stream.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <iostream>

#include "mysql_fs_layer.hpp"
#include "proxy.hpp"

#include "handles/index.hpp"
#include "handles/ping.hpp"
#include "handles/documentMetadata.hpp"

using namespace std;

namespace DPS {

	Proxy::Proxy(fastcgi::ComponentContext *context) :
		fastcgi::Component(context),
		logger_(NULL) {
		}

	Proxy::~Proxy() {
	}

	void
		Proxy::onLoad() {

		}

	void
		Proxy::onUnload() {

		}

	void
		Proxy::handleRequest(fastcgi::Request *request, fastcgi::HandlerContext *context) {
			fastcgi::RequestStream stream(request);
			try {
				string path = request->getScriptName();
						
				if (path == "/ping") {
					Handle::Ping(request, context);
				} else if (path == "/documentMetadata") {
					Handle::DocumentMetadata(request, context);
				} else {
					Handle::Index(request, context);
				}
			} catch(NotFoundException& ex) {
				request->setStatus(404);
				stream << "Error 404: " << ex.what();
			} catch(QueryException& ex) {
				request->setStatus(500);
				stream << "Error 500: " << ex.what();
			} catch(exception& ex) {
				request->setStatus(503);
				stream << "Error 503: " << ex.what();
			} catch(...) {
				request->setStatus(503);
				stream << "Unknown exception - we are in serious troubles.";
			}
		}

	FCGIDAEMON_REGISTER_FACTORIES_BEGIN()
		FCGIDAEMON_ADD_DEFAULT_FACTORY("proxy", Proxy);
	FCGIDAEMON_REGISTER_FACTORIES_END()

} // namespace dps

#include <sstream>
#include <string>
#include <iostream>

#include <library/cpp/uri/uri.h>
#include <util/datetime/base.h>

#include "connection.hpp"
#include "request_handler.hpp"
#include "ucred_option.hpp"

namespace NHttpServer {
    TResponse TRequestHandler::HandleRequest(const TConnection& connection) const {
        const TRequest& request = connection.GetRequest();
        auto handler = FindHandler(request.Method, std::string(request.Uri.GetField(NUri::TField::FieldPath)));
        if (handler.first) {
            TResponse response = handler.second->second(connection);

            if (!response.ContainsHeader("Content-Length")) {
                response.AddHeader("Content-Length", std::to_string(response.Content.size()));
            }

            return response;
        }

        return TResponse::GetStockReply(TResponse::EStatus::NotImplemented);
    }

    void TRequestHandler::AddHandler(const std::string& method, const std::string& uriPath, const TReqHandler& requestHandler) {
        TPathHandlerMap map;

        RequestHandlers.insert(std::pair(method, map)).first->second.insert_or_assign(uriPath, requestHandler);
    }

    std::pair<bool, TRequestHandler::TPathHandlerMap::const_iterator> TRequestHandler::FindHandler(
        const std::string& method,
        const std::string& uriPath
    ) const {
        auto it = RequestHandlers.find(method);
        if (it != RequestHandlers.end()) {
            auto handler = it->second.find(uriPath);
            if (handler != it->second.end()) {
                return std::pair(true, handler);
            }
        }

        return std::pair(false, TPathHandlerMap::const_iterator());
    }
}

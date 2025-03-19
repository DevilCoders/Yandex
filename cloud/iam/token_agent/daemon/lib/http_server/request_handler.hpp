#pragma once

#include <functional>
#include <map>
#include <string>

#include "request.hpp"
#include "response.hpp"

namespace NHttpServer {
    class TConnection;

    class TRequestHandler {
    public:
        using TReqHandler = std::function<TResponse(const TConnection&)>;
        using TPathHandlerMap = std::map<std::string, TReqHandler>;
        using TReqHandlerMap = std::map<std::string, TPathHandlerMap>;

        TRequestHandler() = default;

        TRequestHandler(const TRequestHandler&) = delete;
        TRequestHandler &operator=(const TRequestHandler&) = delete;

        TResponse HandleRequest(const TConnection& connection) const;

        void AddHandler(const std::string& method, const std::string& uriPath, const TReqHandler& requestHandler);

    private:
        std::pair<bool, TPathHandlerMap::const_iterator> FindHandler(const std::string& method, const std::string& uriPath) const;

        TReqHandlerMap RequestHandlers;
    };
}

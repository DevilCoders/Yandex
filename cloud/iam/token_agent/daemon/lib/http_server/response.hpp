#pragma once

#include <string>
#include <vector>
#include <asio.hpp>

#include "header.hpp"

namespace NHttpServer {
    struct TResponse {
        enum class EStatus;

        explicit TResponse(EStatus status = EStatus::Ok)
            : Status(status)
        {
        }

        TResponse& AddHeader(const std::string& name, const std::string& value);
        bool ContainsHeader(const std::string& name);

        /// Convert the reply into a vector of buffers. The buffers do not own the
        /// underlying memory blocks, therefore the response object must remain valid and
        /// not be changed until the write operation has completed.
        std::vector<asio::const_buffer> ToBuffers() const;

        static TResponse GetStockReply(EStatus status, const std::string& message = "");

        enum class EStatus {
            Ok = 200,
            Created = 201,
            Accepted = 202,
            NoContent = 204,
            MultipleChoices = 300,
            MovedPermanently = 301,
            MovedTemporarily = 302,
            NotModified = 304,
            BadRequest = 400,
            Unauthorized = 401,
            Forbidden = 403,
            NotFound = 404,
            InternalServerError = 500,
            NotImplemented = 501,
            BadGateway = 502,
            ServiceUnavailable = 503
        } Status;

        /// The headers to be included in the reply.
        std::vector<THeader> Headers;

        /// The content to be sent in the reply.
        std::string Content;
    };
}

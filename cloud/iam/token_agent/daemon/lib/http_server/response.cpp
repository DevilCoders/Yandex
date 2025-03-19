#include <string>

#include <util/system/compiler.h>

#include "response.hpp"

namespace NHttpServer {
    namespace NStatusStrings {
        const std::string OK                    { "HTTP/1.0 200 OK\r\n"                    };
        const std::string CREATED               { "HTTP/1.0 201 Created\r\n"               };
        const std::string ACCEPTED              { "HTTP/1.0 202 Accepted\r\n"              };
        const std::string NO_CONTENT            { "HTTP/1.0 204 No Content\r\n"            };
        const std::string MULTIPLE_CHOICES      { "HTTP/1.0 300 Multiple Choices\r\n"      };
        const std::string MOVED_PERMANENTLY     { "HTTP/1.0 301 Moved Permanently\r\n"     };
        const std::string MOVED_TEMPORARILY     { "HTTP/1.0 302 Moved Temporarily\r\n"     };
        const std::string NOT_MODIFIED          { "HTTP/1.0 304 Not Modified\r\n"          };
        const std::string BAD_REQUEST           { "HTTP/1.0 400 Bad Request\r\n"           };
        const std::string UNAUTHORIZED          { "HTTP/1.0 401 Unauthorized\r\n"          };
        const std::string FORBIDDEN             { "HTTP/1.0 403 Forbidden\r\n"             };
        const std::string NOT_FOUND             { "HTTP/1.0 404 Not Found\r\n"             };
        const std::string INTERNAL_SERVER_ERROR { "HTTP/1.0 500 Internal Server Error\r\n" };
        const std::string NOT_IMPLEMENTED       { "HTTP/1.0 501 Not Implemented\r\n"       };
        const std::string BAD_GATEWAY           { "HTTP/1.0 502 Bad Gateway\r\n"           };
        const std::string SERVICE_UNAVAILABLE   { "HTTP/1.0 503 Service Unavailable\r\n"   };

        asio::const_buffer ToBuffer(TResponse::EStatus status) {
            switch (status) {
                case TResponse::EStatus::Ok:
                    return asio::buffer(OK);
                case TResponse::EStatus::Created:
                    return asio::buffer(CREATED);
                case TResponse::EStatus::Accepted:
                    return asio::buffer(ACCEPTED);
                case TResponse::EStatus::NoContent:
                    return asio::buffer(NO_CONTENT);
                case TResponse::EStatus::MultipleChoices:
                    return asio::buffer(MULTIPLE_CHOICES);
                case TResponse::EStatus::MovedPermanently:
                    return asio::buffer(MOVED_PERMANENTLY);
                case TResponse::EStatus::MovedTemporarily:
                    return asio::buffer(MOVED_TEMPORARILY);
                case TResponse::EStatus::NotModified:
                    return asio::buffer(NOT_MODIFIED);
                case TResponse::EStatus::BadRequest:
                    return asio::buffer(BAD_REQUEST);
                case TResponse::EStatus::Unauthorized:
                    return asio::buffer(UNAUTHORIZED);
                case TResponse::EStatus::Forbidden:
                    return asio::buffer(FORBIDDEN);
                case TResponse::EStatus::NotFound:
                    return asio::buffer(NOT_FOUND);
                case TResponse::EStatus::InternalServerError:
                    return asio::buffer(INTERNAL_SERVER_ERROR);
                case TResponse::EStatus::NotImplemented:
                    return asio::buffer(NOT_IMPLEMENTED);
                case TResponse::EStatus::BadGateway:
                    return asio::buffer(BAD_GATEWAY);
                case TResponse::EStatus::ServiceUnavailable:
                    return asio::buffer(SERVICE_UNAVAILABLE);
                default:
                    return asio::buffer(INTERNAL_SERVER_ERROR);
            }
        }
    }

    namespace NMiscStrings {
        const char NAME_VALUE_SEPARATOR[]{ ':', ' ' };
        const char CRLF[]{'\r', '\n' };
    }

    namespace NStockResponse {
        const char OK[] = "";
        const char CREATED[] =
            "<html>"
            "<head><title>Created</title></head>"
            "<body><h1>201 Created</h1>";
        const char ACCEPTED[] =
            "<html>"
            "<head><title>Accepted</title></head>"
            "<body><h1>202 Accepted</h1>";
        const char NO_CONTENT[] =
            "<html>"
            "<head><title>No Content</title></head>"
            "<body><h1>204 Content</h1>";
        const char MULTIPLE_CHOICES[] =
            "<html>"
            "<head><title>Multiple Choices</title></head>"
            "<body><h1>300 Multiple Choices</h1>";
        const char MOVED_PERMANENTLY[] =
            "<html>"
            "<head><title>Moved Permanently</title></head>"
            "<body><h1>301 Moved Permanently</h1>";
        const char MOVED_TEMPORARILY[] =
            "<html>"
            "<head><title>Moved Temporarily</title></head>"
            "<body><h1>302 Moved Temporarily</h1>";
        const char NOT_MODIFIED[] =
            "<html>"
            "<head><title>Not Modified</title></head>"
            "<body><h1>304 Not Modified</h1>";
        const char BAD_REQUEST[] =
            "<html>"
            "<head><title>Bad Request</title></head>"
            "<body><h1>400 Bad Request</h1>";
        const char UNAUTHORIZED[] =
            "<html>"
            "<head><title>Unauthorized</title></head>"
            "<body><h1>401 Unauthorized</h1>";
        const char FORBIDDEN[] =
            "<html>"
            "<head><title>Forbidden</title></head>"
            "<body><h1>403 Forbidden</h1>";
        const char NOT_FOUND[] =
            "<html>"
            "<head><title>Not Found</title></head>"
            "<body><h1>404 Not Found</h1>";
        const char INTERNAL_SERVER_ERROR[] =
            "<html>"
            "<head><title>Internal Server Error</title></head>"
            "<body><h1>500 Internal Server Error</h1>";
        const char NOT_IMPLEMENTED[] =
            "<html>"
            "<head><title>Not Implemented</title></head>"
            "<body><h1>501 Not Implemented</h1>";
        const char BAD_GATEWAY[] =
            "<html>"
            "<head><title>Bad Gateway</title></head>"
            "<body><h1>502 Bad Gateway</h1>";
        const char SERVICE_UNAVAILABLE[] =
            "<html>"
            "<head><title>Service Unavailable</title></head>"
            "<body><h1>503 Service Unavailable</h1>";

        const char START_OF_MESSAGE[] = "<body><html>";
        const char END_OF_MESSAGE[] = "</body></html>";

        std::string ToString(TResponse::EStatus status) {
            switch (status) {
                case TResponse::EStatus::Ok:
                    return OK;
                case TResponse::EStatus::Created:
                    return CREATED;
                case TResponse::EStatus::Accepted:
                    return ACCEPTED;
                case TResponse::EStatus::NoContent:
                    return NO_CONTENT;
                case TResponse::EStatus::MultipleChoices:
                    return MULTIPLE_CHOICES;
                case TResponse::EStatus::MovedPermanently:
                    return MOVED_PERMANENTLY;
                case TResponse::EStatus::MovedTemporarily:
                    return MOVED_TEMPORARILY;
                case TResponse::EStatus::NotModified:
                    return NOT_MODIFIED;
                case TResponse::EStatus::BadRequest:
                    return BAD_REQUEST;
                case TResponse::EStatus::Unauthorized:
                    return UNAUTHORIZED;
                case TResponse::EStatus::Forbidden:
                    return FORBIDDEN;
                case TResponse::EStatus::NotFound:
                    return NOT_FOUND;
                case TResponse::EStatus::InternalServerError:
                    return INTERNAL_SERVER_ERROR;
                case TResponse::EStatus::NotImplemented:
                    return NOT_IMPLEMENTED;
                case TResponse::EStatus::BadGateway:
                    return BAD_GATEWAY;
                case TResponse::EStatus::ServiceUnavailable:
                    return SERVICE_UNAVAILABLE;
                default:
                    return INTERNAL_SERVER_ERROR;
            }
        }
    }

    TResponse TResponse::GetStockReply(TResponse::EStatus status, const std::string& message) {
        TResponse response(status);

        response.Content = NStockResponse::ToString(status);
        if (!message.empty()) {
            if (response.Content.empty()) {
                response.Content.append(NStockResponse::START_OF_MESSAGE);
            }

            response.Content.append("<p>").append(message).append("</p>");
        }
        response.Content.append(NStockResponse::END_OF_MESSAGE);

        response.Headers.resize(2);
        response.Headers[0].Name = "Content-Length";
        response.Headers[0].Value = std::to_string(response.Content.size());
        response.Headers[1].Name = "Content-Type";
        response.Headers[1].Value = "text/html";

        return response;
    }

    TResponse& TResponse::AddHeader(const std::string& name, const std::string& value) {
        Headers.emplace_back(name, value);
        return *this;
    }

    std::vector<asio::const_buffer> TResponse::ToBuffers() const {
        std::vector<asio::const_buffer> buffers;
        buffers.push_back(NStatusStrings::ToBuffer(Status));
        for (const THeader& header : Headers) {
            buffers.push_back(asio::buffer(header.Name));
            buffers.push_back(asio::buffer(NMiscStrings::NAME_VALUE_SEPARATOR));
            buffers.push_back(asio::buffer(header.Value));
            buffers.push_back(asio::buffer(NMiscStrings::CRLF));
        }
        buffers.push_back(asio::buffer(NMiscStrings::CRLF));
        buffers.push_back(asio::buffer(Content));

        return buffers;
    }

    bool TResponse::ContainsHeader(const std::string& name) {
        return Headers.end() != std::find_if(Headers.begin(), Headers.end(), [&name](const THeader& item) {
            return std::equal(item.Name.begin(), item.Name.end(), name.begin(), [](char a, char b) {
                return tolower(a) == tolower(b);
            });
        });
    }
}

#include "internal/bison_helpers.h"

#include <yandex/maps/idl/nodes/type_ref.h>

#include <boost/algorithm/string.hpp>

#include <unordered_map>

namespace yandex {
namespace maps {
namespace idl {
namespace parser {

void Errors::setLineNumberAndMessage(
    int lineNumber,
    const std::string& message)
{
    lineNumber_ = lineNumber;
    message_ = message;
}

void Errors::addWithContext(const std::string& context)
{
    std::string error = "line " + std::to_string(lineNumber_);
    if (!context.empty()) {
        error += ", " + context;
    }

    error += ": " + boost::algorithm::erase_all_copy(
        std::string(message_), "syntax error, ");

    errors_.push_back(error);
}

void Errors::handlePendingError()
{
    if (errors_.empty() && !message_.empty()) {
        addWithContext("");
    }
}

const std::vector<std::string>& Errors::errors() const
{
    return errors_;
}

void pushMoved(Scope& scope, std::string* ptr)
{
    if (ptr) {
        scope += *ptr;
        delete ptr;
    }
}

void addDocLink(nodes::DocBlock& docBlock, nodes::DocLink* link)
{
    if (link) {
        docBlock.links.push_back(std::move(*link));
        delete link;

        docBlock.format += '%' + std::to_string(docBlock.links.size()) + '%';
    }
}

void initializeTypeRefFromName(nodes::TypeRef& typeRef, Scope* nameParts)
{
    static const std::unordered_map<std::string, nodes::TypeId> LABELS {
        { "void",           nodes::TypeId::Void          },
        { "bool",           nodes::TypeId::Bool,         },
        { "int",            nodes::TypeId::Int           },
        { "uint",           nodes::TypeId::Uint          },
        { "int64",          nodes::TypeId::Int64         },
        { "float",          nodes::TypeId::Float         },
        { "double",         nodes::TypeId::Double        },
        { "string",         nodes::TypeId::String        },
        { "time_interval",  nodes::TypeId::TimeInterval  },
        { "abs_timestamp",  nodes::TypeId::AbsTimestamp  },
        { "rel_timestamp",  nodes::TypeId::RelTimestamp  },
        { "bytes",          nodes::TypeId::Bytes         },
        { "color",          nodes::TypeId::Color         },
        { "point",          nodes::TypeId::Point         },
        { "bitmap",         nodes::TypeId::Bitmap        },
        { "image_provider", nodes::TypeId::ImageProvider },
        { "animated_image_provider", nodes::TypeId::AnimatedImageProvider },
        { "model_provider", nodes::TypeId::ModelProvider },
        { "animated_model_provider", nodes::TypeId::AnimatedModelProvider },
        { "any",            nodes::TypeId::Any           },
        { "any_collection", nodes::TypeId::AnyCollection },
        { "platform_view",  nodes::TypeId::PlatformView  },
        { "view_provider",  nodes::TypeId::ViewProvider  }
    };

    const std::string name = nameParts->first();
    if ((nameParts->size() == 1) && (LABELS.find(name) != LABELS.end())) {
        typeRef.id = LABELS.at(name);
    } else {
        typeRef.id = nodes::TypeId::Custom;
        typeRef.name = *nameParts;
    }

    delete nameParts;
}

} // namespace parser
} // namespace idl
} // namespace maps
} // namespace yandex

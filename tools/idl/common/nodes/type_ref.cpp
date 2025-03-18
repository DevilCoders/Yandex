#include <yandex/maps/idl/nodes/type_ref.h>

#include <yandex/maps/idl/utils/exception.h>

#include <unordered_map>

namespace yandex {
namespace maps {
namespace idl {
namespace nodes {

namespace {

static const std::unordered_map<TypeId, std::string> TYPE_ID_LABELS{
    { TypeId::Void,                  "void"                    },
    { TypeId::Bool,                  "bool"                    },
    { TypeId::Int,                   "int"                     },
    { TypeId::Uint,                  "uint"                    },
    { TypeId::Int64,                 "int64"                   },
    { TypeId::Float,                 "float"                   },
    { TypeId::Double,                "double"                  },
    { TypeId::String,                "string"                  },
    { TypeId::TimeInterval,          "time_interval"           },
    { TypeId::AbsTimestamp,          "abs_timestamp"           },
    { TypeId::RelTimestamp,          "rel_timestamp"           },
    { TypeId::Bytes,                 "bytes"                   },
    { TypeId::Color,                 "color"                   },
    { TypeId::Point,                 "point"                   },
    { TypeId::Bitmap,                "bitmap"                  },
    { TypeId::ImageProvider,         "image_provider"          },
    { TypeId::AnimatedImageProvider, "animated_image_provider" },
    { TypeId::ModelProvider,         "model_provider"          },
    { TypeId::AnimatedModelProvider, "animated_model_provider" },
    { TypeId::Vector,                "vector"                  },
    { TypeId::Dictionary,            "dictionary"              },
    { TypeId::Any,                   "any"                     },
    { TypeId::AnyCollection,         "any_collection"          },
    { TypeId::PlatformView,          "platform_view"           },
    { TypeId::Custom,                "custom_type"             },
    { TypeId::ViewProvider,          "view_provider"           }
};

} // namespace

std::ostream& operator<<(std::ostream& out, TypeId id)
{
    auto iterator = TYPE_ID_LABELS.find(id);
    if (iterator == TYPE_ID_LABELS.end()) {
        return out << '#' << std::size_t(id);
    } else {
        return out << iterator->second;
    }
}

std::string typeRefToString(const TypeRef& typeRef)
{
    if (typeRef.id == TypeId::Custom) {
        return *typeRef.name;
    } else {
        REQUIRE(TYPE_ID_LABELS.find(typeRef.id) != TYPE_ID_LABELS.end(),
            "Couldn't recognize Idl type id: " << typeRef.id);

        const std::string& typeIdLabel = TYPE_ID_LABELS.at(typeRef.id);
        if (typeRef.id == TypeId::Vector) {
            return typeIdLabel + '<' +
                typeRefToString(typeRef.parameters[0]) + '>';
        } else if (typeRef.id == TypeId::Dictionary) {
            return typeIdLabel + '<' +
                typeRefToString(typeRef.parameters[0]) + ", " +
                typeRefToString(typeRef.parameters[1]) + '>';
        } else {
            return typeIdLabel;
        }
    }
}

} // namespace nodes
} // namespace idl
} // namespace maps
} // namespace yandex

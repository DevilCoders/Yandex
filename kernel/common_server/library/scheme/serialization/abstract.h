#pragma once
#include <kernel/common_server/library/scheme/scheme.h>
#include <kernel/common_server/library/scheme/fields.h>
#include <kernel/common_server/library/scheme/handler.h>

namespace NCS {
    namespace NScheme {
        class ISchemeSerializer {
        public:
            virtual ~ISchemeSerializer() {}
            virtual NJson::TJsonValue SerializeToJson(const TScheme& scheme) const = 0;
            virtual NJson::TJsonValue SerializeToJson(const THandlerSchemasCollection& /*schemasCollection*/) const {
                return NJson::JSON_NULL;
            }
        };
    }
}

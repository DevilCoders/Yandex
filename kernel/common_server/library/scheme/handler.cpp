#include "handler.h"
#include "serialization/opeanapi.h"
#include <build/scripts/c_templates/svnversion.h>
#include <util/system/progname.h>

namespace NCS {
    namespace NScheme {

        NJson::TJsonValue THandlerSchemasCollection::SerializeToJson() const {
            TOpenApiSerializer serializer;
            NJson::TJsonValue result = serializer.SerializeToJson(*this);
            result.InsertValue("openapi", "3.0.3");
            NJson::TJsonValue& info = result.InsertValue("info", NJson::JSON_MAP);
            info.InsertValue("title", GetProgramName());
            info.InsertValue("version", ::ToString(GetProgramSvnRevision()));
            return result;
        }

    }
}

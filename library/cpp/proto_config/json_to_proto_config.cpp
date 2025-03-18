#include "json_to_proto_config.h"

namespace NProtoConfig {
    const NProtobufJson::TJson2ProtoConfig JSON_2_PROTO_CONFIG =
        NProtobufJson::TJson2ProtoConfig()
            .SetMapAsObject(true)
            .SetAllowComments(true)
        ;
}

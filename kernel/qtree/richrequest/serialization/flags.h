#pragma once

enum EQtreeDeserializeMode {
    // Just read qtree from protobuf exactly
    QTREE_DEFAULT_DESERIALIZE = 0,
    // Try to restore qtree as it was in wizard before serialization
    // Note that there is not enough information for presize unserialization,
    // so sometimes the result may still differ. See also REQWIZARD-1173
    QTREE_UNSERIALIZE = 1,
};


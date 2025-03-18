#pragma once

#include <util/generic/yexception.h>

namespace NXmlRPC {
    struct TXmlRPCError: public yexception {
    };

    struct TTypeMismatch: public TXmlRPCError {
    };

    struct TTypeConversionError: public TTypeMismatch {
    };
}

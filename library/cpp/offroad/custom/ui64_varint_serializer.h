#pragma once

#include "varint_serializer.h"

namespace NOffroad {
    class TUi64VarintSerializer: public TVarintSerializer<ui64> {
    public:
        /* Convenience function that is not part of the interface,
         * but is still pretty damn convenient. */
        static void Write(ui64 value, IOutputStream* output) {
            ui8 tmp[MaxSize];
            output->Write(tmp, Serialize(value, tmp));
        }
    };

}

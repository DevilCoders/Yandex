#pragma once

#include <cstddef> /* For size_t. */

namespace NOffroad {
    namespace NPrivate {
        class TDecoderTable;
        class TEncoderTable;
    }

    template <class BaseTable>
    class TTable16;
    using TDecoder16Table = TTable16<NPrivate::TDecoderTable>;
    using TEncoder16Table = TTable16<NPrivate::TEncoderTable>;

    template <class BaseTable>
    class TTable64;
    using TDecoder64Table = TTable64<NPrivate::TDecoderTable>;
    using TEncoder64Table = TTable64<NPrivate::TEncoderTable>;

    template <size_t tupleSize, class BaseTable>
    class TInterleavedTable;

}

#pragma once

#include <util/generic/strbuf.h>
#include <util/digest/city.h>

namespace NQueryData {

    inline ui32 ShardNum(TStringBuf key, ui32 shards) {
        return CityHash64(key) % shards;
    }

}

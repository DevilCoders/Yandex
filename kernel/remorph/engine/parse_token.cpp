#include "parse_token.h"

namespace NReMorph {

size_t TParseTokenDataFormat::GetExtraOffset(size_t pos) const {
    return !ExtraOffsetMap.empty() ? ExtraOffsetMap[pos] : ExtraOffset;
}


} // NReMorph

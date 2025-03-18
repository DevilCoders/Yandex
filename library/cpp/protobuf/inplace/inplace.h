#pragma once

#include <library/cpp/protobuf/inplace/field_id_macro.h>
#include <library/cpp/protobuf/inplace/field_size_macro.h>
#include <library/cpp/protobuf/inplace/parser.h>
#include <library/cpp/protobuf/inplace/region_data_provider.h>
#include <library/cpp/protobuf/inplace/serialize_sizes.h>

namespace NInPlaceProto {

    using TRegionParser = TInplaceParser<TRegionDataProvider>;

} // namespace NInPlaceProto

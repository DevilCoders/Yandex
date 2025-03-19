#pragma once

#include <library/cpp/offroad/codec/interleaved_model.h>
#include <library/cpp/offroad/codec/interleaved_table.h>

namespace NDoom {


template <size_t keySize, size_t dataSize, class BaseModel>
using TStructDiffModel = NOffroad::TInterleavedModel<keySize + (dataSize + 31) / 32 + 2, BaseModel>;

template <size_t keySize, size_t dataSize, class BaseTable>
using TStructDiffTable = NOffroad::TInterleavedTable<keySize + (dataSize + 31) / 32 + 2, BaseTable>;

} // namespace NDoom

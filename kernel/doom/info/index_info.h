#pragma once

#include <util/generic/string.h>

#include <kernel/doom/info/index_info.pb.h>

namespace NDoom {

TIndexInfo MakeDefaultIndexInfo();

void SaveIndexInfo(IOutputStream* stream, const TIndexInfo& data);
TIndexInfo LoadIndexInfo(IInputStream* stream);
TIndexInfo LoadIndexInfo(const TString& indexPath, const TIndexInfo& defaultInfo = TIndexInfo());

} // namespace NDoom

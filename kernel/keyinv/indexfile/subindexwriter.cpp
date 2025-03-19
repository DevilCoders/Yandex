#include "memoryportion.h"

#include "subindexwriter.h"

namespace NIndexerCore::NIndexerDetail {

void Instance(TSubIndexWriter& writer, TOutputIndexFileImpl<TFile>& file, const TSubIndexInfo& info) {
    writer.WriteSubIndex(file, info, 0);
}

void Instance(TSubIndexWriter& writer, TOutputIndexFileImpl<NIndexerCore::NIndexerDetail::TOutputMemoryStream>& file, const TSubIndexInfo& info) {
    writer.WriteSubIndex(file, info, 0);
}

} // namespace NIndexerCore::NIndexerDetail

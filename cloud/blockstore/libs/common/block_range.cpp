#include "block_range.h"

#include <util/string/builder.h>
#include <util/string/cast.h>

namespace NCloud::NBlockStore {

////////////////////////////////////////////////////////////////////////////////

template <typename TBlockIndex>
bool TBlockRange<TBlockIndex>::TryParse(
    TStringBuf s,
    TBlockRange<TBlockIndex>& range)
{
    TStringBuf l, r;
    if (s.TrySplit(':', l, r)) {
        TBlockIndex start = 0, end = 0;
        if (TryFromString(l, start) && TryFromString(r, end) && start <= end) {
            range = TBlockRange<TBlockIndex>(start, end);
            return true;
        }
    } else {
        TBlockIndex blockIndex = 0;
        if (TryFromString(s, blockIndex)) {
            range = TBlockRange<TBlockIndex>(blockIndex);
            return true;
        }
    }
    return false;
}

template bool TBlockRange32::TryParse(TStringBuf s, TBlockRange32& range);
template bool TBlockRange64::TryParse(TStringBuf s, TBlockRange64& range);

////////////////////////////////////////////////////////////////////////////////

template <typename TBlockIndex>
TString DescribeRange(const TBlockRange<TBlockIndex>& blockRange)
{
    return TStringBuilder()
        << "[" << blockRange.Start << ".." << blockRange.End << "]";
}

template TString DescribeRange(const TBlockRange32& blockRange);
template TString DescribeRange(const TBlockRange64& blockRange);

template <typename TBlockIndex>
TString DescribeRange(const TVector<TBlockIndex>& blocks)
{
    if (blocks) {
        return TStringBuilder()
            << "[" << blocks.front() << ".." << blocks.back() << "]";
    }
    return "<none>";
}

template TString DescribeRange(const TVector<ui32>& blocks);
template TString DescribeRange(const TVector<ui64>& blocks);

}   // namespace NCloud::NBlockStore

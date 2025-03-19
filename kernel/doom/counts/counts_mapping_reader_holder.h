#pragma once

#include <kernel/search_types/search_types.h>
#include <kernel/doom/counts/counts_memory_index_reader.h>

#include <search/panther/indexing/operations/index_shard.h>

#include <kernel/doom/info/index_info.h>
#include <kernel/doom/info/info_index_reader.h>
#include <kernel/doom/adaptors/key_filtering_index_reader.h>

namespace NDoom {

class TCountsMappingReaderHolder {
public:
    using TReader = TInfoIndexReader<OffroadCountsIndexFormat, TKeyFilteringIndexReader<TLengthKeyFilter<MAXKEY_LEN>, TCountsMemoryIndexReader>>;

    TCountsMappingReaderHolder(THolder<NPanther::TTermMapping>&& terms, TVector<NPanther::TOrderedTermInfo>&& keys, NPanther::TCountsMapping&& interim, TIndexInfo&& indexInfo)
        : Terms_(std::forward<THolder<NPanther::TTermMapping>>(terms))
        , Keys_(std::forward<TVector<NPanther::TOrderedTermInfo>>(keys))
        , Interim_(std::forward<NPanther::TCountsMapping>(interim))
        , Reader_(std::forward<TIndexInfo>(indexInfo), Terms_.Get(), &Keys_, &Interim_)
    {
    }

    TReader* Get() {
        return &Reader_;
    }

private:
    THolder<NPanther::TTermMapping> Terms_;
    TVector<NPanther::TOrderedTermInfo> Keys_;
    NPanther::TCountsMapping Interim_;
    TReader Reader_;
};

} //namespace NDoom


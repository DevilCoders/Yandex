#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/memory/blob.h>

#include <functional>

namespace NQueryData {
    class TSourceFactors;
    class TFileDescription;
}

namespace NQueryDataSaaS {

    TString TrieMask2SaaSUrlMask(TString rawMask);

    using TOnQDTrieEntry = std::function<void(const NQueryData::TSourceFactors&)>;
    using TOnQDTrieMeta = std::function<void(const NQueryData::TFileDescription&)>;

    void ProcessQDTrie(TBlob trie, TOnQDTrieEntry onEntry, TOnQDTrieMeta onMeta);

    void ProcessQDTrieMeta(TBlob trie, TOnQDTrieMeta onMeta);

}

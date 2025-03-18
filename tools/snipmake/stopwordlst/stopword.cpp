#include "stopword.h"

#include <library/cpp/stopwords/stopwords.h>
#include <library/cpp/resource/resource.h>

#include <util/generic/string.h>
#include <util/stream/mem.h>

namespace NSnippets {
    void InitDefaultStopWordsList(TWordFilter& flt) {
        TString data = NResource::Find("/stopword");
        TMemoryInput mi(data.data(), data.size());
        flt.InitStopWordsList(mi);
    }
}

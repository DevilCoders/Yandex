#pragma once

#include "hit.h"

#include <library/cpp/offroad/custom/subtractors.h>
#include <library/cpp/offroad/custom/ui64_vectorizer.h>
#include <library/cpp/offroad/custom/ui32_vectorizer.h>
#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/tuple/tuple_writer.h>
#include <library/cpp/offroad/tuple/tuple_reader.h>
#include <library/cpp/offroad/tuple/tuple_sampler.h>
#include <library/cpp/offroad/flat/flat_writer.h>
#include <library/cpp/offroad/flat/flat_searcher.h>
#include <library/cpp/offroad/flat/flat_ui32_searcher.h>

#include <util/stream/output.h>
#include <util/system/types.h>

namespace NOffroad::NMinHash {

struct TMinHashIo {
    using TArithmeticProgressionWriter = TFlatWriter<TArithmeticProgressionHit, nullptr_t, TArithmeticProgressionVectorizer, TNullVectorizer>;
    using TArithmeticProgressionSearcher = TFlatSearcher<TArithmeticProgressionHit, nullptr_t, TArithmeticProgressionVectorizer, TNullVectorizer>;

    using TEmptySlotsWriter = TFlatWriter<ui32, nullptr_t, TUi32Vectorizer, TNullVectorizer>;
    using TEmptySlotsSearcher = TFlatUi32Searcher;
};

} // namespace NOffroad::NMinHash

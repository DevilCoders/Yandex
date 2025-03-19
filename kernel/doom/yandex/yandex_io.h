#pragma once

#include "yandex_reader.h"
#include "unsorted_yandex_reader.h"
#include "yandex_writer.h"

#include <kernel/doom/hits/superlong_hit.h>
#include <kernel/doom/hits/panther_hit.h>
#include <kernel/doom/hits/counts_hit.h>

namespace NDoom {

template<class Hit>
struct TGenericYandexIo {
    using TReader = TYandexReader<Hit>;
    using TWriter = TYandexWriter<Hit>;
};

template<class Hit>
struct TUnsortedGenericYandexIo {
    using TReader = TUnsortedYandexReader<Hit>;
};

using TYandexIo = TGenericYandexIo<TSuperlongHit>;
using TYandexPantherIo = TGenericYandexIo<TPantherHit>;
using TYandexCountsIo = TGenericYandexIo<TCountsHit>;

using TUnsortedYandexIo = TUnsortedGenericYandexIo<TSuperlongHit>;

} // namespace NDoom

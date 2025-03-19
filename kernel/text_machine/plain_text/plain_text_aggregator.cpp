#include "plain_text_aggregator.h"

#include <kernel/text_machine/plain_text/pt_aggregator_gen.cpp>

namespace NTextMachine {
namespace NPlainText {
    TPlainTextAggregatorPtr GetPlainTextAggregator(TMemoryPool& pool) {
        return TPlainTextAggregatorPtr(pool.New<TPlainTextAggregatorMachine>());
    }
}
}

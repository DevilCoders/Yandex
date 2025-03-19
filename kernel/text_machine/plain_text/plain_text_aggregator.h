#pragma once

#include <kernel/text_machine/parts/all.h>
#include <kernel/text_machine/plain_text/pt_aggregator_gen.h>

namespace NTextMachine {
namespace NPlainText {
    using TPlainTextAggregator = TPlainTextAggregatorMachine;
    using TPlainTextAggregatorPtr = THolder<TPlainTextAggregatorMachine, TDestructor>;

    TPlainTextAggregatorPtr GetPlainTextAggregator(TMemoryPool& pool);
} // NPlainText
} // NTextMachine

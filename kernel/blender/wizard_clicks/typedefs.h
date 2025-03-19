#pragma once

#include <kernel/blender/wizard_clicks/counters.sc.h>

#include <library/cpp/scheme/domscheme_traits.h>

namespace NWizardsClicks {
    using TCounterContainerWrap = NScProto::TCounterContainer<TSchemeTraits>;
    using TCounterContainerConstWrap = NScProto::TCounterContainerConst<TSchemeTraits>;

    using TWizardCountersWrap = NScProto::TWizardCounters<TSchemeTraits>;
    using TWizardCountersConstWrap = NScProto::TWizardCountersConst<TSchemeTraits>;

    using TNavigCountersWrap = NScProto::TNavigCounters<TSchemeTraits>;
    using TNavigCountersConstWrap = NScProto::TNavigCountersConst<TSchemeTraits>;

    using TCommonCountersWrap = NScProto::TCommonCounters<TSchemeTraits>;
    using TCommonCountersConstWrap = NScProto::TCommonCountersConst<TSchemeTraits>;

    using TVideoCountersWrap = NScProto::TVideoCounters<TSchemeTraits>;
    using TVideoCountersConstWrap = NScProto::TVideoCountersConst<TSchemeTraits>;

    using TSurplusCountersWrap = NScProto::TSurplusCounters<TSchemeTraits>;
    using TSurplusCountersConstWrap = NScProto::TSurplusCountersConst<TSchemeTraits>;

    using TBNACountersWrap = NScProto::TBNACounters<TSchemeTraits>;
    using TBNACountersConstWrap = NScProto::TBNACountersConst<TSchemeTraits>;
}

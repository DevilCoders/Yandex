#pragma once

#include "cbb_list_manager.h"
#include "cbb.h"

#include <antirobot/lib/regex_matcher.h>

#include <library/cpp/threading/future/future.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>


namespace NAntiRobot {


using TRegexMatcherAccessorPtr = TAtomicSharedPtr<NThreading::TRcuAccessor<TMaybe<TRegexMatcher>>>;


struct TCbbReListManagerRequester {
    TFuture<TString> operator()(
        ICbbIO* cbb,
        TCbbGroupId id
    ) const {
        return cbb->ReadList(id, "range_re");
    }
};


struct TCbbReListManagerCallback : public TCbbListManagerCallback {
    TRegexMatcherAccessorPtr Matcher = MakeAtomicShared<TRegexMatcherAccessorPtr::TValueType>();

    void operator()(const TVector<TString>& listStrs);
};


class TCbbReListManager : public TCbbListManager<
    TCbbReListManagerRequester,
    TCbbReListManagerCallback
> {
private:
    using TBase = TCbbListManager<TCbbReListManagerRequester, TCbbReListManagerCallback>;

public:
    TRegexMatcherAccessorPtr Add(TCbbGroupId id);

    using TBase::Remove;
};


} // namespace NAntiRobot

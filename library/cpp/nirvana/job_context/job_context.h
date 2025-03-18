#pragma once

#include <library/cpp/nirvana/job_context/job_context.sc.h>
#include <library/cpp/json/domscheme_traits.h>

#include <util/generic/maybe.h>
#include <util/string/builder.h>

namespace NNirvana {
    struct TJobContextException: public yexception {};

    namespace NPrivate {
        TMaybe<NJson::TJsonValue> ReadJson(const TString& filePath);
    }

    TString JobContextFilePath();

    template <template <class> class T>
    class TJobContextBase: public TMoveOnly {
    public:
        using TJobContextDomscheme = T<TJsonTraits>;

        TJobContextBase(NJson::TJsonValue&& json, bool strict = false);

        TJobContextBase(TJobContextBase&& rhs)
            : RawJobContext(std::move(rhs.RawJobContext))
            , JobContext(&RawJobContext)
        {
        }

        const TJobContextDomscheme& Context() const {
            return JobContext;
        }
        const NJson::TJsonValue& RawContext() const {
            return RawJobContext;
        }

    private:
        NJson::TJsonValue RawJobContext;
        TJobContextDomscheme JobContext;
    };

    template <template <class> class T>
    TJobContextBase<T>::TJobContextBase(NJson::TJsonValue&& json, bool strict /* = false*/)
        : RawJobContext(std::move(json))
        , JobContext(&RawJobContext)
    {
        TStringBuilder diag;
        if (!JobContext.Validate({}, strict, [&diag](TString lhs, TString rhs) {
                diag << lhs << ": " << rhs << Endl;
            })) {
            ythrow TJobContextException() << "Nirvana job context schema validation failed:\n"
                                          << diag;
        }
    }

    using TJobContext = TJobContextBase<TJobContextData>;
    using TMrJobContext = TJobContextBase<TMrJobContextData>;

    template <template <class> class T = TJobContextData>
    TMaybe<TJobContextBase<T>> LoadJobContext(bool strict = false, const TString& filePath = JobContextFilePath()) {
        auto json = NPrivate::ReadJson(filePath);
        if (!json) {
            return {};
        }

        TJobContextBase<T> result(std::move(*json), strict);
        return std::move(result);
    }
}

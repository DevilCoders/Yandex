#pragma once

#include <library/cpp/scheme/scheme.h>

namespace NExtendedMx {

    // owning class for domscheme
    template <typename TSchemeProto>
    class TBasedOn {
    public:
        TBasedOn(NSc::TValue scheme)
            : Scheme_(scheme)
            , Proto_(&Scheme_)
        {
        }

        TBasedOn(const TBasedOn& other)
            : Scheme_(other.Scheme_.Clone())
            , Proto_(&Scheme_)
        {}

        TBasedOn(TBasedOn&& other)
            : Scheme_(other.Scheme_)
            , Proto_(&Scheme_)
        {}

        TSchemeProto& Scheme() {
            return Proto_;
        }

        const TSchemeProto& Scheme() const {
            return Proto_;
        }

        void ValidateProtoThrow() const {
            TBasedOn<TSchemeProto>::ValidateProtoThrow(Scheme_);
        }

        static void ValidateProtoThrow(const NSc::TValue& scheme) {
            TSchemeProto(&scheme).Validate("", false, [](const TString& path, const TString& msg) {
                    ythrow yexception() << "   " << path << " " << msg << "\n";
            });
        }

    private:
        NSc::TValue Scheme_;
        TSchemeProto Proto_;
    };

} // NExtendedMx

#pragma once

#include <util/generic/fwd.h>
#include <util/generic/ptr.h>
#include <util/generic/strbuf.h>
#include <util/string/cast.h>

class IInputStream;

class TFuidChecker {
public:
    struct TFuid {
        ui32 Time;
        ui32 Rand;

        inline TString ToUid() const noexcept {
            return ToString((ui64)Rand) + ToString((ui64)Time);
        }
    };

    TFuidChecker(const TString& file);
    TFuidChecker(IInputStream& key);

    ~TFuidChecker();

    bool Parse(const TStringBuf& data, TFuid& res) const;

    inline bool VerifyFuid(const TStringBuf& data, TString* res) const {
        TFuid fuid;

        if (Parse(data, fuid)) {
            *res = fuid.ToUid();

            return true;
        }

        return false;
    }

private:
    class TImpl;
    THolder<TImpl> Impl_;
};

const TFuidChecker& FuidChecker();

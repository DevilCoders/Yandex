#pragma once
#include "key_provider.h"
#include <util/generic/vector.h>
#include <util/datetime/base.h>

namespace NTurboLogin {

    TString Sign(
        const TVector<ui8>& key,
        TStringBuf data,
        TInstant now
    );

    TString Sign(
        const TKeyProvider* keyProvider,
        TStringBuf data,
        TInstant now
    );

    bool ValidateSign(
        const TVector<ui8>& key,
        TStringBuf data,
        TStringBuf sign,
        TInstant now,
        TDuration maxAge = TDuration::Days(3)
    );

    bool ValidateSign(
        const TKeyProvider* keyProvider,
        TStringBuf data,
        TStringBuf sign,
        TInstant now,
        TDuration maxAge = TDuration::Days(3)
    );
}

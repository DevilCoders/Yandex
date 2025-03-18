#pragma once

#include <library/cpp/json/writer/json_value.h>

#include <util/datetime/base.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>


namespace NHypocrisy {


struct TBundle;


struct TFingerprintData {
    NJson::TJsonValue Fingerprint;
    TInstant FingerprintTimestamp; // Timestamp of fingerprint collection,
    TInstant InstanceTimestamp; // Timestamp of Greed.js generation.

    static TMaybe<TFingerprintData> Decrypt(TStringBuf key, TStringBuf s);

    bool IsStale(
        const TBundle& bundle,
        TDuration instanceLeeway,
        TDuration fingerprintLifetime,
        TInstant now = TInstant::Now()
    ) const;
};


struct TBundle {
    TInstant PrevGenerationTime;
    TInstant GenerationTime;
    TString DescriptorKey;
    TVector<TString> Instances;

    static TBundle Load(const TString& path);
};


} // namespace NHypocrisy

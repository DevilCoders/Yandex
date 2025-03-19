#pragma once

#include <kernel/hosts/owner/owner.h>
#include <kernel/querydata/server/qd_server.h>

#include <util/generic/fwd.h>

namespace NFacts {

    class TCredibilityDatabase {
    public:
        static constexpr float DefaultKernelScore = 0.5f;

    public:
        enum class ECredibilityMark : int {
            Unknown = -1,
            NonCredible = 0,
            Credible = 1
        };

        struct TCredibilityData {
            ECredibilityMark CredibilityMark = ECredibilityMark::Unknown;
            float KernelScore = DefaultKernelScore;
        };

    public:
        TCredibilityDatabase(
            const TString& hostTrieFileName,
            const TString& maskTrieFileName,
            const TString& urlTrieFileName
        );

        TCredibilityDatabase(THolder<NQueryData::TQueryDatabase>&& queryDatabase);

        TCredibilityData FindCredibility(const TString& url, const TString& yandexTld) const;

    private:
        THolder<NQueryData::TQueryDatabase> QueryDatabase;
        TOwnerCanonizer Canonizer;
    };

};

#pragma once

#include <library/cpp/json/json_value.h>

#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>

#include "api.h"
#include "share_object.h"

namespace NSkyboneD {

class TSkyShare {
public:
    TSkyShare(const TString& tvmTicket,
              const NSkyboneD::NApi::TApiConfig& config);

    TShareObjectPtr AddFile(TString path = "",
                            TString httpLink = "",
                            bool executable = false);

    // Unique source id.
    // You won't be able to delete your rbtorrent without providing same source id.
    // Use it if you are unsure if your rbtorrent will be unique
    // Use it if torrent is too small
    // Don't use it if you can't save source id to your own database for later deletion
    void SetSourceId(TString sourceId);

    // non-empty directories will be created automatically
    void AddEmptyDirectory(TString path);

    void AddSymlink(TString target, TString linkName);

    // throws on any incomplete file
    // uses exponential backoff by rules from TSkyShareManager
    TString FinalizeAndGetRBTorrent();

    // throws on any incomplete file
    TString GetTemporaryRBTorrentWithoutSkybonedRegistration();

private:
    void GenerateApiRequest();

    TVector<TShareObjectPtr> Files_;
    TVector<TString> EmptyDirs_;
    TVector<std::pair<TString, TString>> Links_;

    NJson::TJsonValue ApiRequest_;
    TString SourceId_;

    const TString TvmHeader_;
    const NSkyboneD::NApi::TApiConfig& ApiConfig_;
};
using TSkySharePtr = THolder<TSkyShare>;

}

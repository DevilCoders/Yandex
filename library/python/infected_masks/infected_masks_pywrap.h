#pragma once

#include <library/cpp/infected_masks/infected_masks.h>

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/str.h>

// A list of data strings associated with a mask.
typedef std::vector<std::string> MatchData;

// A single match: (mask, data_list).
typedef std::pair<std::string, MatchData> MatchItem;

// A list of matched masks with data.
typedef std::vector<MatchItem> MatchResult;

class BaseMatcher {
public:
    BaseMatcher() {
    }

    bool IsInfectedUrl(const TString& url) {
        return SafeBrowsingMasks_->IsInfectedUrl(url);
    }

    MatchResult FindAll(const TString& url);

    // Returns a state for pickling.
    TString __getstate__() {
        if (!SafeBrowsingMasks_) {
            throw yexception() << "Use of uninitialized BaseMatcher";
        }
        TString data;
        TStringOutput dataStream(data);
        SafeBrowsingMasks_->Save(&dataStream);
        return data;
    }

    // This is not a real __setstate__. A real one is written in Python and
    // defined in infected_masks.swg.
    void _setstate(TString const& state) {
        SafeBrowsingMasks_.Reset(new TSafeBrowsingMasks());
        TStringInput dataStream(state);
        SafeBrowsingMasks_->Load(&dataStream);
    }

private:
    explicit BaseMatcher(TSafeBrowsingMasks* safeBrowsingMasks)
        : SafeBrowsingMasks_(safeBrowsingMasks)
    {
    }

    THolder<TSafeBrowsingMasks> SafeBrowsingMasks_;

    friend class BaseMatcherMaker;
};

class BaseMatcherMaker {
public:
    BaseMatcherMaker()
        : SafeBrowsingMasks_(MakeHolder<TSafeBrowsingMasks>())
        , Initializer_(SafeBrowsingMasks_->GetInitializer())
    {
    }

    void AddMask(const TString& mask, const TString& data) {
        EnsureInitializer();
        Initializer_->AddMask(mask, data);
    }

    BaseMatcher* GetMatcher() {
        EnsureInitializer();
        Initializer_->Finalize();
        Initializer_.Destroy();
        return new BaseMatcher(SafeBrowsingMasks_.Release());
    }

private:
    THolder<TSafeBrowsingMasks> SafeBrowsingMasks_;
    THolder<TSafeBrowsingMasks::IInitializer> Initializer_;

    void EnsureInitializer() {
        if (!Initializer_) {
            throw yexception()
                << "BaseMatcherMaker may only be used once to build a BaseMatcher";
        }
    }
};

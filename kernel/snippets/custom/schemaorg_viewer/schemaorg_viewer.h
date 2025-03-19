#pragma once

#include <kernel/tarc/docdescr/docdescr.h>

#include <library/cpp/langs/langs.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NSchemaOrg {
    class TOffer;
    class TMovie;
    class TSozlukComments;
    class TYoutubeChannel;
    class TCreativeWork;
    class TQuestion;
    class TSoftwareApplication;
    class TRating;
}

namespace NSnippets {
    class IArchiveViewer;
    class TConfig;

    class TSchemaOrgArchiveViewer {
        class TImpl;
        THolder<TImpl> Impl;
    public:
        TSchemaOrgArchiveViewer(const TConfig& cfg, const TDocInfos& docInfos, const TString& url, ELanguage lang);
        ~TSchemaOrgArchiveViewer();
        IArchiveViewer& GetViewer();
        const NSchemaOrg::TOffer* GetOffer() const;
        const NSchemaOrg::TMovie* GetMovie() const;
        const NSchemaOrg::TSozlukComments* GetEksisozlukComments() const;
        const NSchemaOrg::TYoutubeChannel* GetYoutubeChannel() const;
        const NSchemaOrg::TCreativeWork* GetCreativeWork() const;
        const NSchemaOrg::TQuestion* GetQuestion() const;
        const NSchemaOrg::TSoftwareApplication* GetSoftwareApplication() const;
        const NSchemaOrg::TRating* GetRating() const;
    };
}

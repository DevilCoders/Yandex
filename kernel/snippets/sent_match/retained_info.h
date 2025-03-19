#pragma once

#include "sent_match.h"

#include <kernel/snippets/archive/view/storage.h>
#include <kernel/snippets/config/enums.h>
#include <kernel/snippets/sent_info/sent_info.h>

#include <library/cpp/langs/langs.h>

#include <util/generic/list.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>

namespace NSnippets {
    class TConfig;
    class TQueryy;
    class TArchiveMarkup;
    class TArchiveView;
    class TSnip;

    class TRetainedSentsMatchInfo
    {
    public:
        class TParams {
        public:
            const TConfig& Cfg;
            const TQueryy& Query;
            bool PutDot = false;
            bool ParaTables = false;
            ELanguage DocLangId = LANG_UNK;
            const TArchiveView* MetaDescrAdd = nullptr;

        public:
            TParams(const TConfig& cfg, const TQueryy& query)
              : Cfg(cfg)
              , Query(query)
            {
            }

            TParams& SetPutDot(bool putDot = true) {
                PutDot = putDot;
                return *this;
            }

            TParams& SetParaTables(bool paraTables = true) {
                ParaTables = paraTables;
                return *this;
            }

            TParams& SetLang(ELanguage docLangId) {
                DocLangId = docLangId;
                return *this;
            }

            TParams& SetMetaDescrAdd(const TArchiveView* metaDescrAdd) {
                MetaDescrAdd = metaDescrAdd;
                return *this;
            }
        };
    private:
        void CreateSentsMatchInfo(const TArchiveMarkup* markup, const TArchiveView& view, const TParams& params);

    private:
        TSimpleSharedPtr<TArchiveStorage> Storage;
        TSimpleSharedPtr<const TSentsInfo> SentsInfo;
        TSimpleSharedPtr<const TSentsMatchInfo> SentsMatchInfo;

    public:
        TRetainedSentsMatchInfo();
        ~TRetainedSentsMatchInfo();
        void SetView(const TUtf16String& source, const TParams& params);
        void SetView(const TVector<TUtf16String>& source, const TParams& params);
        void SetView(const TArchiveMarkup* markup, const TArchiveView& view, const TParams& params);
        const TSentsInfo* GetSentsInfo() const {
            return SentsInfo.Get();
        }
        const TSentsMatchInfo* GetSentsMatchInfo() const {
            return SentsMatchInfo.Get();
        }
        TSnip AllAsSnip() const;
    };

    class TCustomSnippetsStorage
    {
    private:
        TList<TRetainedSentsMatchInfo> RetainedInfos;
    public:
        TCustomSnippetsStorage();
        ~TCustomSnippetsStorage();
        TRetainedSentsMatchInfo& CreateRetainedInfo();
        void RetainCopy(const TRetainedSentsMatchInfo& retainedInfo);
    };
}

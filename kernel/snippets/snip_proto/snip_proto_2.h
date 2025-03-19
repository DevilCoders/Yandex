#pragma once

#include <kernel/snippets/custom/schemaorg_viewer/schemaorg_viewer.h>

#include <util/generic/string.h>

namespace NSnippets
{

    class TSentsMatchInfo;
    class TSnip;
    class TSnipTitle;
    class TConfig;
    class TLengthChooser;
    class TWordSpanLen;
    struct ISnippetsCallback;
    class TTopCandidateCallback;

    TSnip GetBestSnip(
            const TSentsMatchInfo& sentsMatchInfo
            , const TSnipTitle& unnaturalTitle
            , const TConfig& cfg
            , const TString& url
            , const TLengthChooser& lenCfg
            , const TWordSpanLen& wordSpanLen
            , ISnippetsCallback& callback
            , TTopCandidateCallback* fsCallback
            , bool isByLink
            , float maxLenMultiplier = 1.0
            , bool dontGrow = false
            , TSnip* oneFragmentSnip = nullptr
            , const TSchemaOrgArchiveViewer* schemaOrgViewer = nullptr
    );
    TSnip GetBestSnip(
            const TSentsMatchInfo& sentsMatchInfo
            , const TSnipTitle& unnaturalTitle
            , const TConfig& cfg
            , const TString& url
            , const TLengthChooser& lenCfg
            , const TWordSpanLen& wordSpanLen
            , ISnippetsCallback& callback
            , bool isByLink
            , float maxLenMultiplier = 1.0
            , bool dontGrow = false
            , TSnip* oneFragmentSnip = nullptr
            , const TSchemaOrgArchiveViewer* schemaOrgViewer = nullptr
    );



}

#pragma once

#include <kernel/snippets/titles/make_title/make_title.h>
#include <kernel/snippets/titles/make_title/util_title.h>

#include <library/cpp/langs/langs.h>
#include <kernel/tarc/docdescr/docdescr.h>
#include <kernel/snippets/urlmenu/common/common.h>

namespace NSnippets {

struct ISnippetsCallback;
class TForumMarkupViewer;
class TMetaDescription;

class TSnipTitleSupplementer {
private:
    const TConfig& Cfg;
    const TQueryy& Query;
    TMakeTitleOptions Options;
    TMakeTitleOptions ForbidCuttingOptions;
    ELanguage Lang;
    const TString& Url;
    TUrlMenuVector ArcMenu;
    const TDefinition NaturalTitleDefinition;
    bool QuestionTitle;
    ISnippetsCallback& Callback;

public:
    typedef bool TTransformFunc(TUtf16String&);

    TSnipTitleSupplementer(const TConfig& cfg, const TQueryy& query, const TMakeTitleOptions& options, ELanguage lang,
                           const TString& url, const TString& urlmenu, const TDefinition& naturalTitleDefinition, bool questionTitles, ISnippetsCallback& callback);
    void AddForumToTitle(TSnipTitle& res, const TForumMarkupViewer& arcViewer);
    void AddCatalogToTitle(TSnipTitle& res);
    void AddUrlmenuToTitle(TSnipTitle& res);
    void AddHostNameToUrlTitle(TSnipTitle& res);
    void AddPopularHostNameToTitle(TSnipTitle& res);
    void AddDefinitionToNewsTitle(TSnipTitle& res);
    void AddSplittedHostNameToTitle(TSnipTitle& res, const TSentsInfo* sentsInfo);
    void GenerateUrlBasedTitle(TSnipTitle& res, const TSentsInfo* sentsInfo);
    void GenerateBNATitle(TSnipTitle& res);
    void GenerateMetaDescriptionBasedTitle(TSnipTitle& res, const TMetaDescription& metaDescr);
    void GenerateTwoLineTitle(TSnipTitle& res);
    void AddUrlRegionToTitle(TSnipTitle& res);
    void AddUserRegionToTitle(TSnipTitle& res);
    void AddHostDefinitionToTitle(TSnipTitle& res);
    void EliminateDefinitionFromTitle(TSnipTitle& res);
    void GenerateOpenGraphBasedTitle(TSnipTitle& res, const TDocInfos& docInfos);
    void EraseBadSymbolsFromTitle(TSnipTitle& res);
    void CapitalizeWordTitleLetters(TSnipTitle& res);
    void TransformTitle(TSnipTitle& res, TTransformFunc transformFunc);
};

}

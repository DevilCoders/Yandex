#pragma once

#include <library/cpp/langs/langs.h>
#include <util/generic/string.h>
#include <util/generic/ptr.h>

namespace NSnippets {

class TConfig;
class TArchiveStorage;
class TArchiveMarkup;
class TArchiveView;
class TMetadataViewer;
class TTrashViewer;
class TListArcViewer;
class TTableArcViewer;
class TForumMarkupViewer;
class TStatAnnotViewer;
class TContentPreviewViewer;
class THeaderViewer;
class TSchemaOrgArchiveViewer;
class TEntityViewer;
class TQueryy;
class TArcManip;
struct ISnippetsCallback;

class TSentHandler {
private:
    struct TImpl;
    THolder<TImpl> Impl;

public:
    TSentHandler(const TConfig& cfg, TArchiveStorage& storage, TArchiveMarkup& markup, const TQueryy* queryy,
        TArcManip& actx, ISnippetsCallback* callback, const TString& url, ELanguage lang);

    ~TSentHandler();

    bool DoUnpack(bool doText, bool doLink);

    const TArchiveView& GetTextView() const;
    const TArchiveView& GetExtendedTextView() const;
    const TArchiveView& GetLinkView() const;

    const TMetadataViewer& GetMetadataViewer() const;
    const TTrashViewer& GetTrashViewer() const;
    const TListArcViewer& GetListViewer() const;
    const TTableArcViewer& GetTableViewer() const;
    const TForumMarkupViewer& GetForumsViewer() const;
    const TStatAnnotViewer& GetStatAnnotViewer() const;
    const TContentPreviewViewer& GetContentPreviewViewer() const;
    const THeaderViewer& GetHeaderViewer() const;
    const TSchemaOrgArchiveViewer& GetSchemaOrgViewer() const;
    const TEntityViewer& GetEntityViewer() const;
};

}

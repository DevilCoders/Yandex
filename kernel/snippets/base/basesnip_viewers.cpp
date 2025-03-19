#include "basesnip_viewers.h"

#include <kernel/snippets/archive/markup/markup.h>
#include <kernel/snippets/archive/unpacker/unpacker.h>
#include <kernel/snippets/archive/view/storage.h>
#include <kernel/snippets/archive/view/view.h>
#include <kernel/snippets/archive/viewers.h>

#include <kernel/snippets/config/config.h>

#include <kernel/snippets/custom/entity_viewer.h>
#include <kernel/snippets/custom/forums_handler/forums_handler.h>
#include <kernel/snippets/custom/list_handler.h>
#include <kernel/snippets/custom/preview_viewer/preview_viewer.h>
#include <kernel/snippets/custom/schemaorg_viewer/schemaorg_viewer.h>
#include <kernel/snippets/custom/table_handler.h>
#include <kernel/snippets/custom/trash_viewer.h>

#include <kernel/snippets/iface/archive/manip.h>

#include <kernel/snippets/qtree/query.h>
#include <kernel/snippets/sent_match/callback.h>
#include <kernel/snippets/static_annotation/static_annotation.h>
#include <kernel/snippets/titles/header_based.h>

namespace NSnippets {

struct TSentHandler::TImpl {
public:
    const NSnippets::TConfig& Cfg;
    TArchiveStorage& Storage;
    TArchiveMarkup& Markup;
    TFirstAndHitSentsViewer TextHitViewer;
    TFirstAndHitSentsViewer LinkHitViewer;
    TStatAnnotViewer StatAnnotViewer;
    TContentPreviewViewer ContentPreviewViewer;
    THeaderViewer HeaderViewer;
    TForumMarkupViewer ForumsViewer;
    TTrashViewer TrashViewer;
    TListArcViewer ListViewer;
    TTableArcViewer TableViewer;
    TSchemaOrgArchiveViewer SchemaOrgViewer;
    TEntityViewer EntityViewer;

    IArchiveViewer* TextCallback;
    IArchiveViewer* LinkCallback;

private:
    TArcManip& ArcCtx;

public:
    TImpl(const TConfig& cfg, TArchiveStorage& storage, TArchiveMarkup& markup, const TQueryy* queryy,
        TArcManip& actx, ISnippetsCallback* callback, const TString& url, ELanguage lang)
        : Cfg(cfg)
        , Storage(storage)
        , Markup(markup)
        , TextHitViewer(false, Cfg, queryy, markup)
        , LinkHitViewer(true, Cfg, queryy, markup)
        , StatAnnotViewer(cfg, markup)
        , ContentPreviewViewer(cfg, *actx.GetTextArc().GetDocInfosPtr())
        , HeaderViewer()
        , ForumsViewer(cfg.UseForumReplacer(), *actx.GetTextArc().GetDocInfosPtr())
        , TrashViewer(markup)
        , ListViewer()
        , TableViewer()
        , SchemaOrgViewer(cfg, *actx.GetTextArc().GetDocInfosPtr(), url, lang)
        , EntityViewer(TextHitViewer.GetMetadataViewer())
        , TextCallback(callback->GetTextHandler(false))
        , LinkCallback(callback->GetTextHandler(true))
        , ArcCtx(actx)
    {
        TextHitViewer.SetForumMarkupViewer(ForumsViewer);
    }

    bool DoUnpack(bool doText, bool doLink) {
        if (doText) {
            TUnpacker unpacker(Cfg, &Storage, &Markup, ARC_TEXT);
            unpacker.AddRequester(&TextHitViewer);
            unpacker.AddRequester(&StatAnnotViewer.GetViewer());
            unpacker.AddRequester(&ContentPreviewViewer.GetViewer());
            unpacker.AddRequester(&HeaderViewer);
            unpacker.AddRequester(&ForumsViewer);
            if (TextCallback) {
                unpacker.AddRequester(TextCallback);
            }
            unpacker.AddRequester(&TrashViewer.GetViewer());
            if (Cfg.UseListSnip()) {
                unpacker.AddRequester(&ListViewer);
            }
            if (Cfg.UseTableSnip()) {
                unpacker.AddRequester(&TableViewer);
            }
            unpacker.AddRequester(&SchemaOrgViewer.GetViewer());
            if (Cfg.HasEntityClassRequest()) {
                unpacker.AddRequester(&EntityViewer.GetViewer());
            }

            if (ArcCtx.GetUnpText(unpacker, false) != 0) {
                return false;
            }
        }

        if (doLink) {
            TUnpacker unpacker(Cfg, &Storage, &Markup, ARC_LINK);
            unpacker.AddRequester(&LinkHitViewer);
            if (LinkCallback) {
                unpacker.AddRequester(LinkCallback);
            }
            if (ArcCtx.GetUnpText(unpacker, true) != 0) {
                return false;
            }
        }

        return true;
    }
};

TSentHandler::TSentHandler(const TConfig& cfg, TArchiveStorage& storage, TArchiveMarkup& markup, const TQueryy* queryy,
    TArcManip& actx, ISnippetsCallback* callback, const TString& url, ELanguage lang)
  : Impl(new TImpl(cfg, storage, markup, queryy, actx, callback, url, lang))
{
}

TSentHandler::~TSentHandler() {
}

bool TSentHandler::DoUnpack(bool doText, bool doLink) {
    return Impl->DoUnpack(doText, doLink);
}

const TEntityViewer& TSentHandler::GetEntityViewer() const {
    return Impl->EntityViewer;
}

const TArchiveView& TSentHandler::GetTextView() const {
    return Impl->TextHitViewer.GetResult();
}

const TArchiveView& TSentHandler::GetExtendedTextView() const {
    return Impl->TextHitViewer.GetExtendedTextResult();
}

const TArchiveView& TSentHandler::GetLinkView() const {
    return Impl->LinkHitViewer.GetResult();
}

const TMetadataViewer& TSentHandler::GetMetadataViewer() const {
    return Impl->TextHitViewer.GetMetadataViewer();
}

const TTrashViewer& TSentHandler::GetTrashViewer() const {
    return Impl->TrashViewer;
}

const TListArcViewer& TSentHandler::GetListViewer() const {
    return Impl->ListViewer;
}

const TTableArcViewer& TSentHandler::GetTableViewer() const {
    return Impl->TableViewer;
}

const TForumMarkupViewer& TSentHandler::GetForumsViewer() const {
    return Impl->ForumsViewer;
}

const TStatAnnotViewer& TSentHandler::GetStatAnnotViewer() const {
    return Impl->StatAnnotViewer;
}

const TContentPreviewViewer& TSentHandler::GetContentPreviewViewer() const {
    return Impl->ContentPreviewViewer;
}

const THeaderViewer& TSentHandler::GetHeaderViewer() const {
    return Impl->HeaderViewer;
}

const TSchemaOrgArchiveViewer& TSentHandler::GetSchemaOrgViewer() const {
    return Impl->SchemaOrgViewer;
}

}

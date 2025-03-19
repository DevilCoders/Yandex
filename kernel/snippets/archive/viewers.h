#pragma once

#include "metadata_viewer.h"

#include <kernel/snippets/archive/unpacker/unpacker.h>
#include <kernel/snippets/archive/view/view.h>
#include <kernel/snippets/archive/view/order.h>

#include <kernel/snippets/iface/archive/viewer.h>

namespace NSnippets {

class TQueryy;
class TConfig;
class TArchiveMarkup;
class TForumMarkupViewer;

class TFirstAndHitSentsViewer : public IArchiveViewer {
private:
    TUnpacker* Unpacker = nullptr;
    TMetadataViewer MetadataViewer;
    const bool IsLinkArc = false;
    TVector<ui16> HitSents;
    const NSnippets::TConfig& Cfg;
    const NSnippets::TQueryy* Query = nullptr;
    TForumMarkupViewer* ForumMarkupViewer = nullptr;
    TArchiveMarkup& Markup;

    TSentsOrder HitOrder;
    TSentsOrder NearHitOrder;
    TSentsOrder ExtNearHitOrder;
    TSentsOrderGenerator HitGen;
    TSentsOrderGenerator NearHitGen;
    TSentsOrderGenerator ExtNearHitGen;

    TArchiveView Result;
    TArchiveView ExtResult;

    int GetUnpackAfterHit(bool link);
    int GetUnpackBeforeHit(bool link);
    int GetExtsnipAdditionalSents(bool link);
public:
    TFirstAndHitSentsViewer(bool isLinkArc, const NSnippets::TConfig& cfg, const NSnippets::TQueryy* query, TArchiveMarkup& markup);

    void SetForumMarkupViewer(TForumMarkupViewer& forumMarkupViewer) {
        ForumMarkupViewer = &forumMarkupViewer;
    }

    void OnUnpacker(TUnpacker* unpacker) override {
        MetadataViewer.OnUnpacker(unpacker);
        Unpacker = unpacker;
    }
    void OnHitsAndSegments(const TVector<ui16>&, const NSegments::TSegmentsInfo*) override;
    void OnMarkup(const TArchiveMarkupZones& zones) override {
        MetadataViewer.OnMarkup(zones);
        OnTelephoneMarkup(zones);
    }
    void OnBeforeSents() override;

    const TMetadataViewer& GetMetadataViewer() const {
        return MetadataViewer;
    }

    const TSentsOrder& GetHitOrder() const {
        return HitOrder;
    }

    const TSentsOrder& GetNearHitOrder() const {
        return NearHitOrder;
    }

    void OnEnd() override;

    const TArchiveView& GetResult() const {
        return Result;
    }

    const TArchiveView& GetExtendedTextResult() const {
        return ExtResult;
    }

private:
    void OnTelephoneMarkup(const TArchiveMarkupZones& markupZones);
};
}

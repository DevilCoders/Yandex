#include "metadata_viewer.h"

#include <kernel/snippets/archive/unpacker/unpacker.h>

#include <kernel/tarc/iface/tarcface.h>

#include <util/generic/vector.h>

namespace NSnippets {
    void TMetadataViewer::OnMarkup(const TArchiveMarkupZones& zones) {
        const TVector<TArchiveZoneSpan>& title = zones.GetZone(AZ_TITLE).Spans;
        const TVector<TArchiveZoneSpan>& meta = zones.GetZone(AZ_ABSTRACT).Spans;
        if (!title.empty()) {
            if (!MultiTitles) {
                Title.PushBack(title[0].SentBeg, title[0].SentEnd);
            } else {
                for (size_t i = 0; i < title.size(); ++i) {
                    Title.PushBack(title[i].SentBeg, title[i].SentEnd);
                }
            }
        }
        if (!meta.empty()) {
            Meta.PushBack(meta[0].SentBeg, meta[0].SentEnd);
        }
        Unpacker->AddRequest(Title);
        Unpacker->AddRequest(Meta);
    }
}

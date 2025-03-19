#include "entity_viewer.h"

#include <kernel/snippets/archive/unpacker/unpacker.h>
#include <kernel/snippets/archive/viewers.h>
#include <kernel/snippets/archive/view/view.h>
#include <kernel/snippets/archive/view/order.h>
#include <kernel/snippets/iface/archive/viewer.h>

namespace NSnippets {
    struct TEntityViewer::TImpl : IArchiveViewer {
        const TMetadataViewer& MetadataViewer;
        TUnpacker* Unpacker = nullptr;
        TSentsOrder EntityClassifyOrder;
        TArchiveView Result;

        TImpl(const TMetadataViewer& metadataViewer)
          : MetadataViewer(metadataViewer)
        {
        }
        void OnUnpacker(TUnpacker* unpacker) override {
            Unpacker = unpacker;
        }
        void OnBeforeSents() override {
           EntityClassifyOrder.PushBack(1, 5000);
           TSentsOrderGenerator::Cutoff(MetadataViewer.Title, EntityClassifyOrder);
           TSentsOrderGenerator::Cutoff(MetadataViewer.Meta, EntityClassifyOrder);
           Unpacker->AddRequest(EntityClassifyOrder);
        }
        void OnEnd() override {
            DumpResult(EntityClassifyOrder, Result);
        }
    };

    TEntityViewer::TEntityViewer(const TMetadataViewer& metadataViewer)
      : Impl(new TImpl(metadataViewer))
    {
    }

    TEntityViewer::~TEntityViewer() {
    }

    IArchiveViewer& TEntityViewer::GetViewer() {
        return *Impl;
    }

    const TArchiveView& TEntityViewer::GetResult() const {
        return Impl->Result;
    }
}

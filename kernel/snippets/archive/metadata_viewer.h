#pragma once

#include <kernel/snippets/iface/archive/viewer.h>

#include <kernel/snippets/archive/view/order.h>

class TArchiveMarkupZones;

namespace NSnippets {

class TUnpacker;

class TMetadataViewer : public IArchiveViewer {
public:
    TUnpacker* Unpacker = nullptr;
    TSentsOrder Title;
    TSentsOrder Meta;
    bool MultiTitles = false;
    explicit TMetadataViewer(bool multiTitles)
      : MultiTitles(multiTitles)
    {
    }
    void OnUnpacker(TUnpacker* unpacker) override {
        Unpacker = unpacker;
    }
    void OnMarkup(const TArchiveMarkupZones& zones) override;
};

}

#pragma once

#include <util/memory/blob.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NSnippets {
    class TCanvas;
    typedef TAtomicSharedPtr<TCanvas> TCanvasPtr;
    class TCanvas {
    private:
        struct TImpl;
        THolder<TImpl> Impl;

        TCanvas(const TBlob& rawPng);

    public:
        static TCanvasPtr FromBlob(const TBlob& rawPng);
        static TCanvasPtr FromBase64(const TString& pngBase64);
        ~TCanvas();

        size_t GetW() const;
        size_t GetH() const;
        ui64 GetHash() const;
        ui64 GetSubHash(size_t x, size_t y, size_t w, size_t h);
    };
}

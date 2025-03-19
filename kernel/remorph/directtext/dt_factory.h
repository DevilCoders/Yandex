#pragma once

#include "dt_input_symbol.h"
#include "dt_breaksegment.h"
#include "dt_segment.h"

#include <kernel/indexer/direct_text/dt.h>
#include <library/cpp/langmask/langmask.h>

#include <util/generic/vector.h>
#include <util/generic/ylimits.h>
#include <library/cpp/charset/doccodes.h>

namespace NDT {

class TDTInpuSymbolFactory {
public:
    enum {
        DefaultMaxPunctCount = 10
    };
protected:
    size_t MaxPunctCount;

protected:
    virtual TDTInputSymbolPtr NewSymbol();

    bool ProcessPunctuation(TDTInputSymbols& res, const NIndexerCore::TDirectTextEntry2& entry, const TLangMask& langs);

public:
    TDTInpuSymbolFactory()
        : MaxPunctCount(DefaultMaxPunctCount)
    {
    }

    virtual ~TDTInpuSymbolFactory() {
    }

    // MaxPunctCount cannot be 0
    void SetMaxPunctCount(size_t count) {
        Y_ASSERT(count > 0);
        if (count > 0) {
            MaxPunctCount = count;
        }
    }

    // NOTE: the method doesn't clear res before inserting new symbols
    virtual void CreateSymbols(TDTInputSymbols& res, const NIndexerCore::TDirectTextEntry2* entries, size_t count,
        const TLangMask& langs);
};

class TDTInpuSymbolCacheFactory : public TDTInpuSymbolFactory {
private:
    enum {
        MaxCacheSize = 1000
    };

protected:
    TVector<TDTInputSymbolPtr> Cache;
    size_t CacheOffset;

protected:
    TDTInputSymbolPtr NewSymbol() override;

public:
    TDTInpuSymbolCacheFactory()
        : CacheOffset(0)
    {
    }

    ~TDTInpuSymbolCacheFactory() override {
    }

    void CreateSymbols(TDTInputSymbols& res, const NIndexerCore::TDirectTextEntry2* entries, size_t count,
        const TLangMask& langs) override;
};

template <class TSymbolsCallback>
class TSplitCallbackProxy: public ISegmentCallback {
private:
    TSymbolsCallback& Callback;
    TDTInpuSymbolFactory& Factory;
    TDTInputSymbols InputSymbols;
public:
    TSplitCallbackProxy(TSymbolsCallback& cb, TDTInpuSymbolFactory& factory)
        : Callback(cb)
        , Factory(factory)
    {
    }

    void OnSegment(const NIndexerCore::TDirectTextEntry2* entries, size_t from, size_t to, const TLangMask& langs) override {
        InputSymbols.clear();
        Factory.CreateSymbols(InputSymbols, entries + from, to - from, langs);
        if (!InputSymbols.empty()) {
            Callback(entries + from, to - from, InputSymbols);
        }
    }
};

template <class TSymbolsCallback>
inline void SplitDirectText(TSymbolsCallback& callback, const NIndexerCore::TDirectTextData2& dt,
    const TLangMask& langs, TDTInpuSymbolFactory& factory, const TDTSegmenter& segmenter) {

    TSplitCallbackProxy<TSymbolsCallback> proxy(callback, factory);
    segmenter.Segment(proxy, dt.Entries, 0, dt.EntryCount, langs);
}

template <class TSymbolsCallback>
inline void SplitDirectText(TSymbolsCallback& callback, const NIndexerCore::TDirectTextData2& dt,
    const TLangMask& langs, const TDTSegmenterPtr& segmenter = TDTSegmenterPtr()) {

    TDTInpuSymbolCacheFactory factory;
    if (!segmenter) {
        TDTSentSegmenter defSegmenter;
        SplitDirectText(callback, dt, langs, factory, defSegmenter);
    } else {
        SplitDirectText(callback, dt, langs, factory, *segmenter);
    }
}

} // NDT

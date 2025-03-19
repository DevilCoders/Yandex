#pragma once

#include <kernel/searchlog/errorlog.h>

#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/system/compiler.h>
#include <util/system/guard.h>
#include <util/system/mutex.h>
#include <util/system/types.h>
#include <util/thread/singleton.h>

#include <atomic>


class TDocHitCounters {
public:
    enum EStage {
        None = 0,
        L2 = 2,
        L3 = 3,
        L4 = 4
    };

    class atomic_ui32 {
    public:
        atomic_ui32()
            :Data_()
        {}

        atomic_ui32(ui32 value)
            :Data_(value)
        {}

        atomic_ui32(const std::atomic<ui32>& data)
            :Data_(data.load())
        {}

        atomic_ui32(const atomic_ui32& other)
            :Data_(other.Data_.load())
        {}

        atomic_ui32(const atomic_ui32&& other)
            :Data_(other.Data_.load())
        {}

        atomic_ui32& operator=(const atomic_ui32& other) {
            Data_.store(other.Data_.load());
            return *this;
        }

        atomic_ui32& operator++() {
            ++Data_;
            return *this;
        }

        atomic_ui32& operator--() {
            --Data_;
            return *this;
        }

        ui32 load() const {
            return Data_.load();
        }

        void store(ui32 value) {
            return Data_.store(value);
        }

    protected:
        std::atomic<ui32> Data_ = 0;
    };

    static void Setup(size_t shardDocCount = 120000000) {
        Enable_ = true;
        DocsPopularity_.resize(shardDocCount);
    }

    void SetStage(EStage stage) {
        Stage_ = stage;
    }

    static TVector<ui32> DocsPopularity(EStage stage) {
        if (stage == None || stage == L3 || stage == L4 || !Enable_) {
            return TVector<ui32>();
        }

        size_t docsPopularitySize = DocsPopularity_.size();
        TVector<ui32> docsPopularity(docsPopularitySize);
        for (ui32 docId = 0; docId < docsPopularitySize; ++docId) {
            docsPopularity[docId] = DocsPopularity_[docId].load();
        }

        return docsPopularity;
    }

    TVector<ui32> DocsPopularity() const {
        return DocsPopularity(Stage_);
    }

    void Hit(ui32 docId) {
        if (Stage_ == None || Stage_ == L3 || Stage_ == L4 || !Enable_ || docId >= DocsPopularity_.size()) {
            return;
        }
        ++DocsPopularity_[docId];
    }

protected:
    EStage Stage_ = EStage::None;

    static bool Enable_;
    static TVector<atomic_ui32> DocsPopularity_;
};


Y_FORCE_INLINE TDocHitCounters& ThreadLocalDocHitCounters() {
    return *FastTlsSingleton<TDocHitCounters>();
}

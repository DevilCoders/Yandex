#pragma once

#include "reader.h"


namespace NIndexAnn {
    template<typename TDerived>
    class TVisitorIterator {
        TDerived* Derived;
        THit PrevKey = THit(-1, -1, -1, -1, -1);
        THit Current_;
        bool Stop_ = false;

    public:
        TVisitorIterator()
            : Derived(static_cast<TDerived*>(this))
        {
        }

        void Stop() {
            Stop_ = true;
        }

        void OnDoc(ui32 /*docId*/) {}
        void OnBreak(ui32 /*breakId*/) {}
        void OnRegion(ui32 /*regId*/) {}
        void OnStream(ui32 /*streamId*/, ui32 /*value*/) {}

        void LeaveDoc(ui32 /*docId*/) {}
        void LeaveBreak(ui32 /*breakId*/) {}
        void LeaveRegion(ui32 /*regId*/) {}
        void LeaveStream(ui32 /*streamId*/) {}

        const THit& Current() const {
            return Current_;
        }

        void Iterate(IDocDataIterator* iter) {
            for (; iter->Valid() && !Stop_; iter->Next()) {
                Current_ = iter->Current();
                ProcessNewItem();
            }
            if (!Stop_) {
                Current_ = THit();
                ProcessNewItem();
            }
        }
    private:
        void ProcessNewItem() {
            bool force = false;
            if (Current_.DocId() != PrevKey.DocId()) {
                if (PrevKey.DocId() != static_cast<ui32>(-1)) {
                    Derived->LeaveDoc(PrevKey.DocId());
                }
                if (Current_.DocId() != static_cast<ui32>(-1)) {
                    Derived->OnDoc(Current_.DocId());
                }
                force = true;
            }
            if (force || Current_.Break() != PrevKey.Break()) {
                if (PrevKey.Break() != static_cast<ui16>(-1)) {
                    Derived->LeaveBreak(PrevKey.Break());
                }
                if (Current_.Break() != static_cast<ui16>(-1)) {
                    Derived->OnBreak(Current_.Break());
                }
                force = true;
            }
            if (force || Current_.Region() != PrevKey.Region()) {
                if (PrevKey.Region() != static_cast<ui32>(-1)) {
                    Derived->LeaveRegion(PrevKey.Region());
                }
                if (Current_.Region() != static_cast<ui32>(-1)) {
                    Derived->OnRegion(Current_.Region());
                }
                force = true;
            }
            if (force || Current_.Stream() != PrevKey.Stream()) {
                if (PrevKey.Stream() != static_cast<ui8>(-1)) {
                    Derived->LeaveStream(PrevKey.Stream());
                }
                if (Current_.Stream() != static_cast<ui8>(-1)) {
                    Derived->OnStream(Current_.Stream(), Current_.Value());
                }
            }
            PrevKey = Current_;
        }
    };

} // NIndexAnn

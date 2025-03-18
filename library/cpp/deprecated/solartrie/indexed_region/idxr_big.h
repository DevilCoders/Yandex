#pragma once

#include "idxr_base.h"

#include <library/cpp/succinct_arrays/eliasfanomonotone.h>

#include <util/stream/mem.h>
#include <util/stream/buffer.h>

namespace NIndexedRegion {
    class TBigIndexedRegion: public TIndexedBase<NSuccinctArrays::TEliasFanoMonotoneArray<ui64>> {
    public:
        TBigIndexedRegion() {
        }

        TBigIndexedRegion(TStringBuf raw) {
            Decode(raw);
        }

        void DoCommit() override {
            IndexRegion.Clear();
            DataRegion.Clear();

            if (!IndexBuffer.empty()) {
                IndexRegion.Learn(IndexBuffer.back() + 1, IndexBuffer.size());

                for (auto it : IndexBuffer) {
                    IndexRegion.Add(it);
                }

                IndexBuffer.clear();
            }

            IndexRegion.Finish();
            DataRegion = AsDataRegion(DataBuffer);
        }

        void Encode(TBuffer& b) const override {
            TBufferOutput bout;
            Save(&bout);
            b.Swap(bout.Buffer());
        }

        void Save(IOutputStream* out) const {
            IndexRegion.Save(out);
            out->Write(DataRegion.begin(), DataRegion.size());
        }

        TBuffer& GetDataBuffer() {
            return DataBuffer;
        }

        void Decode(TStringBuf raw) override {
            TMemoryInput min(raw.begin(), raw.size());
            IndexRegion.Clear();
            IndexRegion.Load(&min);
            size_t consumed = raw.size() - min.Avail();
            raw.Skip(consumed);
            DataRegion = raw;
        }
    };

}

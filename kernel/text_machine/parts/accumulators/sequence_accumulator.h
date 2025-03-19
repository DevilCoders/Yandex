#pragma once

#include <kernel/text_machine/parts/common/storage.h>

#include <util/generic/vector.h>
#include <util/system/defaults.h>

#include <kernel/text_machine/module/save_to_json.h>

namespace NTextMachine {
namespace NCore {
    class TSequenceAccumulator : public NModule::TJsonSerializable {
    public:
        void Init(TMemoryPool& pool, ui16 numWords);
        void Clear();
        void Update(ui16 queryWordPos, ui16 annWordPos);
        size_t GetMaxSequenceLength();

        void SaveToJson(NJson::TJsonValue& value) const;

    private:
        //QueryAnnPos array contains information about max sequence length for each query element
        //but only for annWordPos,annWordPos-1. More older entries are not needed
        //to calculate MaxSequenceLength. They will be erased.
        Y_FORCE_INLINE ui16 GetRecordIndex(ui16 queryWordPos, ui16 annWordPos) {
            return queryWordPos + (annWordPos & 1) * NumWords;
        }
        Y_FORCE_INLINE ui16 GetPrevRecordIndex(ui16 curIndex) {
            Y_ASSERT(curIndex != 0 && curIndex != NumWords);
            if (curIndex >= NumWords) {
                return curIndex - NumWords - 1;
            } else {
                return curIndex + NumWords - 1;
            }
        }

        ui16 MaxSequenceLength = Max<ui16>();
        ui16 NumWords = Max<ui16>();

        struct TQueryAnnPos
            : public NModule::TJsonSerializable
        {
            ui16 AnnWordPos = Max<ui16>();
            ui16 MaxSequenceLength = Max<ui16>();

            void SaveToJson(NJson::TJsonValue& value) const;
        };

        TPoolPodHolder<TQueryAnnPos> QueryAnnPos;
    };
};
};

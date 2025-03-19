#pragma once

#include <kernel/text_machine/module/module_def.inc>

namespace NTextMachine {
namespace NCore {
    class TBitMaskAccumulator : public NModule::TJsonSerializable {
    public:
        void Clear() {
            Mask = 0;
            Count = 0;
        }

        size_t GetCount() const {
            return Count;
        }

        void Update(const ui32 wordPos) {
            ui32 flag = 1U << (wordPos & 31);
            if (!(Mask & flag)) {
                Mask |= flag;
                ++Count;
            }
        }

        void SaveToJson(NJson::TJsonValue& value) const {
            SAVE_JSON_VAR(value, Mask);
            SAVE_JSON_VAR(value, Count);
        }

    private:
        ui32 Mask = 0;
        size_t Count = 0;
    };
};
};

#include <kernel/text_machine/module/module_undef.inc>

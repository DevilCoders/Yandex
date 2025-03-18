#pragma once

#include "private/decoder_table.h"
#include "private/encoder_table.h"

#include "model_16.h"

namespace NOffroad {
    template <class BaseTable>
    class TTable16 {
    public:
        using TModel = TModel16;

        TTable16() {
        }

        TTable16(const TModel& model) {
            Reset(model);
        }

        void Reset(const TModel& model) {
            Table_.Reset(model.Groups());
        }

        const BaseTable& Base() const {
            return Table_;
        }

    private:
        BaseTable Table_;
    };

}

#pragma once

#include <array>

#include "interleaved_model.h"

namespace NOffroad {
    template <size_t tupleSize, class BaseTable>
    class TInterleavedTable {
    public:
        enum {
            TupleSize = tupleSize
        };

        using TModel = TInterleavedModel<TupleSize, typename BaseTable::TModel>;

        TInterleavedTable() {
        }

        TInterleavedTable(const TModel& model) {
            Reset(model);
        }

        void Reset(const TModel& model) {
            for (size_t i = 0; i < TupleSize; i++)
                Base_[i].Reset(model.Base(i));
        }

        const BaseTable& Base(size_t index) const {
            return Base_[index];
        }

    private:
        std::array<BaseTable, TupleSize> Base_;
    };

}

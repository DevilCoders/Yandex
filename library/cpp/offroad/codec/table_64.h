#pragma once

#include "model_64.h"

namespace NOffroad {
    template <class BaseTable>
    class TTable64 {
    public:
        using TModel = TModel64;

        TTable64() = default;

        TTable64(const TModel& model) {
            Reset(model);
        }

        void Reset(const TModel& model) {
            Table0_.Reset(model.Groups0());
            Table1_.Reset(model.Groups1());
        }

        const BaseTable& Base0() const {
            return Table0_;
        }

        const BaseTable& Base1() const {
            return Table1_;
        }

    private:
        BaseTable Table0_;
        BaseTable Table1_;
    };

}

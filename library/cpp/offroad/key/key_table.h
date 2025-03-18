#pragma once

#include "key_model.h"

namespace NOffroad {
    template <class BaseTable>
    class TKeyTable {
    public:
        using TModel = TKeyModel<typename BaseTable::TModel>;

        TKeyTable() {
        }

        TKeyTable(const TModel& model) {
            Reset(model);
        }

        void Reset(const TModel& model) {
            Base_.Reset(model.Base());
        }

        const BaseTable& Base() const {
            return Base_;
        }

    private:
        BaseTable Base_;
    };

}

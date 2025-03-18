#pragma once

#include <type_traits>

#include <util/generic/ptr.h>

namespace NOffroad {
    template <class Base0, class Base1>
    class TMultiTable: public Base0, public Base1 {
        static_assert(std::is_same<typename Base0::TModel, typename Base1::TModel>::value, "Expecting same model for both of the base tables.");

    public:
        using TModel = typename Base0::TModel;

        TMultiTable() {
        }

        TMultiTable(const TModel& model) {
            Reset(model);
        }

        void Reset(const TModel& model) {
            Base0::Reset(model);
            Base1::Reset(model);
        }
    };

    template <class Model>
    THolder<TMultiTable<typename Model::TEncoderTable, typename Model::TDecoderTable>> NewMultiTable(const Model& model) {
        return MakeHolder<TMultiTable<typename Model::TEncoderTable, typename Model::TDecoderTable>>(model);
    }

}

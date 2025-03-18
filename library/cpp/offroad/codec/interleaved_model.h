#pragma once

#include <util/string/cast.h>
#include <util/ysaveload.h>

#include <array>

#include "fwd.h"
#include "serializable_model.h"

namespace NOffroad {
    template <size_t tupleSize, class BaseModel>
    class TInterleavedModel: public TSerializableModel<TInterleavedModel<tupleSize, BaseModel>> {
    public:
        using TEncoderTable = TInterleavedTable<tupleSize, typename BaseModel::TEncoderTable>;
        using TDecoderTable = TInterleavedTable<tupleSize, typename BaseModel::TDecoderTable>;

        enum {
            TupleSize = tupleSize
        };

        TInterleavedModel() = default;

        const BaseModel& Base(size_t index) const {
            return Base_[index];
        }

        BaseModel& Base(size_t index) {
            return Base_[index];
        }

        static TString TypeName() {
            return "TInterleavedModel<" + ToString(tupleSize) + "," + BaseModel::TypeName() + ">";
        }

    protected:
        void DoLoad(IInputStream* in) override {
            ::Load(in, Base_);
        }

        void DoSave(IOutputStream* out) const override {
            ::Save(out, Base_);
        }

    private:
        template <size_t count, class Sampler>
        friend class TInterleavedSampler;

        std::array<BaseModel, TupleSize> Base_;
    };

}

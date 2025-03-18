#pragma once

#include <util/ysaveload.h>

#include <library/cpp/offroad/codec/serializable_model.h>
#include <library/cpp/offroad/codec/interleaved_model.h>

#include "fwd.h"
#include "key_subtractor.h"

namespace NOffroad {
    template <class BaseModel>
    class TKeyModel: public TSerializableModel<TKeyModel<BaseModel>> {
        using TBase = TSerializableModel<TKeyModel<BaseModel>>;

    public:
        using TEncoderTable = TKeyTable<typename BaseModel::TEncoderTable>;
        using TDecoderTable = TKeyTable<typename BaseModel::TDecoderTable>;

        TKeyModel() {
        }

        const BaseModel& Base() const {
            return Base_;
        }

        static TString TypeName() {
            return CompatibilityTypeName(static_cast<BaseModel*>(nullptr));
        }

    protected:
        void DoLoad(IInputStream* in) override {
            CompatibilityLoadData(in, Base_);
        }

        void DoSave(IOutputStream* out) const override {
            CompatibilitySaveData(out, Base_);
        }

        template <class Model>
        static TString CompatibilityTypeName(Model*) {
            return "TKeyModel<" + Model::TypeName() + ">";
        }

        template <size_t tupleSize, class Model>
        static TString CompatibilityTypeName(TInterleavedModel<tupleSize, Model>*) {
            return "TKeyModel<" + TInterleavedModel<tupleSize - 1, Model>::TypeName() + "," + Model::TypeName() + ">";
        }

        template <class Model>
        static void CompatibilityLoadData(IInputStream* in, Model& base) {
            ::LoadMany(in, base);
        }

        template <size_t tupleSize, class Model>
        static void CompatibilityLoadData(IInputStream* in, TInterleavedModel<tupleSize, Model>& base) {
            TInterleavedModel<tupleSize - 1, Model> dataModel;
            Model textModel;

            ::LoadMany(in, dataModel, textModel);

            for (size_t i = 0; i < tupleSize - 1; i++)
                base.Base(i) = std::move(dataModel.Base(i));
            base.Base(tupleSize - 1) = std::move(textModel);
        }

        template <class Model>
        static void CompatibilitySaveData(IOutputStream* out, const Model& base) {
            ::SaveMany(out, base);
        }

        template <size_t tupleSize, class Model>
        static void CompatibilitySaveData(IOutputStream* out, const TInterleavedModel<tupleSize, Model>& base) {
            TInterleavedModel<tupleSize - 1, Model> dataModel;
            Model textModel;

            for (size_t i = 0; i < tupleSize - 1; i++)
                dataModel.Base(i) = base.Base(i);
            textModel = base.Base(tupleSize - 1);

            ::SaveMany(out, dataModel, textModel);
        }

    private:
        template <class Data, class Vectorizer, class Subtractor, EKeySubtractor>
        friend class TKeySampler;

        BaseModel Base_;
    };

}

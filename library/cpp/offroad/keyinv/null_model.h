#pragma once

#include <library/cpp/offroad/codec/serializable_model.h>

#include <util/generic/string.h>
#include <util/stream/output.h>

namespace NOffroad {
    class TNullModel: public TSerializableModel<TNullModel> {
    public:
        static TString TypeName() {
            return "NullModel";
        }

    private:
        void DoLoad(IInputStream*) override {
        }
        void DoSave(IOutputStream*) const override {
        }
    };

    class TNullTable {
    public:
        using TModel = TNullModel;

        template <class... Args>
        TNullTable(Args&&... /*args*/) {
        }

        void Reset(const TModel&) {
        }
    };

}

#pragma once

#include <util/ysaveload.h>

#include "private/prefix_group.h"
#include "fwd.h"
#include "serializable_model.h"

namespace NOffroad {
    class TModel16: public TSerializableModel<TModel16> {
    public:
        using TEncoderTable = TEncoder16Table;
        using TDecoderTable = TDecoder16Table;

        TModel16() = default;

        const NPrivate::TPrefixGroupList& Groups() const {
            return Groups_;
        }

        static TString TypeName() {
            return "TModel16";
        }

    protected:
        void DoLoad(IInputStream* in) override {
            ui64 tmp = 0; // for compatibility
            ::LoadMany(in, Groups_, tmp);
        }

        void DoSave(IOutputStream* out) const override {
            ui64 tmp = 0; // for compatibility
            ::SaveMany(out, Groups_, tmp);
        }

    private:
        friend class TSampler16;

        NPrivate::TPrefixGroupList Groups_;
    };

}

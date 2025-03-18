#pragma once

#include <util/ysaveload.h>

#include "private/prefix_group.h"
#include "fwd.h"
#include "serializable_model.h"

namespace NOffroad {
    class TModel64: public TSerializableModel<TModel64> {
    public:
        using TEncoderTable = TEncoder64Table;
        using TDecoderTable = TDecoder64Table;

        TModel64() = default;

        /**
         * Returns hash info that is used for scheme (de)compression.
         */
        const NPrivate::TPrefixGroupList& Groups0() const {
            return Groups0_;
        }

        /**
         * Returns hash info that is used for data (de)compression.
         */
        const NPrivate::TPrefixGroupList& Groups1() const {
            return Groups1_;
        }

        static TString TypeName() {
            return "TModel64";
        }

    protected:
        void DoLoad(IInputStream* in) override {
            ui64 tmp = 0; // for compatibility
            ::LoadMany(in, Groups0_, tmp, Groups1_, tmp);
        }

        void DoSave(IOutputStream* out) const override {
            ui64 tmp = 0; // for compatibility
            ::SaveMany(out, Groups0_, tmp, Groups1_, tmp);
        }

    private:
        friend class TSampler64;
        friend class TBitSampler16;
        NPrivate::TPrefixGroupList Groups0_;
        NPrivate::TPrefixGroupList Groups1_;
    };

}

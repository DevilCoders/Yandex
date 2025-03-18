#pragma once

#include <util/generic/string.h>

namespace NOffroad {
    template <class KeyData>
    class TNullKeyWriter {
    public:
        using TKey = TString;
        using TKeyRef = TStringBuf;
        using TKeyData = KeyData;

        using TTable = void;
        using TModel = void;

        enum {
            Stages = 0
        };

        TNullKeyWriter() {
        }

        void Reset() {
        }

        void WriteKey(const TKeyRef&, const TKeyData&) {
        }

        void Finish() {
            Finished_ = true;
        }

        bool IsFinished() {
            return Finished_;
        }

    private:
        bool Finished_ = false;
    };

}

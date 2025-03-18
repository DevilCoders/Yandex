#pragma once

#include <util/generic/string.h>
#include <util/system/yassert.h>

#include "null_model.h"

namespace NOffroad {
    namespace NPrivate {
        struct TAnyModel {
            template <class Model>
            operator Model() {
                Y_VERIFY(false, "You shouldn't really be calling this.");
            }
        };

        struct TUniAnyModel: public TAnyModel {
            TAnyModel first;
            TAnyModel second;
        };
    }

    class TNullKeyInvSampler {
    public:
        using TKey = TString;
        using TKeyRef = TStringBuf;
        using THit = void;

        using THitModel = TNullModel;
        using TKeyModel = TNullModel;

        enum {
            Stages = 0,
        };

        template <class... Args>
        TNullKeyInvSampler(Args&&...) {
        }

        template <class... Args>
        void Reset(Args&&...) {
        }

        void WriteKey(const TKeyRef&) {
            /* Just ignore it. */
        }

        template <class Hit>
        void WriteHit(const Hit&) {
            /* Just ignore it. */
        }

        NPrivate::TUniAnyModel Finish() {
            Y_VERIFY(false, "You shouldn't really be calling this.");
        }
    };

}

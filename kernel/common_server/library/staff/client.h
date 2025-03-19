#pragma once

#include "entry.h"
#include <kernel/common_server/library/tvm_services/abstract/abstract.h>

namespace NCS {

    class TStaffClient {
    public:
        using TStaffEntries = TVector<TStaffEntry>;

    public:
        TStaffClient(NExternalAPI::TSender::TPtr sender);

        bool GetUserData(const TStaffEntrySelector& selector,
            TStaffEntries& results,
            const TStaffEntry::TStaffEntryFields& fields = {},
            size_t limit = 250
        ) const;

    private:
        NExternalAPI::TSender::TPtr Sender;
    };

}

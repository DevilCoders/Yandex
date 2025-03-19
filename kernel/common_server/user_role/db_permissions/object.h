#pragma once
#include <kernel/common_server/api/history/db_entities.h>
#include <kernel/common_server/api/links/link.h>
#include <kernel/common_server/api/links/manager.h>
#include <kernel/common_server/user_role/abstract/abstract.h>

class TDBLinkUserRole: public TDBLink<TString, ui32> {
public:
    static TString GetTableName() {
        return "links_user_role";
    }

    static TString GetHistoryTableName() {
        return "links_user_role_history";
    }
};


#pragma once

#include <util/generic/string.h>
#include <util/generic/set.h>
#include <util/datetime/base.h>
#include <util/string/builder.h>
#include <util/string/printf.h>
#include <util/string/cast.h>
#include <kernel/common_server/library/searchserver/simple/http_status_config.h>
#include <kernel/common_server/util/accessor.h>
#include <library/cpp/mediator/messenger.h>

namespace NFrontend {
    class TCacheRefreshMessage: public IMessage {
        CSA_DEFAULT(TCacheRefreshMessage, TSet<TString>, Components);
    public:
        TCacheRefreshMessage() = default;
        TCacheRefreshMessage(const TSet<TString>& entities)
            : Components(entities) {
        }
        TCacheRefreshMessage(const TString& componentId) {
            Components.emplace(componentId);
        }
    };
}

enum class EObjectHistoryAction {
    SetPerformer = 0 /* "set_performer" */,
    DropPerformer = 1 /* "drop_performer" */,
    AddSnapshot = 2 /* "add_snapshot" */,
    TagEvolve = 3 /* "evolve" */,
    ForceTagPerformer = 4 /* "force_performer" */,
    Add = 5 /* "add" */,
    Remove = 6 /* "remove" */,
    UpdateData = 7 /* "update_data" */,

    Proposition = 9 /* "proposition" */,
    Confirmation = 10 /* "confirmation" */,
    Approved = 11 /* "approved" */,
    Rejected = 12 /* "rejected" */,

    TakeResource = 13 /* "take_resource" */,
    PutResource = 14 /* "put_resource" */,
    ReturnResource = 15 /* "return_resource" */,

    Inconsistency = 16 /* "inconsistency" */,
    ExternalInfo = 17 /* "external_info" */,

    UpsertData = 18 /* "upsert_data" */,

    Unknown = 100 /* "unknown" */,
};

enum class ELocalizationCodes {
    NoPermissions = 0 /* "no_permissions" */,
    UserNotFound = 1 /* "user_not_found" */,
    InternalServerError = 2 /* "internal_server_error" */,
    SyntaxUserError = 3 /* "syntax_user_error" */,
    ObjectNotFound = 4 /* "object_not_found" */,
};

enum class ESessionResult {
    Success = 0 /* "success" */,
    InconsistencyUser /* "inconsistency_user" */,
    InconsistencySystem /* "inconsistency_system" */,
    TransactionProblem /* "transaction_problem" */,
    InternalError /* "internal_error" */,
    ResourceLocked /* "resource_locked" */,
    NoUserPermissions /* "no_user_permissions" */,
    DataCorrupted /* "data_corrupted" */,
    IncorrectRequest /* "incorrect_request" */
};

namespace NCS {

    class TDecoder {
    public:
        static bool DecodeEnumCode(const ESessionResult value, const THttpStatusManagerConfig& config, int& code);
    };
}

#pragma once

#include <kernel/common_server/library/async_impl/logger.h>

enum EStartrekOperationType {
    GetIssue /* "get_issue" */,
    CreateIssue /* "create_issue" */,
    PatchIssue /* "patch_issue" */,
    AddComment /* "add_comment" */,
    GetComment /* "get_comment" */,
    GetAllComments /* "get_all_comments" */,
    DeleteComment /* "delete_comment" */,
    GetTransitions /* "get_transitions" */,
    ExecuteTransition /* "execute_transition" */,
};

class TStartrekLogger : public TRequestLogger<EStartrekOperationType> {
    using TBase = TRequestLogger<EStartrekOperationType>;

public:
    TStartrekLogger(const TString& source)
        : TBase(source, "startrek-api")
    {
    }
};

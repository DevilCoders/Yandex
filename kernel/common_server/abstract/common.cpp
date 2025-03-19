#include "common.h"

bool NCS::TDecoder::DecodeEnumCode(const ESessionResult value, const THttpStatusManagerConfig& config, int& code) {
    code = (int)HTTP_OK;
    switch (value) {
        case ESessionResult::Success:
            return false;
        case ESessionResult::InternalError:
        case ESessionResult::InconsistencySystem:
        case ESessionResult::TransactionProblem:
        case ESessionResult::DataCorrupted:
            code = config.UnknownErrorStatus;
            break;
        case ESessionResult::InconsistencyUser:
        case ESessionResult::ResourceLocked:
            code = config.UserErrorState;
            break;
        case ESessionResult::IncorrectRequest:
            code = config.SyntaxErrorStatus;
            break;
        case ESessionResult::NoUserPermissions:
            code = config.PermissionDeniedStatus;
            break;
    }
    return true;
}

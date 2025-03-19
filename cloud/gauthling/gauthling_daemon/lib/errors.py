import json

import grpc


class GauthlingError(Exception):
    def __init__(self, message, user_message):
        self.message = message
        self.user_message = user_message

    @staticmethod
    def from_dict(d):
        code = d['Code']
        if code not in ERRORS:
            return GauthlingError("Unexpected error code: {}".format(code), "")
        return ERRORS[code](d['Message'], d["UserMessage"])


class GauthlingInvalidArgument(GauthlingError):
    pass


class GauthlingInvalidInvocationContext(GauthlingError):
    pass


class GauthlingInvalidInvocationContextAttribute(GauthlingError):
    pass


class GauthlingInvalidInvocationContextAttributeValue(GauthlingError):
    pass


class GauthlingInvalidStorageLocation(GauthlingError):
    pass


class GauthlingAuthenticationInternalError(GauthlingError):
    pass


class GauthlingAuthorizationInternalError(GauthlingError):
    pass


class GauthlingStorageInternalError(GauthlingError):
    pass


def handle_rpc_error(fun):
    def wrapper(*args, **kwargs):
        try:
            return fun(*args, **kwargs)
        except grpc.RpcError as e:
            try:
                gauthling_error = json.loads(e.details())
                e = GauthlingError.from_dict(gauthling_error)
            except:
                pass
            raise e

    return wrapper


ERRORS = {
    # Commented error codes are unused
    # 0: GauthlingNoError
    1: GauthlingInvalidArgument,
    2: GauthlingInvalidInvocationContext,
    3: GauthlingInvalidInvocationContextAttribute,
    4: GauthlingInvalidInvocationContextAttributeValue,
    5: GauthlingInvalidStorageLocation,
    6: GauthlingAuthenticationInternalError,
    7: GauthlingAuthorizationInternalError,
    8: GauthlingStorageInternalError,
    # 9: GauthlingIssueInternalError,
    # 10: GauthlingSynchronizationError
}

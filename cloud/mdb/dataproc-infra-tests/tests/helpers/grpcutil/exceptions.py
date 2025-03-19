# TODO: may be we should replace this with cloud.mdb.internal.python.grpcutil.exceptions?
import logging


class GRPCError(Exception):
    """
    Base class for errors received during grpc interaction.
    """

    message: str
    err_type: str
    code: int

    def __init__(self, logger: logging.Logger, message: str, err_type: str, code: int):
        super().__init__(message)
        logger.info('Error of "%s" happened: "%s"', self.__class__.__name__, message)
        self.message = message
        self.err_type = err_type
        self.code = code


class CancelledError(GRPCError):
    pass


class TemporaryUnavailableError(GRPCError):
    pass


class NotFoundError(GRPCError):
    pass


class DeadlineExceededError(GRPCError):
    pass


class AuthenticationError(GRPCError):
    pass


class AuthorizationError(GRPCError):
    pass


class ResourceExhausted(GRPCError):
    pass


class UnparsedGRPCError(GRPCError):
    """
    When no suitable parsers provided, do not return this in your code.
    """

    pass

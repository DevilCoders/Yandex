# -*- coding: utf-8 -*-

from yandex_tracker_client import exceptions as external_exceptions


class StartrekError(external_exceptions.TrackerError):
    """Base class for all Startrek client errors."""


class StartrekClientError(StartrekError, external_exceptions.TrackerClientError):
    """Base class for exceptions on client side."""


class UnencodableValue(StartrekClientError, external_exceptions.UnencodableValue, ValueError):
    pass


class StartrekRequestError(StartrekError, external_exceptions.TrackerRequestError, IOError):
    pass


class StartrekServerError(StartrekError, external_exceptions.TrackerServerError, IOError):
    pass


class OutOfRetries(StartrekServerError, external_exceptions.OutOfRetries):
    pass


class BadRequest(StartrekServerError, external_exceptions.BadRequest):
    pass


class Forbidden(StartrekServerError, external_exceptions.Forbidden):
    pass


class NotFound(StartrekServerError, external_exceptions.NotFound):
    pass


class Conflict(StartrekServerError, external_exceptions.Conflict):
    pass


class PreconditionFailed(StartrekServerError, external_exceptions.PreconditionFailed):
    pass


class UnprocessableEntity(StartrekServerError, external_exceptions.UnprocessableEntity):
    pass


class PreconditionRequired(StartrekServerError, external_exceptions.PreconditionRequired):
    pass


EXCEPTIONS_MAP = {
    external_exceptions.TrackerError: StartrekError,
    external_exceptions.TrackerClientError: StartrekClientError,
    external_exceptions.UnencodableValue: UnencodableValue,
    external_exceptions.TrackerRequestError: StartrekRequestError,
    external_exceptions.TrackerServerError: StartrekServerError,
    external_exceptions.OutOfRetries: OutOfRetries,
    external_exceptions.BadRequest: BadRequest,
    external_exceptions.Forbidden: Forbidden,
    external_exceptions.NotFound: NotFound,
    external_exceptions.Conflict: Conflict,
    external_exceptions.PreconditionFailed: PreconditionFailed,
    external_exceptions.UnprocessableEntity: UnprocessableEntity,
    external_exceptions.PreconditionRequired: PreconditionRequired,

}

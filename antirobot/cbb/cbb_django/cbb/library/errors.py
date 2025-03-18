class CbbError(Exception):

    def __init__(self, ivalue):
        Exception.__init__(self, ivalue)
        self.value = ivalue


class ValidationError(CbbError):
    """ Api request validation failed """
    def __init__(self, msg="Request validation failed", errors=None):
        # TODO: better errors serialization?
        if errors:
            str_errors = errors.as_text()
            msg = "%s: %s" % (msg, str_errors)

        super(ValidationError, self).__init__(msg)


class NoGroupError(CbbError):
    pass


class NotEmptyGroupError(CbbError):
    pass


class NotActiveGroupError(CbbError):
    pass


class BadGroupTypeError(CbbError):
    pass


class WrongOperationError(CbbError):
    pass


class InvalidTimeError(CbbError):
    pass


class ExceptionError(CbbError):
    pass


class ExceptionToInternalError(ExceptionError):
    pass


class ExceptionEqualsNetError(ExceptionError):
    pass


class ExceptionMismatchError(ExceptionError):
    pass


class OverlapExceptionsError(ExceptionError):
    pass


class InvalidRangeError(CbbError):
    pass


class RangeAlreadyExistsError(InvalidRangeError):
    pass


class TxtRangeAlreadyExistsError(InvalidRangeError):
    pass


class InvalidNetRangeError(InvalidRangeError):
    pass


class OverlapRangeError(InvalidRangeError):
    pass


class EmptyRangeError(InvalidRangeError):
    pass


class InvalidTxtRangeError(InvalidRangeError):
    pass

class LMSException(Exception):
    pass


class UnknownIncrementTypeError(LMSException):
    pass


class PathIsNotADirectoryError(LMSException):
    pass


class UnknownFileLoadTypeError(LMSException):
    pass


class InvalidObjectIdError(LMSException):
    pass


class InvalidConnIdError(LMSException):
    pass


class InvalidFileMaskError(LMSException):
    pass

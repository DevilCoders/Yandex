# coding: utf-8
"""
Worker exceptions
"""


class ExposedException(Exception):
    """
    Generic exposed exception
    """

    def __init__(self, message, err_type='InternalServerError', code=13, exposable=False):
        super().__init__(message)
        self.code = code
        self.type = err_type
        self.message = message
        self.exposable = exposable

    def serialize(self):
        """
        Get dict representation
        """
        return dict(code=self.code, type=self.type, message=self.message, exposable=self.exposable)


class UserExposedException(ExposedException):
    """
    User exposed exception
    """

    def __init__(self, message, err_type='InternalServerError', code=13):
        super().__init__(message=message, err_type=err_type, code=code, exposable=True)


class TimeOut(BaseException):
    """
    Task execution timeout
    """


class Interrupted(BaseException):
    """
    Task interruption exception
    """

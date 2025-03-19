import six  # noqa


class BaseApiError(Exception):
    def __init__(self, msg, code):
        # type: (six.text_type, int) -> None
        self.msg = msg
        self.code = code

    def __str__(self):
        # type: () -> six.text_type
        return repr(self)

    def __repr__(self):
        # type: () -> six.text_type
        return "[{}] {}".format(self.code, self.msg)


class NotFoundError(BaseApiError):
    def __init__(self, msg="", code=404):
        # type: (six.text_type, int) -> None
        super(NotFoundError, self).__init__(msg, code)


class ServerError(BaseApiError):
    """
    Hope this exception will never be called
    """
    def __init__(self, msg="", code=500):
        # type: (six.text_type, int) -> None
        super(ServerError, self).__init__(msg, code)


class AuthError(BaseApiError):
    def __init__(self, msg="", code=401):
        # type: (six.text_type, int) -> None
        super(AuthError, self).__init__(msg, code)


class BadRequestError(BaseApiError):
    def __init__(self, msg="", code=400):
        # type: (six.text_type, int) -> None
        super(BadRequestError, self).__init__(msg, code)

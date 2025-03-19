from yc_common.exceptions import Error


class BaseError(Error):
    def __init__(self, client_name, message):
        self.client = client_name
        super().__init__("{} client error: {}", client_name.capitalize(), message)


class ClientError(BaseError):
    def __init__(self, client_name, code, message):
        self.code = code
        super().__init__(client_name, message)


class UnknownError(BaseError):
    pass


class ConfigurationError(BaseError):
    pass


class BadRequest(ClientError):
    def __init__(self, client_name, message):
        super().__init__(client_name, 400, message)


class Unauthorized(ClientError):
    def __init__(self, client_name, message):
        super().__init__(client_name, 401, message)


class Forbidden(ClientError):
    def __init__(self, client_name, message):
        super().__init__(client_name, 403, message)


class NotFound(ClientError):
    def __init__(self, client_name, message):
        super().__init__(client_name, 404, message)


class ServerError(ClientError):
    pass


class RequestError(BaseError):
    pass

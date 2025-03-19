class RacktablesError(RuntimeError):
    pass


class RacktablesClientError(RacktablesError):
    pass


class RacktablesUnknownClientError(RacktablesClientError):
    pass


class RacktablesNotFoundError(RacktablesClientError):
    pass


class RacktablesServerError(RacktablesError):
    pass


class RacktablesUnknownServerError(RacktablesServerError):
    pass

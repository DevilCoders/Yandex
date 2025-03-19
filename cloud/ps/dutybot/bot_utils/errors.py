class Error(Exception):
    """Base exception class with message formatting."""

    def __init__(self, *args):
        message, args = args[0], args[1:]
        super().__init__(message.format(*args) if args else message)


class LogicalError(Exception):
    """Raised from code that never should be reached."""

    def __init__(self, *args):
        if args:
            message, args = args[0], args[1:]
            if args:
                message = message.format(*args)
        else:
            message = "Logical error."

        super().__init__(message)

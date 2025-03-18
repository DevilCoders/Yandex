from flask import request

from exit_msg import ExitMessageFile, ExitCodes


def shutdown(exit_code=ExitCodes.NORMAL):
    """Shutdowns the server after processing the request."""

    shutdown_func = request.environ.get("werkzeug.server.shutdown")
    if shutdown_func is None:
        raise RuntimeError("Not running with the Werkzeug Server")

    ExitMessageFile.set_code(exit_code)
    shutdown_func()

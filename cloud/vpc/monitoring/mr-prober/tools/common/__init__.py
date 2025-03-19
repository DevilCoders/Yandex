import logging
import pathlib

import settings
from .cli import *
from .stop import *


def read_api_key(api_key_file: str) -> str:
    if not api_key_file:
        return settings.API_KEY

    api_key_file = pathlib.Path(api_key_file)
    if not api_key_file.is_file():
        logging.error(
            f"File [bold]{api_key_file}[/bold] not found. "
            f"Specify correct path in [italic]--api-key-file[/italic] option."
        )
        raise StopCliProcess("API key file not found", ExitCode.API_KEY_NOT_FOUND)

    return api_key_file.read_text().rstrip()

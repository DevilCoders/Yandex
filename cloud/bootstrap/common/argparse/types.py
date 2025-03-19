"""Custom argparse types"""

import functools
import getpass
from typing import Callable


DEFAULT_PASSWORD = "Password if not specified"


def _prompt_password(prompt: str, value: str) -> str:
    if value == DEFAULT_PASSWORD:
        return getpass.getpass(prompt=prompt)
    else:
        return value


def prompt_password(prompt: str) -> Callable:
    """Interactively specify password"""
    return functools.partial(_prompt_password, prompt)

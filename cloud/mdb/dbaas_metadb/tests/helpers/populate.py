"""
Helper for populate_table
"""

import subprocess
from pathlib import Path
from typing import Union

from .locate import locate_populate_table


def run_populate_table(table: str, dsn: str, key: str, file_path: Union[str, Path]) -> None:
    """
    Run populate table
    """
    subprocess.check_call(
        [
            locate_populate_table(),
            '-t',
            table,
            '-d',
            dsn,
            '-k',
            key,
            '-f',
            str(file_path),
        ]
    )

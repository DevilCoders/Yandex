import os
from pathlib import Path
from os.path import join


def curr_dir() -> Path:
    return Path(__file__).parent.absolute()


def childfile2str(filepath: str, path: str) -> str:
    return Path(join(os.path.dirname(filepath)), path).read_text()

__all__ = ['curr_dir', 'childfile2str']

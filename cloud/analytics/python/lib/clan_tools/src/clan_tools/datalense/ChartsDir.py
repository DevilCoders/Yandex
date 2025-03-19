from pathlib import Path
from os.path import join


class ChartsDir:

    def __init__(self, dir: str) -> None:
        self._dir = dir

    def read(self, file_name: str) -> str:
        return Path(join(self._dir, file_name)).read_text()

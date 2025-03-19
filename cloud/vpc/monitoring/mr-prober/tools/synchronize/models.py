import pathlib
from typing import List, Iterable, Tuple, Any

from pydantic import BaseModel

# This module contains frequently used parts of models.
# The models themselves are located in data/models/ and iac/models/.

__all__ = ["FileCollection", "VariableType"]

# Strictly speaking, VariableType is something like Union[float, int, str, bool, List, Dict],
# but we can't specify this type explicitly, because pydantic will convert
# all ints to floats or all bools to ints during YAML parsing.
#
# So we only assume this Union, but in the wild allow any type to be used in IaC configs.
#
VariableType = Any


class FileCollection(BaseModel):
    directory: str
    glob: str = "*"
    recursive: bool = False
    exclude: List[str] = []

    def get_files(self, base_path: pathlib.Path) -> Iterable[Tuple[pathlib.Path, pathlib.Path]]:
        files_directory = base_path / self.directory
        # TODO check that files_directory.is_dir()
        glob_func = files_directory.glob if not self.recursive else files_directory.rglob
        for filename in glob_func(self.glob):
            if not filename.is_file():
                continue
            relative_file_path = filename.relative_to(files_directory)
            if relative_file_path.as_posix() not in self.exclude:
                yield relative_file_path, filename

import logging
import os
import pathlib
import stat
from typing import TypeVar, List

FileModel = TypeVar("FileModel", "RecipeFile", "ProberFile")


def are_file_contents_changed(old_files: List[FileModel], new_files: List[FileModel]) -> bool:
    old_files_by_path = {file.relative_file_path: file for file in old_files}
    new_files_by_path = {file.relative_file_path: file for file in new_files}
    if old_files_by_path.keys() != new_files_by_path.keys():
        return True

    for file_path in old_files_by_path.keys():
        if new_files_by_path[file_path].content != old_files_by_path[file_path].content:
            return True

    return False


def save_files_to_disk(files: List[FileModel], path: pathlib.Path):
    path.mkdir(parents=True, exist_ok=True)
    for file in files:
        file_path = path / file.relative_file_path
        logging.debug(f"Saving content of recipe file to {file_path.as_posix()!r}...")
        file_path.write_bytes(file.content)


def is_file_executable(filename: pathlib.Path) -> bool:
    return bool(os.stat(filename).st_mode & stat.S_IXUSR)

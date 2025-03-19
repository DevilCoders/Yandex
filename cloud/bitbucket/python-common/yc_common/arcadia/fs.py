"""ArcadiaFS allows you to work with builtin resources.

Example:

    fs = ArcadiaFS()

    with fs.open_rb("my_module/default_config.yaml") as conf:
        default_config_yaml = conf.read()

    for file_name in fs.listdir("my_module/subfolder"):
        print(file_name)

    module_resources = fs.open_dir("my_module")
    with module_resources.open_rb("default_config.yaml") as conf:
        default_config_yaml = conf.read()
"""

import errno
import io
import os
import sys
from typing import BinaryIO, List, Optional, TextIO, Union


try:
    import library.python.resource as arcadia_res
except ImportError:
    arcadia_res = None


class _FileType:
    DIR = "dir"
    FILE = "file"
    ANY = "any"


class _ArcadiaDir:

    DEFAULT_ENCODING = "utf-8"

    def __init__(self, parent: Optional["_ArcadiaDir"], name: str):
        self.__parent = parent
        self.__name = name
        self.__full_path = self.__name if parent is None else os.path.join(parent.full_path, name)
        self.__dirs = {}
        self.__files = set()

    @property
    def name(self) -> str:
        return self.__name

    @property
    def full_path(self) -> str:
        return self.__full_path

    def add_child(self, path: List[str]):
        if len(path) == 1:
            self.__files.update([path[0]])
            return

        dir_name = path[0]
        tree = self.__dirs.get(dir_name)
        if tree is None:
            tree = _ArcadiaDir(self, dir_name)
            self.__dirs[dir_name] = tree
        tree.add_child(path[1:])

    def items(self) -> List[str]:
        items = []
        for i in self.__dirs:
            items.append(i)
        for i in self.__files:
            items.append(i)
        return items

    def __checked_relpath(self, relpath: str) -> List[str]:
        if os.path.isabs(relpath):
            raise ValueError("A relative path is required.")
        splitted = relpath.split(os.path.sep)
        if ".." in splitted:
            raise ValueError("Symbol '..' is not allowed.")
        return splitted

    def __lookup(self, relpath: str, file_type=_FileType.ANY) -> Union["_ArcadiaDir", BinaryIO]:
        path = self.__checked_relpath(relpath)

        if not path:
            return self  # lookup("") returns current directory

        if len(path) == 1:
            item = path[0]

            maybe_dir = self.__dirs.get(item)
            if maybe_dir is not None:
                if file_type == _FileType.FILE:
                    raise IsADirectoryError(errno.EISDIR, os.path.join(self.full_path, item))
                return maybe_dir

            if item in self.__files:
                if file_type == _FileType.DIR:
                    raise NotADirectoryError(errno.ENOTDIR, os.path.join(self.full_path, item))
                return self.__open_rb(item)

            raise FileNotFoundError(errno.ENOENT, os.path.join(self.full_path, item))

        subdir, subpath = path[0], path[1:]
        if subdir not in self.__dirs:
            raise FileNotFoundError(errno.ENOENT, os.path.join(self.full_path, subdir))
        node = self.__dirs[subdir]
        return node.__lookup(os.path.join(*subpath), file_type=file_type)

    def __open_rb(self, file_name: str) -> BinaryIO:
        content = arcadia_res.find(os.path.join(self.full_path, file_name))
        return io.BytesIO(content)

    def open_dir(self, relpath: str) -> "_ArcadiaDir":
        return self.__lookup(relpath, file_type=_FileType.DIR)

    def open_rb(self, relpath: str) -> BinaryIO:
        return self.__lookup(relpath, file_type=_FileType.FILE)

    def open_r(self, relpath: str, encoding=None) -> TextIO:
        with self.open_rb(relpath) as f:
            return io.StringIO(str(f.read(), encoding or self.DEFAULT_ENCODING))

    def listdir(self, relpath: str) -> List[str]:
        d = self.open_dir(relpath)
        return d.items()

    def isfile(self, relpath: str) -> bool:
        dirname = os.path.dirname(relpath)
        if not dirname:
            return relpath in self.__files
        try:
            return self.__lookup(dirname, file_type=_FileType.DIR).isfile(os.path.basename(relpath))
        except OSError:
            return False


# FIXME(zasimov-a): methods rescan resource on each call
class ArcadiaFS:
    def __init__(self, prefix: Optional[str]=None):
        self.__root = _ArcadiaDir(None, "")
        self.__rescan(prefix=prefix)

    def __rescan(self, prefix=None):
        for i in range(arcadia_res.count()):
            path = str(arcadia_res.key_by_index(i), sys.getfilesystemencoding())
            if os.path.isabs(path):
                continue  # skip /fs/...
            if prefix is not None and not path.startswith(prefix):
                continue
            self.__root.add_child(path.split(os.path.sep))

    def open_dir(self, relpath: str) -> _ArcadiaDir:
        self.__rescan(prefix=relpath)
        return self.__root.open_dir(relpath)

    def open_rb(self, relpath: str) -> BinaryIO:
        self.__rescan(prefix=relpath)
        return self.__root.open_rb(relpath)

    def open_r(self, relpath: str, encoding=None) -> TextIO:
        self.__rescan(prefix=relpath)
        return self.__root.open_r(relpath, encoding=encoding)

    def listdir(self, relpath: str) -> List[str]:
        self.__rescan(prefix=relpath)
        d = self.__root.open_dir(relpath)
        return d.items()

    def isfile(self, relpath: str) -> bool:
        self.__rescan(prefix=relpath)
        return self.__root.isfile(relpath)

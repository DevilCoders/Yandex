"""Module allows to work with builtin resources in unified way."""

import os
from typing import BinaryIO, List, TextIO

from yc_common import arcadia


_ARCADIA_PACKAGE_RESOURCES_FOLDER = "package_resources"


def package_resources(module):
    if arcadia.is_arcadia():
        return arcadia.fs.open_dir(os.path.join(_ARCADIA_PACKAGE_RESOURCES_FOLDER, module.__name__))
    else:
        return _PackageResources(module)


class _PackageResources:
    def __init__(self, module):
        self.__module = module

    @staticmethod
    def __check_relpath(relpath: str):
        if os.path.isabs(relpath):
            raise ValueError("A relative path is required.")
        splitted = relpath.split(os.path.sep)
        if ".." in splitted:
            raise ValueError("Symbol '..' is not allowed.")

    def __abs_path(self, relpath: str) -> str:
        return os.path.join(os.path.dirname(self.__module.__file__), relpath)

    def open_rb(self, relpath: str) -> BinaryIO:
        self.__check_relpath(relpath)
        return open(self.__abs_path(relpath), "rb")

    def open_r(self, relpath: str, encoding=None) -> TextIO:
        self.__check_relpath(relpath)
        return open(self.__abs_path(relpath), "r", encoding=encoding)

    def listdir(self, relpath: str) -> List[str]:
        self.__check_relpath(relpath)
        return os.listdir(self.__abs_path(relpath))

    def isfile(self, relpath: str) -> bool:
        self.__check_relpath(relpath)
        return os.path.isfile(self.__abs_path(relpath))

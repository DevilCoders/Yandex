import pathlib
from typing import Type, TypeVar

import yaml
from pydantic import BaseModel

from tools.synchronize.iac import utils

T = TypeVar("T")


class BaseIacModel(BaseModel):
    def _dump_object_to_file(self, path: pathlib.Path):
        with path.open(encoding="utf-8", mode="w") as file:
            yaml.dump(self.dict(exclude_unset=True), file, Dumper=utils.EnumSupportedSafeDumper, sort_keys=False)

    @classmethod
    def _create_object_from_file(cls: Type[T], path: pathlib.Path) -> T:
        with path.open(encoding="utf-8") as file:
            return cls.parse_obj(yaml.load(file, Loader=utils.UniqueKeySafeLoader))

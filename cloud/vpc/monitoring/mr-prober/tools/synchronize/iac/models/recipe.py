import logging
import pathlib
from typing import List

import yaml
import yaml.constructor
from pydantic import ValidationError

from tools.synchronize.models import FileCollection
from tools.synchronize.iac.errors import IacDataError
from tools.synchronize.iac.models.common import BaseIacModel

__all__ = ["Recipe"]


class Recipe(BaseIacModel):
    name: str
    description: str
    files: List[FileCollection]

    @classmethod
    def load_from_file(cls, path: pathlib.Path) -> "Recipe":
        try:
            return cls._create_object_from_file(path)
        except (ValidationError, yaml.constructor.ConstructorError) as e:
            logging.error(f"Invalid recipe in [bold]{path}[/bold]: {e}")
            raise IacDataError(f"Invalid recipe in {path}") from e

    def save_to_file(self, path: pathlib.Path):
        self._dump_object_to_file(path)

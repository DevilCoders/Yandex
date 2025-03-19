import glob
import logging
import pathlib
from typing import Dict, List, Iterable, Optional

import pydantic
import yaml
import yaml.constructor
from pydantic import BaseModel

from api.client import MrProberApiClient
from tools.synchronize.iac.errors import IacDataError
from tools.synchronize.iac.models.common import BaseIacModel

__all__ = ["Config", "Environment"]


class Environment(BaseModel):
    endpoint: str
    clusters: List[str]
    probers: List[str]

    @property
    def cluster_files(self) -> Iterable[str]:
        for cluster_glob in self.clusters:
            yield from glob.glob(cluster_glob, recursive=True)

    @property
    def prober_files(self) -> Iterable[str]:
        for prober_glob in self.probers:
            yield from glob.glob(prober_glob, recursive=True)

    def get_mr_prober_api_client(self, api_key: Optional[str] = None):
        return MrProberApiClient(self.endpoint, api_key=api_key, cache_control="no-store")


class Config(BaseIacModel):
    environments: Dict[str, Environment]

    @classmethod
    def load_from_file(cls, path: pathlib.Path) -> "Config":
        try:
            return cls._create_object_from_file(path)
        except (pydantic.ValidationError, yaml.constructor.ConstructorError) as e:
            logging.error(f"Invalid configuration: {e}")
            raise IacDataError("Invalid configuration") from e

    def save_to_file(self, path: pathlib.Path):
        self._dump_object_to_file(path)

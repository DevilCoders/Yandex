import logging
import pathlib
from typing import Optional, List, Dict, Any

import yaml
import yaml.constructor
from pydantic import BaseModel, ValidationError

import api.models
import database.models
from tools.synchronize.models import FileCollection
from tools.synchronize.iac.errors import IacDataError
from tools.synchronize.iac.models.common import BaseIacModel

__all__ = ["Prober", "ProberConfig"]


class ProberConfig(BaseModel):
    is_prober_enabled: Optional[bool] = None
    interval_seconds: Optional[int] = None
    timeout_seconds: Optional[int] = None
    s3_logs_policy: Optional[database.models.UploadProberLogPolicy] = None
    default_routing_interface: Optional[str] = None
    dns_resolving_interface: Optional[str] = None
    clusters: Optional[List[str]] = None
    hosts_re: Optional[str] = None
    matrix: Optional[Dict[str, List[Any]]] = None
    variables: Optional[Dict[str, Any]] = None


class Prober(BaseIacModel):
    name: str
    slug: Optional[str]
    description: str
    runner: api.models.ProberRunner
    files: List[FileCollection]
    configs: List[ProberConfig]

    @classmethod
    def load_from_file(cls, path: pathlib.Path) -> "Prober":
        try:
            prober = cls._create_object_from_file(path)
        except (ValidationError, yaml.constructor.ConstructorError) as e:
            logging.error(f"Invalid prober in [bold]{path}[/bold]: {e}")
            raise IacDataError(f"Invalid prober in {path}") from e

        if prober.slug is None:
            prober.slug = path.parent.name

        return prober

    def save_to_file(self, path: pathlib.Path):
        self._dump_object_to_file(path)

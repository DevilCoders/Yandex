import logging
import pathlib
from typing import Optional, Dict, List

import yaml
import yaml.constructor
from pydantic import ValidationError

import api.models
from tools.synchronize.iac.errors import IacDataError
from tools.synchronize.iac.models.common import BaseIacModel
from tools.synchronize.models import VariableType

__all__ = ["Cluster"]


class Cluster(BaseIacModel):
    name: str
    slug: Optional[str]
    recipe: str
    variables: Dict[str, VariableType] = {}
    untracked_variables: List[str] = []
    deploy_policy: Optional[api.models.ClusterDeployPolicy] = None

    @classmethod
    def load_from_file(cls, path: pathlib.Path) -> "Cluster":
        try:
            cluster = cls._create_object_from_file(path)
        except (ValidationError, yaml.constructor.ConstructorError) as e:
            logging.error(f"Invalid cluster in [bold]{path}[/bold]: {e}")
            raise IacDataError(f"Invalid cluster in {path}") from e

        if cluster.slug is None:
            cluster.slug = path.parent.name

        return cluster

    def save_to_file(self, path: pathlib.Path):
        self._dump_object_to_file(path)

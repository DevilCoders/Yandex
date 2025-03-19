import os
from typing import Dict

import pydantic.parse
import yaml
from pydantic import BaseModel, Protocol

from api.client import MrProberApiClient
from tools.common import StopCliProcess, ExitCode


class SolomonConfig(BaseModel):
    endpoint: str
    project: str
    cluster: str
    service: str


class Cluster(BaseModel):
    cluster_id: int


class Environment(BaseModel):
    endpoint: str
    solomon: SolomonConfig
    clusters: Dict[str, Cluster]


class Config(BaseModel):
    environments: Dict[str, Environment]


def load_environment(config_path: str, environment_name: str) -> Environment:
    try:
        raw_cfg = pydantic.parse.load_file(config_path, proto=Protocol.json,
                                           json_loads=yaml.safe_load)
        cfg = Config.parse_obj(raw_cfg)
    except FileNotFoundError as e:
        raise StopCliProcess(f"Config file {config_path} is not found", ExitCode.CONFIG_NOT_FOUND)
    except (pydantic.ValidationError, yaml.constructor.ConstructorError) as e:
        raise StopCliProcess(f"Cannot parse config file {config_path}: {e}", ExitCode.BAD_CONFIG)

    try:
        return cfg.environments[environment_name]
    except KeyError:
        raise StopCliProcess(f"Cannot find environment {environment_name} in config {config_path}",
                             ExitCode.ENVIRONMENT_NOT_FOUND)


def get_mr_prober_api_client(environment: Environment) -> MrProberApiClient:
    # There are many APIs out there. Do not assume that API_KEY is universal
    api_key = os.environ.get("MR_PROBER_API_KEY")
    if not api_key:
        api_key = os.environ.get("API_KEY")

    if not api_key:
        raise StopCliProcess(f"MR_PROBER_API_KEY environment variable is not specified or empty")

    return MrProberApiClient(environment.endpoint, api_key=api_key)

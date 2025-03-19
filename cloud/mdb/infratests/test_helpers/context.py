# -*- coding: utf-8 -*-
import logging
import requests
from dataclasses import dataclass
from typing import Dict, Sequence

from cloud.mdb.infratests.config import InfratestConfig
from cloud.mdb.infratests.test_helpers.types import CliResult, Cluster, Host, Subcluster


@dataclass
class Context:
    cid: str
    cli_result: CliResult
    cluster: Cluster
    cluster_type: str
    job_id: str
    logger: logging.Logger
    hosts: Sequence[Host]
    operation_id: str
    response: requests.Response
    session: requests.Session
    subclusters: Sequence[Subcluster]
    subcluster_id_by_name: Dict[str, str]
    test_config: InfratestConfig
    text: str
    user_iam_token: str

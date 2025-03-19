# -*- coding: utf-8 -*-
from dataclasses import dataclass
from typing import Optional, TypedDict


@dataclass
class CliResult:
    exit_code: int
    command: str
    stdout: Optional[bytes]
    stderr: Optional[bytes]


class Cluster(TypedDict):
    health: str
    id: str
    name: str
    status: str


class Job(TypedDict):
    id: str
    status: str


class Host(TypedDict):
    computeInstanceId: str
    name: str
    role: str
    subclusterId: str


class Subcluster(TypedDict):
    id: str
    name: str

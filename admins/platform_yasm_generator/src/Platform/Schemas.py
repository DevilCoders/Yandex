from collections import defaultdict
from dataclasses import field
from typing import List, ClassVar, Type, Dict, Optional

from marshmallow import EXCLUDE, Schema  # noqa: F401
from marshmallow_dataclass import dataclass


@dataclass
class Base:
    Schema: ClassVar[Type[Schema]] = Schema

    class Meta:
        unknown = EXCLUDE


@dataclass
class Route(Base):
    location: str
    componentName: str
    proxyReadTimeout: str
    proxyWriteTimeout: str
    proxyConnectTimeout: str
    proxyNextUpstreamTries: int
    proxyNextUpstreamTimeout: int


@dataclass
class Router(Base):
    name: str
    urlPath: str
    balancerType: str


@dataclass
class L7Balancer(Base):
    name: str
    environmentId: str


@dataclass
class Component(Base):
    name: str
    objectId: str
    applicationName: str


@dataclass
class Instance(Base):
    name: str
    objectId: str
    applicationName: str
    useHealthCheck: bool
    healthCheckRise: int
    healthCheckFall: int
    healthCheckTimeout: int


@dataclass
class Environment(Base):
    name: str
    objectId: str
    admin: bool
    applicationName: str
    projectName: str
    version: Optional[int]
    routes: Optional[List[Route]]
    routers: Optional[List[Router]]
    components: Dict[str, Component] = field(default_factory=defaultdict(list))


@dataclass
class Application(Base):
    name: str
    objectId: str
    environments: List[Environment]


@dataclass
class Project(Base):
    name: str
    applications: List[Application]

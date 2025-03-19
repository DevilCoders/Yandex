from typing import List, NamedTuple, Optional
from enum import Enum


class Network(NamedTuple):
    """
    YC Network
    """

    network_id: str
    folder_id: str
    default_security_group_id: str


class Subnet(NamedTuple):
    """
    YC Subnet
    """

    subnet_id: str
    network_id: str
    folder_id: str
    zone_id: str
    v4_cidr_blocks: List[str]
    v6_cidr_blocks: List[str]
    egress_nat_enable: bool


class SecurityGroupRuleDirection(Enum):
    """
    Possible Security Group Rule Direction
    """

    UNSPECIFIED = 0
    INGRESS = 1
    EGRESS = 2


class SecurityGroupRule(NamedTuple):
    """
    YC Security Group Rule
    """

    id: str
    description: str
    direction: SecurityGroupRuleDirection
    ports_from: Optional[int]
    ports_to: Optional[int]
    protocol_name: str
    protocol_number: int
    v4_cidr_blocks: List[str]
    v6_cidr_blocks: List[str]
    predefined_target: Optional[str]
    security_group_id: Optional[str]


class SecurityGroup(NamedTuple):
    """
    YC Security Group
    """

    id: str
    name: str
    folder_id: str
    network_id: str
    default_for_network: bool
    rules: List[SecurityGroupRule]

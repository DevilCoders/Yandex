from typing import NamedTuple, Optional
from enum import Enum

from datacloud.network.v1 import network_pb2


class NetworkStatus(Enum):
    INVALID = 0
    CREATING = 1
    ACTIVE = 2
    DELETING = 3
    ERROR = 4


class AWSSubnetResource(NamedTuple):
    subnet_id: str
    zone_id: str


class AWSExternalResources(NamedTuple):
    vpc_id: str
    security_group_id: str
    subnets: list[AWSSubnetResource]
    account_id: Optional[str]
    iam_role_arn: Optional[str]


class Network(NamedTuple):
    network_id: str
    project_id: str
    provider: str
    region_id: str
    ipv4_cidr_block: str
    ipv6_cidr_block: str
    status: NetworkStatus
    aws_external_resources: Optional[AWSExternalResources]


def network_from_pb(net: network_pb2.Network) -> Network:
    return Network(
        network_id=net.id,
        project_id=net.project_id,
        provider=net.cloud_type,
        region_id=net.region_id,
        ipv4_cidr_block=net.ipv4_cidr_block,
        ipv6_cidr_block=net.ipv6_cidr_block,
        status=NetworkStatus(net.status),
        aws_external_resources=AWSExternalResources(
            vpc_id=net.aws.vpc_id,
            security_group_id=net.aws.security_group_id,
            subnets=[
                AWSSubnetResource(
                    subnet_id=subnet.id,
                    zone_id=subnet.zone_id,
                )
                for subnet in net.aws.subnets
            ],
            account_id=net.aws.account_id.value if net.aws.HasField('account_id') else None,
            iam_role_arn=net.aws.iam_role_arn.value if net.aws.HasField('iam_role_arn') else None,
        ),
    )

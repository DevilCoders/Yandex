from typing import NamedTuple, Callable

from cloud.mdb.internal.python import grpcutil
from cloud.mdb.internal.python.grpcutil import exceptions
from cloud.mdb.internal.python.logs import MdbLoggerAdapter

from datacloud.network.v1 import (
    network_service_pb2,
    network_service_pb2_grpc,
)

from . import errors, models


class Config(NamedTuple):
    transport: grpcutil.Config
    timeout: float = 5.0


class API(object):
    def __init__(self, config: Config, logger: MdbLoggerAdapter, token_getter: Callable[[], str]):
        self.config = config
        self.logger = logger

        self.logger.info('init VPC provider channel with transport: %s', repr(config.transport))
        channel = grpcutil.new_grpc_channel(config.transport)
        self.network_service = grpcutil.WrappedGRPCService(
            logger=self.logger,
            channel=channel,
            grpc_service=network_service_pb2_grpc.NetworkServiceStub,
            timeout=self.config.timeout,
            get_token=token_getter,
            error_handlers={},
        )

    def get_network(self, network_id: str) -> models.Network:
        try:
            return models.network_from_pb(
                self.network_service.Get(network_service_pb2.GetNetworkRequest(network_id=network_id))
            )
        except exceptions.NotFoundError:
            self.logger.warn("Network was not found %s", network_id)
            raise errors.VPCNotFound(f"Network was not found {network_id}")

    def list_networks(self, project_id: str) -> list[models.Network]:
        self.logger.info('Listing networks in %s, %s', project_id, self.config.transport.url)

        try:
            res = self.network_service.List(
                network_service_pb2.ListNetworksRequest(project_id=project_id),
            )
        except exceptions.NotFoundError:
            self.logger.warn("Networks were not found in %s", project_id)
            raise errors.VPCNotFound(f"Networks were not found in {project_id}")

        return [models.network_from_pb(net) for net in res.networks]

    def create_network(
        self, project_id: str, provider: str, region_id: str, name: str, description: str, ipv4_cidr_block: str
    ) -> str:
        self.logger.info('Create network in %s, %s', project_id, self.config.transport.url)
        return self.network_service.Create(
            network_service_pb2.CreateNetworkRequest(
                project_id=project_id,
                cloud_type=provider,
                region_id=region_id,
                name=name,
                description=description,
                ipv4_cidr_block=ipv4_cidr_block,
            ),
        ).network_id

    def list_networks_in_region(self, project_id: str, region_id: str) -> list[models.Network]:
        nets = self.list_networks(project_id)
        if res := list(filter(lambda net: net.region_id == region_id, nets)):
            return res
        else:
            self.logger.warning("Networks were not found in project %s in region %s", project_id, region_id)
            raise errors.VPCNotFound(f"Networks were not found in project  {project_id} in region {region_id}")

import uuid
from cloud.mdb.internal.python import grpcutil
from dbaas_common import tracing
from typing import Callable, Iterable, NamedTuple
from yandex.cloud.priv.dns.v1 import (
    dns_zone_service_pb2,
    dns_zone_service_pb2_grpc,
    operation_service_pb2,
    operation_service_pb2_grpc,
)

from cloud.mdb.internal.python.compute.service import ComputeGRPCService
from cloud.mdb.internal.python.compute.models import OperationModel
from cloud.mdb.internal.python.grpcutil.retries import client_retry
from cloud.mdb.internal.python.grpcutil.exceptions import GRPCError
from cloud.mdb.internal.python.logs import MdbLoggerAdapter
from .models import DnsRecordSet


class DnsClientConfig(NamedTuple):
    transport: grpcutil.Config
    timeout: float = 15.0

    @staticmethod
    def make(url: str, cert_file: str) -> 'DnsClientConfig':
        return DnsClientConfig(
            transport=grpcutil.Config(
                url=url,
                cert_file=cert_file,
            )
        )


def list_records_filter(name: str, types: Iterable[str]) -> str:
    records_filter = [f'name="{name}"']
    if types:
        records_filter.append('type IN (%s)' % ", ".join(['"%s"' % t for t in types]))
    return ' AND '.join(records_filter)


class DnsClient:
    "Yandex Cloud DNS (ex Rurik.DNS) client"
    __channel = None
    __dsn_zone_service = None
    __operation_service = None

    def __init__(
        self,
        config: DnsClientConfig,
        logger: MdbLoggerAdapter,
        token_getter: Callable[[], str],
        error_handlers: dict[GRPCError, Callable[[GRPCError], None]],
    ) -> None:
        self.config = config
        self.logger = logger.copy_with_ctx(client_name='DNSClient')
        self.token_getter = token_getter
        self.error_handlers = error_handlers

    def __new_service(self, stub):
        if self.__channel is None:
            self.__channel = grpcutil.new_grpc_channel(self.config.transport)
        return ComputeGRPCService(
            self.logger,
            self.__channel,
            stub,
            timeout=self.config.timeout,
            get_token=self.token_getter,
            error_handlers=self.error_handlers,
        )

    @property
    def _dns_zone_service(self):
        if self.__dsn_zone_service is None:
            self.__dsn_zone_service = self.__new_service(dns_zone_service_pb2_grpc.DnsZoneServiceStub)
        return self.__dsn_zone_service

    @property
    def _operation_service(self):
        if self.__operation_service is None:
            self.__operation_service = self.__new_service(operation_service_pb2_grpc.OperationServiceStub)
        return self.__operation_service

    @client_retry
    @tracing.trace('YC.DNS list records with name')
    def list_records(self, dns_zone_id: str, name: str, types: Iterable[str] = ('A', 'AAAA')) -> list[DnsRecordSet]:
        """
        Get DNS zone records with given name
        """
        request_id = str(uuid.uuid4())
        tracing.set_tag('dns.zone_id', dns_zone_id)
        tracing.set_tag('dns.record.name', name)
        tracing.set_tag('request_id', request_id)
        # Sadly, there are no to specify page_size,
        # but it should be ok, cause we get recodes only for one name
        request = dns_zone_service_pb2.ListDnsZoneRecordSetsRequest(
            dns_zone_id=dns_zone_id,
            filter=list_records_filter(name, types),
        )
        self.logger.debug('ListDnsZoneRecordSetsRequest = %r', request)
        response = self._dns_zone_service.ListRecordSets(request, request_id=request_id)
        if response.next_page_token:
            raise RuntimeError(
                f'Got more then one page with DNS zone {dns_zone_id=} records for {name=}, next_page_token: {response.next_page_token}'
            )
        return [DnsRecordSet.from_api(raw) for raw in response.record_sets]

    @client_retry
    @tracing.trace('YC.DNS upsert records')
    def upsert(
        self,
        dns_zone_id: str,
        deletions: Iterable[DnsRecordSet] = None,
        replacements: Iterable[DnsRecordSet] = None,
        merges: Iterable[DnsRecordSet] = None,
    ) -> None:
        """
        Upsert DNS records in given zone
        """
        request_id = str(uuid.uuid4())
        tracing.set_tag('dns.zone_id', dns_zone_id)
        tracing.set_tag('request_id', request_id)
        request = dns_zone_service_pb2.UpsertRecordSetsRequest(
            dns_zone_id=dns_zone_id,
            deletions=[rec.to_api() for rec in deletions or []],
            replacements=[rec.to_api() for rec in replacements or []],
            merges=[rec.to_api() for rec in merges or []],
        )
        self.logger.debug('UpsertRecordSetsRequest = %r', request)
        operation = self._dns_zone_service.UpsertRecordSets(request, request_id=request_id)
        return OperationModel().operation_from_api(
            self.logger,
            operation,
            self._dns_zone_service.error_from_rich_status,
        )

    @client_retry
    @tracing.trace('YC.DNS Operations Get')
    def get_operation(self, operation_id: str) -> OperationModel:
        """
        Get operation by id
        """
        tracing.set_tag('dns.operation.id', operation_id)
        request = operation_service_pb2.GetOperationRequest()
        request.operation_id = str(operation_id)
        with self.logger.context(operation_id=operation_id):
            operation = OperationModel().operation_from_api(
                self.logger,
                self._operation_service.Get(request),
                self._operation_service.error_from_rich_status,
            )
            if operation.error:
                self.logger.info('Operation "%s" ended with error: "%s"', operation_id, operation.error)
        return operation

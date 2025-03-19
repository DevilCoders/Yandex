# -*- coding: utf-8 -*-
import logging
import time
from typing import Generator, List, NamedTuple

from yandex.cloud.priv.dns.v1 import (
    dns_zone_pb2,
    dns_zone_service_pb2,
    dns_zone_service_pb2_grpc,
    operation_service_pb2,
    operation_service_pb2_grpc,
)

from cloud.mdb.internal.python import grpcutil
from cloud.mdb.internal.python.logs import MdbLoggerAdapter
from cloud.mdb.infratests.config import InfratestConfig
from cloud.mdb.infratests.provision.iam import create_iam_jwt


class DnsRecord(NamedTuple):
    name_prefix: str
    ip: str


def list_records(config: InfratestConfig, logger: logging.Logger) -> List[dns_zone_pb2.RecordSet]:
    return DnsClient(config, logger).list_records()


def create_records(config: InfratestConfig, logger: logging.Logger, records: List[DnsRecord]):
    DnsClient(config, logger).create_records(records)


def delete_records(config: InfratestConfig, logger: logging.Logger):
    DnsClient(config, logger).delete_records()


class DnsClient:
    def __init__(self, config: InfratestConfig, logger: logging.Logger):
        self.config: InfratestConfig = config
        self.logger: logging.Logger = logger
        self.domain_name_suffix = f".{config.base_domain}."

        iam_jwt = create_iam_jwt(config, logger)

        dns_config = config.dns
        channel = grpcutil.new_grpc_channel(
            grpcutil.Config(
                url=dns_config.url,
                cert_file=dns_config.cert_file,
                server_name=dns_config.server_name,
                insecure=dns_config.insecure,
            )
        )
        self.dns_zone_service = grpcutil.WrappedGRPCService(
            MdbLoggerAdapter(logger, extra={}),
            channel,
            dns_zone_service_pb2_grpc.DnsZoneServiceStub,
            timeout=dns_config.timeout,
            get_token=iam_jwt.get_token,
            error_handlers={},
        )

        self.operation_service = grpcutil.WrappedGRPCService(
            MdbLoggerAdapter(logger, extra={}),
            channel,
            operation_service_pb2_grpc.OperationServiceStub,
            timeout=dns_config.timeout,
            get_token=iam_jwt.get_token,
            error_handlers={},
        )

    def list_records(self) -> List[dns_zone_pb2.RecordSet]:
        return [rs for rs in self._records_iterator() if rs.name.endswith(self.domain_name_suffix)]

    def _records_iterator(self) -> Generator[dns_zone_pb2.RecordSet, None, None]:
        page_token = ''
        while True:
            req = dns_zone_service_pb2.ListDnsZoneRecordSetsRequest(
                dns_zone_id=self.config.dns.zone_id, page_token=page_token
            )
            resp = self.dns_zone_service.ListRecordSets(req)
            for rs in resp.record_sets:
                yield rs
            page_token = resp.next_page_token
            if not page_token:
                break

    def create_records(self, records: List[DnsRecord]):
        record_sets = []
        for record in records:
            rs = dns_zone_pb2.RecordSet(
                name=f"{record.name_prefix}{self.domain_name_suffix}", type="AAAA", ttl=600, data=[record.ip]
            )
            record_sets.append(rs)
        req = dns_zone_service_pb2.UpsertRecordSetsRequest(
            dns_zone_id=self.config.dns.zone_id, replacements=record_sets
        )
        self.logger.debug(f'Sending request to dns api: {req}')
        operation = self.dns_zone_service.UpsertRecordSets(req)
        self._operation_wait(operation.id)

    def delete_records(self):
        record_sets = self.list_records()
        if not record_sets:
            return

        self.logger.info("Following dns records will be deleted:")
        for rs in record_sets:
            self.logger.info(f"  {rs.name} => {rs.data}")

        req = dns_zone_service_pb2.UpsertRecordSetsRequest(dns_zone_id=self.config.dns.zone_id, deletions=record_sets)
        self.logger.debug(f'Sending request to dns api: {req}')
        operation = self.dns_zone_service.UpsertRecordSets(req)
        self._operation_wait(operation.id)

    def _get_operation(self, operation_id):
        """
        Get operation by id
        """
        request = operation_service_pb2.GetOperationRequest(operation_id=str(operation_id))
        return self.operation_service.Get(request)

    def _operation_wait(self, operation_id: str, timeout=600):
        """
        Wait while operation finishes
        """
        if operation_id is None:
            return
        stop_time = time.time() + timeout
        while time.time() < stop_time:
            operation = self._get_operation(operation_id)
            if not operation.done:
                self.logger.debug('Waiting for operation %s', operation_id)
                time.sleep(1)
                continue
            if operation.HasField('error'):
                raise RuntimeError(f'Operation {operation_id} failed with error {operation.error}')
            return

        raise RuntimeError(f'{timeout}s passed. Operation {operation_id} is still running')

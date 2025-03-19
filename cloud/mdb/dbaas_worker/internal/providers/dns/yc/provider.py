"""
Yandex Cloud DNS provider
"""
import time
from typing import Optional
from dbaas_common import tracing
from ...common import BaseProvider, Change
from ...iam_jwt import IamJwt

from ....exceptions import ExposedException
from ..client import Record, BaseDnsClient, ensure_dot, normalized_address, records_stable_hash
from .upsert import RecordsType2Addresses, UpsertPlan, UpsertRequest, records_to_type_map
from cloud.mdb.internal.python.compute.dns import (
    DnsClient,
    DnsClientConfig,
    DnsRecordSet,
)


class YCDnsError(ExposedException):
    pass


class OperationTimedOutError(YCDnsError):
    pass


class YCDns(BaseDnsClient, BaseProvider):
    """
    Yandex.Cloud DNS (ex known Rurik.DNS) provider

    We want to mainly use Compute integration with YC.DNS.
    (Passing DnsRecordsSpec to compute/InstanceService.Create)
    https://cloud.yandex.ru/docs/dns/concepts/compute-integration

    But currently we can't use it in all cases.
    DNS records for One-To-One-Nat (public_ip in our APIs)
    is not implemented at the moment - CLOUD-63662.

    But we decided to use this integration for:
    - Our managed FQDN
    - Public ipv6 FQDN

    Our Dns provider set all (managed and user) records.
    So that provide has two workarounds:

    1. yandex_cloud_dns.ignored_zones, cause we don't want
       to touch manged_zone (.mdb.yandexcloud.co.il. in Israel)
    2. yandex_cloud_dns.record_types, cause we don't want
       to touch 'public' ipv6 records that created by
       YC.DNS and Compute integration.
    """

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.iam_jwt = IamJwt(config, task, queue)
        self._dns_client = DnsClient(
            config=DnsClientConfig.make(
                url=self.config.yc_dns.url,
                cert_file=self.config.yc_dns.ca_path,
            ),
            logger=self.logger,
            token_getter=self.iam_jwt.get_token,
            error_handlers={},
        )
        self._zones: dict[str, str] = self.config.yc_dns.zones
        self._ignored_zones: frozenset[str] = frozenset(self.config.yc_dns.ignored_zones)
        self._record_types: frozenset[str] = frozenset(self.config.yc_dns.record_types)
        self._ttl = self.config.yc_dns.ttl
        self._operation_wait_timeout: int = self.config.yc_dns.operation_wait_timeout
        self._operation_wait_step: float = self.config.yc_dns.operation_wait_step

    def _define_zone_id(self, fqdn: str) -> Optional[str]:
        fqdn_zone = fqdn[fqdn.find('.') + 1 :]
        if fqdn_zone not in self._zones:
            if fqdn_zone in self._ignored_zones:
                self.logger.debug(
                    'YC.DNS ignore %s, because that zone is in ignored_zones: %r', fqdn, self._ignored_zones
                )
                return None
            raise RuntimeError(
                f'Unsupported zone {fqdn_zone} for {fqdn=}. Supported are: {self._zones}. Ignored are: {self._ignored_zones}'
            )
        return self._zones[fqdn_zone]

    def _get_records(self, fqdn: str, dns_zone_id: str) -> RecordsType2Addresses:
        actual = RecordsType2Addresses()
        for record in self._dns_client.list_records(dns_zone_id, fqdn, self._record_types):
            actual[record.type] = set(normalized_address(addr, record.type) for addr in record.data)
        return actual

    @tracing.trace('YC.DNS Set Records Operation Wait')
    def _operation_wait(self, operation_id: str) -> None:
        """
        Wait while operation finishes.
        """
        tracing.set_tag('dns.operation.id', operation_id)
        deadline = time.time() + self._operation_wait_timeout
        with self.interruptable:
            while time.time() < deadline:
                operation = self._dns_client.get_operation(operation_id)
                if not operation.done:
                    self.logger.info('Waiting for YC.DNS operation %s', operation_id)
                    time.sleep(self._operation_wait_step)
                    continue
                if not operation.error:
                    return
                raise YCDnsError(
                    message=operation.error.message, err_type=operation.error.err_type, code=operation.error.code
                )

            raise OperationTimedOutError(
                f'{self._operation_wait_timeout}s passed. YC.DNS operation {operation_id} is still running'
            )

    def _apply_upsert(self, dns_zone_id: str, fqdn: str, request: UpsertRequest) -> str:
        def mk_records_sets(request_map: RecordsType2Addresses) -> list[DnsRecordSet]:
            ret = []
            for record_type, addresses in request_map.items():
                ret.append(
                    DnsRecordSet(
                        name=fqdn,
                        type=record_type,
                        data=list(addresses),
                        ttl=self._ttl,
                    )
                )
            return ret

        operation = self._dns_client.upsert(
            dns_zone_id=dns_zone_id,
            deletions=mk_records_sets(request.deletions),
            merges=mk_records_sets(request.merges),
        )
        self.logger.info('YC.DNS upsert operation: %r', operation)
        return operation.operation_id

    @tracing.trace('YC.DNS Set Records')
    def set_records(self, fqdn: str, records: list[Record]) -> None:
        fqdn = ensure_dot(fqdn)
        dns_zone_id = self._define_zone_id(fqdn)
        if not dns_zone_id:
            return
        supported_records = [r for r in records if r.record_type in self._record_types]

        # It's sad, but we can't use simple '{fqdn}.records-upsert' key in context,
        # Because in some task we set_records for same FQDN twice.
        context_key = f'yc.dns.{fqdn}.records-upsert.{records_stable_hash(records)}'
        operation_id = ''

        if context := self.context_get(context_key):
            operation_id = context['operation_id']
            rollback_plan = UpsertRequest.from_serializable(context['rollback_plan'])
            self.add_change(
                Change(
                    f'yc.dns.{fqdn}{len(supported_records)}-records-upsert.got-from-context',
                    operation_id,
                    context=context,
                    rollback=lambda task, safe_revision: self._apply_upsert(dns_zone_id, fqdn, rollback_plan),
                )
            )
            self.logger.info('Got YC.DNS %r operation from context', operation_id)
        else:
            actual = self._get_records(fqdn, dns_zone_id)
            expected = records_to_type_map(supported_records)

            upsert = UpsertPlan.build(actual=actual, expected=expected)
            self.logger.debug('%s set_records upsert plan is %r', fqdn, upsert)
            if upsert.plan.is_empty():
                self.logger.info('All %s records %r are in correct state', fqdn, supported_records)
                return
            operation_id = self._apply_upsert(dns_zone_id, fqdn, upsert.plan)
            context = {
                context_key: {
                    'operation_id': operation_id,
                    'rollback_plan': upsert.rollback_plan.to_serializable(),
                }
            }

            self.add_change(
                Change(
                    f'yc.dns.{fqdn}{len(supported_records)}-records-upsert.initiated',
                    operation_id,
                    context=context,
                    rollback=lambda task, safe_revision: self._apply_upsert(dns_zone_id, fqdn, upsert.rollback_plan),
                )
            )

        if operation_id:
            self._operation_wait(operation_id)

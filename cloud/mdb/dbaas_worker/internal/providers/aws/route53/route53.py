from ....exceptions import ExposedException
from ...common import BaseProvider, Change
from ...dns import Record

from typing import Callable
from dbaas_common import tracing

import boto3
from botocore.config import Config


class Route53APIError(ExposedException):
    """
    Base Route53 error
    """


class Route53DisabledError(RuntimeError):
    """
    Route 53 provider not initialized. Enable it in config'
    """


class MisconfiguredEnvError(ExposedException):
    pass


def _add_dot_to_fqdn(fqdn: str) -> str:
    """
    Add dot to FQDN

    Route53 treats www.example.com (without a trailing dot) and
    www.example.com. (with a trailing dot) as identical.
    """
    if not fqdn.endswith('.'):
        fqdn = fqdn + '.'
    return fqdn


class _RecordAction:
    CREATE = 'CREATE'
    DELETE = 'DELETE'


class Route53(BaseProvider):
    _route53 = None

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        if config.route53.enabled:
            self._route53 = boto3.client(
                'route53',
                aws_access_key_id=config.aws.access_key_id,
                aws_secret_access_key=config.aws.secret_access_key,
                region_name=config.aws.region_name,
                config=Config(
                    retries=self.config.route53.retries,
                ),
            )

    @tracing.trace('List Route53 Records')
    def _get_records(self, host_zone_id, fqdn) -> list[Record]:
        fqdn = _add_dot_to_fqdn(fqdn)
        tracing.set_tag('dns.fqdn', fqdn)
        tracing.set_tag('dns.host_zone_id', host_zone_id)
        response = self._route53.list_resource_record_sets(  # type: ignore
            HostedZoneId=host_zone_id,
            StartRecordName=fqdn,
            MaxItems='300',  # it's not a bug, MaxItems is string
        )
        # https://docs.aws.amazon.com/Route53/latest/APIReference/API_ListResourceRecordSets.html
        # ListResourceRecordSets returns up to 300 resource record sets at a time in ASCII order,
        # beginning at a position specified by the name and type elements.
        #
        # - ListResourceRecordSets returns up-to 300 records,
        # - we starts with `fqdn`
        #
        # so we should get all FQDNs records on one page.
        if response.get('IsTruncated') and response['NextRecordName'] == fqdn:
            raise RuntimeError(f'Got more than one page with records for {fqdn}: {response}')
        ret = []
        for rec in response['ResourceRecordSets']:
            if rec['Name'] != fqdn:
                # We process all `fqdn` records and should stop
                # (ListResourceRecordSets response is sorted by RecordName)
                break
            for resource_record in rec['ResourceRecords']:
                ret.append(
                    Record(
                        address=resource_record['Value'],
                        record_type=rec['Type'],
                    )
                )
        return ret

    def _gen_add_rollback(self, hosted_zone_id: str, fqdn: str, rec: Record) -> Callable:
        return lambda task, safe_revision: self._delete_record(hosted_zone_id, fqdn, rec)

    def _gen_delete_rollback(self, hosted_zone_id: str, fqdn: str, rec: Record) -> Callable:
        return lambda task, safe_revision: self._create_record(hosted_zone_id, fqdn, rec)

    @tracing.trace('Route53 Change Record Sets')
    def _change_resource_record_sets(self, hosted_zone_id: str, changes: list[dict]) -> None:
        response = self._route53.change_resource_record_sets(  # type: ignore
            HostedZoneId=hosted_zone_id,
            ChangeBatch={'Changes': changes},
        )
        self.logger.debug('%r change records response.ChangeInfo: %r', changes, response['ChangeInfo'])

    def _format_record_set_action(self, action: str, fqdn: str, rec: Record) -> dict:
        return {
            'Action': action,
            'ResourceRecordSet': {
                'Name': _add_dot_to_fqdn(fqdn),
                'Type': rec.record_type,
                'TTL': self.config.route53.ttl,
                'ResourceRecords': [
                    {
                        'Value': rec.address,
                    }
                ],
            },
        }

    def _group_records_set_action(self, action: str, fqdn: str, records: list[Record]) -> list[dict]:
        types: dict[str, list[str]] = {}
        for rec in records:
            rec_list = types.get(rec.record_type, [])
            rec_list.append(rec.address)
            types[rec.record_type] = rec_list

        changes = []
        for record_type, address_list in types.items():
            resource_records = []
            for address in address_list:
                resource_records.append({'Value': address})
            changes.append(
                {
                    'Action': action,
                    'ResourceRecordSet': {
                        'Name': _add_dot_to_fqdn(fqdn),
                        'Type': record_type,
                        'TTL': self.config.route53.ttl,
                        'ResourceRecords': resource_records,
                    },
                }
            )

        return changes

    @tracing.trace('Route53 Delete Record')
    def _delete_record(self, hosted_zone_id: str, fqdn: str, rec: Record) -> None:
        tracing.set_tag('dns.fqdn', fqdn)
        tracing.set_tag('dns.record', rec)
        self._change_resource_record_sets(
            hosted_zone_id,
            [self._format_record_set_action(_RecordAction.DELETE, fqdn, rec)],
        )

    @tracing.trace('Route53 Create Record')
    def _create_record(self, hosted_zone_id: str, fqdn: str, rec: Record) -> None:
        tracing.set_tag('dns.fqdn', fqdn)
        tracing.set_tag('dns.record', rec)
        self._change_resource_record_sets(
            hosted_zone_id,
            [self._format_record_set_action(_RecordAction.CREATE, fqdn, rec)],
        )

    def _set_records(self, hosted_zone_id: str, fqdn: str, records: list[Record]) -> None:
        """
        CAS-style update of records for fqdn
        """
        tracing.set_tag('dns.fqdn', fqdn)
        tracing.set_tag('dns.records', records)
        tracing.set_tag('dns.hosted_zone_id', hosted_zone_id)
        self.logger.debug('set %r records %r', fqdn, records)

        existed = set(self._get_records(hosted_zone_id, fqdn))
        to_delete = set(existed) - set(records)
        to_create = set(records) - set(existed)

        if not to_create and not to_delete:
            self.logger.info('everything in sync. We do not need to change any records')
            return

        if to_delete:
            self.logger.info('need delete records: %r', to_delete)
        if to_create:
            self.logger.info('need create records: %r', to_create)
        records_changes = []
        for rec in to_delete:
            self.add_change(
                Change(
                    f'dns.{fqdn}-{rec.record_type}-{rec.address}',
                    'removed',
                    rollback=self._gen_delete_rollback(hosted_zone_id, fqdn, rec),
                )
            )
        records_changes.extend(self._group_records_set_action(_RecordAction.DELETE, fqdn, list(to_delete)))
        for rec in to_create:
            self.add_change(
                Change(
                    f'dns.{fqdn}-{rec.record_type}-{rec.address}',
                    'created',
                    rollback=self._gen_add_rollback(hosted_zone_id, fqdn, rec),
                )
            )
        records_changes.extend(self._group_records_set_action(_RecordAction.CREATE, fqdn, list(to_create)))
        self._change_resource_record_sets(hosted_zone_id, records_changes)

    @tracing.trace('Route53 Set Records In Public Zone')
    def set_records_in_public_zone(self, fqdn: str, records: list[Record]) -> None:
        if self._route53 is None:
            raise Route53DisabledError
        self._set_records(
            hosted_zone_id=self.config.route53.public_hosted_zone_id,
            fqdn=fqdn,
            records=records,
        )

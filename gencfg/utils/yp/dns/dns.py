# coding: utf-8
import collections
import ipaddress
import logging
import time
import sys


def update_ypdns(clt, gencfg_records, source=None):
    """
    source: something traceable, e.g. sbr:501593700
    """
    forward, reverse = read(clt)
    update_batch = []
    create_batch = []

    # labels = {'gencfg': {'timestamp': int(time.time()),
    #                      'source': source}}
    for record in gencfg_records:
        try:
            fqdn, ip6 = record
            reverse_pointer = ipaddress.ip_address(unicode(ip6)).reverse_pointer
        except ValueError:  # gencfg contains data errors of any kind, even broken ip addresses;
            continue        # bypass them

        if fqdn in forward:
            if ip6 not in forward[fqdn]:
                update_batch.append((fqdn, ip6))
        else:
            create_batch.append((fqdn, ip6))

        if reverse_pointer in reverse:
            if fqdn not in reverse[reverse_pointer]:
                update_batch.append()

        else:
            create_batch.append((fqdn, reverse_pointer))


def read(clt):
    record_sets = read_gencfg_record_sets(clt, 5000)
    forward = collections.defaultdict(set)
    reverse = collections.defaultdict(set)
    for record_set in record_sets:
        dns_id, dns_records = record_set
        if not dns_records:
            _log.error('Empty dns_record_set: %s', record_set)
            continue

        # dns_type = dns_records[0]['type']
        for record in dns_records:
            if record['type'] == 'PTR':
                reverse[dns_id].add(record['data'])
            elif record['type'] == 'AAAA':
                forward[dns_id].add(record['data'])
            else:
                _log.error('Unknown dns record type: %s', record_set)

    return forward, reverse


# batch_size = 10000 leads to GRPC error: StatusCode.RESOURCE_EXHAUSTED
# todo: use transaction instead of blind retries
def read_gencfg_record_sets(clt, batch_size=5000, batch_count=None):
    record_sets = []
    last_id = None
    for batch_number in xrange(batch_count or sys.maxint):
        _log.debug('Read batch %s from [%s]', batch_number, last_id)
        try:
            batch = clt.select_objects('dns_record_set',
                                       selectors=['/meta/id', '/spec/records'],
                                       filter='[/meta/id] > "{}" and not is_null([/labels/gencfg])'.format(last_id) if last_id else None,
                                       limit=batch_size)
        except Exception:
            _log.exception('Retrying batch {} from [{}]'.format(batch_number, last_id))
        else:
            if not batch:
                break
            record_sets += batch
            last_id = batch[-1][0]

    _log.info('Successfully read %s dns_record_sets', len(record_sets))
    return record_sets


def upload(clt, gencfg_data, existing_ids, batch_size=2000, source='501593700'):
    """source: traceable source, e.g. 'sbr:501593700'"""
    labels = {'gencfg': {'timestamp': int(time.time()),
                         'source': source}}
    batch = []
    for record in gencfg_data:
        try:
            host = record['host']
            ip6 = record['ip6']
            reverse_pointer = ipaddress.ip_address(unicode(ip6)).reverse_pointer
        except ValueError:  # gencfg contains data errors of any kind, even broken ip addresses;
            continue        # bypass them

        if host in existing_ids or reverse_pointer in existing_ids:
            continue

        existing_ids.add(host)
        existing_ids.add(reverse_pointer)

        batch.append(('dns_record_set', aaaa(host, ip6, labels)))
        batch.append(('dns_record_set', ptr(host, reverse_pointer, labels)))

        if len(batch) > batch_size:
            clt.create_objects(batch)
            print batch[-1]
            batch = []

    if batch:
        clt.create_objects(batch)
        print batch[-1]


def aaaa(fqdn, ip6, labels):
    return {'meta': {'id': fqdn},
            'spec': {'records': [{'data': ip6, 'type': 'AAAA', 'class': 'IN'}]},
            'labels': {'gencfg': labels}}


def ptr(fqdn, reverse_pointer, labels):
    return {'meta': {'id': reverse_pointer},
            'spec': {'records': [{'data': fqdn, 'type': 'PTR', 'class': 'IN'}]},
            'labels': {'gencfg': labels}}


# trash, remove
def read_all(clt, batch_size=10000):
    """
    reads ids only
    """
    record_sets = []
    last_id = None
    while True:
        try:
            batch = clt.select_objects('dns_record_set',
                                       selectors=['/meta/id', '/labels/gencfg'],
                                       filter='[/meta/id] > "{}" and not is_null([/labels/gencfg])'.format(last_id) if last_id else None,
                                       limit=batch_size)
        except Exception as e:
            print e
            continue
        else:
            if not batch:
                break
            record_sets += batch
            last_id = batch[-1][0]

    return record_sets


_log = logging.getLogger(__name__)

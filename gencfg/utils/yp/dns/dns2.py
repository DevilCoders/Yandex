# coding: utf-8
import collections
import ipaddress
import logging
import time
import sys


def update_objs(clt, kind, update_dict, batch_size):
    batch = []
    for key, value in update_dict.iteritems():
        if kind == 'aaaa':
            batch.append({'object_type': 'dns_record_set', 'object_id': key, 'set_updates': aaaa_append(value)})
        else:
            batch.append({'object_type': 'dns_record_set', 'object_id': key, 'set_updates': ptr_append(value)})

        if len(batch) > batch_size:
            try:
                clt.update_objects(batch)  # todo: retry single batch
            except Exception:
                logging.exception('unhandled')

            print batch[-1]
            batch = []

    if batch:
        clt.update_objects(batch)
        print batch[-1]


def create_objs(clt, kind, create_dict, batch_size):
    """
    todo: source parameter: something traceable, e.g. sbr:501593700
    """
    labels = {'gencfg': {'timestamp': int(time.time()),
                         'source': '6004728'}}
    batch = []
    for key, value in create_dict.iteritems():
        if kind == 'aaaa':
            batch.append(('dns_record_set', aaaa(key, value, labels)))
        else:
            batch.append(('dns_record_set', ptr(key, value, labels)))

        if len(batch) > batch_size:
            clt.create_objects(batch)  # todo: retry
            print batch[-1]
            batch = []

    if batch:
        clt.create_objects(batch)
        print batch[-1]


def calc_ypdns_update(forward, reverse, gencfg_records):
    update_aaaa = collections.defaultdict(set)
    create_aaaa = collections.defaultdict(set)
    update_ptr = collections.defaultdict(set)
    create_ptr = collections.defaultdict(set)

    for record in gencfg_records:
        try:
            fqdn, ip6 = record
            reverse_pointer = ipaddress.ip_address(unicode(ip6)).reverse_pointer
        except ValueError:  # gencfg contains data errors of any kind, even broken ip addresses;
            continue        # bypass them

        if fqdn in forward:
            if ip6 not in forward[fqdn]:
                update_aaaa[fqdn].add(ip6)
        else:
            create_aaaa[fqdn].add(ip6)

        if reverse_pointer in reverse:
            if fqdn not in reverse[reverse_pointer]:
                update_ptr[reverse_pointer].add(fqdn)
        else:
            create_ptr[reverse_pointer].add(fqdn)

    return update_aaaa, create_aaaa, update_ptr, create_ptr


def read(clt):
    record_sets = read_gencfg_record_sets(clt, 10000)
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
# todo: use transaction(s) instead of blind retries
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


def aaaa_append(ip6s):
    return [{'path': '/spec/records/end', 'value': {'data': ip6, 'type': 'AAAA', 'class': 'IN'}} for ip6 in ip6s]


def aaaa(fqdn, ip6s, labels):
    return {'meta': {'id': fqdn},
            'spec': {'records': [{'data': ip6, 'type': 'AAAA', 'class': 'IN'} for ip6 in ip6s]},
            'labels': {'gencfg': labels}}


def ptr(reverse_pointer, fqdns, labels):
    return {'meta': {'id': reverse_pointer},
            'spec': {'records': [{'data': fqdn, 'type': 'PTR', 'class': 'IN'} for fqdn in fqdns]},
            'labels': {'gencfg': labels}}


def ptr_append(fqdns):
    return [{'path': '/spec/records/end', 'value': {'data': fqdn, 'type': 'PTR', 'class': 'IN'}} for fqdn in fqdns]


_log = logging.getLogger(__name__)

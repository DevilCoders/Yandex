#!/usr/bin/python -u
# -*- coding: utf-8 -*-

import sys
import socket
import json
import uuid
import ipaddress
import logging

fields = [
    'tskv_format', 'timestamp', 'x_s3_folder_id', 'x_s3_storage_class',
    'x_s3_handler', 'x_s3_bucket', 'x_s3_bucket_tags', 'ip', 'method', 'bytes_sent',
    'bytes_received', 'unixtime', 'status'
]

# logging.basicConfig(filename='/var/log/statbox/debug.log', level=logging.DEBUG)
logging.basicConfig(filename='/dev/null', level=logging.DEBUG)
logger = logging.getLogger()
logger.setLevel(logging.DEBUG)


def tskv_parse(logs):
    row = dict()
    for token in logs[5:].strip().split("\t"):
        name, value = token.split("=", 1)
        if name in fields:
            if name == 'x_s3_bucket_tags' and 'yc_registry_payer' in value:
                try:
                    row.update(json.loads(value.replace('\\', '')))
                except Exception:
                    pass
            else:
                row.update(dict([token.split("=", 1)]))
    return row


def resolve_cloud_net(ip):
    net_type = 'inet'
    # MDS-6028: todo dynamic resolve via netbox
    # MDS-7529: extend ipv6 rules for internal traffic detecting
    if ipaddress.ip_address(ip).version == 4:
        if ipaddress.ip_address(ip) in ipaddress.ip_network(u'198.18.0.0/15'):
            net_type = 'cloud'
    elif ipaddress.ip_address(ip).version == 6:
        if ipaddress.ip_address(ip) in ipaddress.ip_network(u'2a02:6b8:bf00::/40') or \
           ipaddress.ip_address(ip) in ipaddress.ip_network(u'2a02:6b8:c01:900::/56') or \
           ipaddress.ip_address(ip) in ipaddress.ip_network(u'2a02:6b8:c02:900::/56') or \
           ipaddress.ip_address(ip) in ipaddress.ip_network(u'2a02:6b8:c03:500::/56') or \
           ipaddress.ip_address(ip) in ipaddress.ip_network(u'2a02:6b8:c0e:500::/56') or \
           ipaddress.ip_address(ip) in ipaddress.ip_network(u'2a0d:d6c0::/29'):
            net_type = 'cloud'
    return net_type


def main(argv):
    try:
        while True:
            line = sys.stdin.readline()

            if line == '':
                break
            else:
                magic, tskv_line = line.split('tskv\t', 1)
                record = tskv_parse(tskv_line)

            if record.get('method') == 'GET':
                transferred = int(record.get('bytes_received')) + int(
                    record.get('bytes_sent'))
            else:
                transferred = record.get('bytes_received')

            # https://st.yandex-team.ru/CLOUD-10123#1535996237000
            BILLING_SCHEMA = {
                "folder_id": record.get('x_s3_folder_id'),
                "id": str(uuid.uuid4()),
                "schema": "s3.api.v1",
                "source_id": socket.gethostname(),
                "source_wt": int(record.get('unixtime')),
                "tags": {
                    "method": record.get('method'),
                    "status_code": record.get('status'),
                    "handler": record.get('x_s3_handler'),
                    "storage_class": record.get('x_s3_storage_class'),
                    "net_type": resolve_cloud_net(unicode(record.get('ip'))),
                    "transferred": int(transferred),
                },
                "usage": {
                    "start": int(record.get('unixtime')),
                    "finish": int(record.get('unixtime')),
                    "quantity": 1,
                    "type": "delta",
                    "unit": "requests"
                },
                "resource_id": record.get('x_s3_bucket')
            }

            if record.get('x_s3_folder_id', '-') == '-':
                pass
            else:
                json_data = json.dumps(BILLING_SCHEMA)
                print("%s%s" % (magic, json_data))
                # logger.info("%s%s",magic,json_data)
                if record.get('yc_registry_payer', False):
                    BILLING_SCHEMA["folder_id"] = record.get('yc_registry_payer')
                    BILLING_SCHEMA["schema"] = 'cr.api.v1'
                    json_data = json.dumps(BILLING_SCHEMA)
                    print("%s%s" % (magic, json_data))

    except Exception:
        logger.exception("CRIT ERROR:")


if (__name__ == "__main__"):
    sys.exit(main(sys.argv))

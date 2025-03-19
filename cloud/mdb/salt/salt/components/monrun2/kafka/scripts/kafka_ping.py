#!/usr/bin/python

import os
import sys
from confluent_kafka.admin import AdminClient

TIMEOUT = 10

def die(status=0, message='OK'):
    """
    Print status in monrun-compatible format
    """
    print('{status};{message}'.format(status=status, message=message))
    sys.exit(0)


def get_client():
    fqdn = "{{ salt.grains.get('fqdn') }}"
    params = {
        'bootstrap.servers': fqdn + ':9091',
        'request.timeout.ms': TIMEOUT * 1000,
        'security.protocol': 'SASL_SSL',
        'ssl.ca.location': '/etc/kafka/ssl/cert-ca.pem',
        'sasl.mechanism': 'SCRAM-SHA-512',
        'sasl.username': 'mdb_admin',
        'sasl.password': "{{ salt.pillar.get('data:kafka:admin_password') }}",
    }

    # Set from pillar
    if {{ salt.dbaas.is_aws() }}:
        params['broker.address.family'] = 'v6'

    return AdminClient(params)

def _main():
    try:
        client = get_client()
    except Exception:
        die(1, "Failed to connect to Kafka broker")
    try:
        client.list_topics(timeout=TIMEOUT)
    except Exception:
        die(2, "Failed to get Kafka metadata")
    die()


if __name__ == '__main__':
    _main()

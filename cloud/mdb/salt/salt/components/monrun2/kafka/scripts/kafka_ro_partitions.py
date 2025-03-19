#!/usr/bin/python3

import sys
import requests

METRIC_NAME = 'kafka_server_ReplicaManager_UnderMinIsrPartitionCount'

def die(status=0, message='OK'):
    """
    Print status in monrun-compatible format
    """
    print('{status};{message}'.format(status=status, message=message))
    sys.exit(0)


def has_ro_partitions():
    resp = requests.get("http://localhost:7071/metrics")
    resp.raise_for_status()
    for line in resp.text.split('\n'):
        if not line.startswith(METRIC_NAME):
            continue
        words = line.split()
        if words[1] == '0.0':
            return False
    return True


def _main():
    has_ro = False
    try:
        has_ro = has_ro_partitions()
    except Exception:
        die(1, "Failed to get Kafka metrics")
    if has_ro:
        die(2, "Kafka has RO partitions. Perhaps some brokers a offline")
    die()


if __name__ == '__main__':
    _main()

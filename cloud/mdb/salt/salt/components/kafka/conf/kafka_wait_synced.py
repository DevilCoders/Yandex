#!/usr/bin/env python{{ salt.pillar.get('data:salt_py_version', 2) }}
"""
This script is useful when we make some cluster modification operation that requires
relatively long broker downtime, eg. nbs disk shrink or any resource change on vm with local-ssd.

Consider following example. We have topic with name topic1, partitions=6, replication_factor=2.
And we have producer that is actively publishes messages to topic1.
Before update we have following distribution of partition replicas:

/opt/kafka/bin/kafka-topics.sh --describe --topic topic1
Topic: topic1   PartitionCount: 6       ReplicationFactor: 2    Configs: compression.type=snappy,min.insync.replicas=1
        Topic: topic1   Partition: 0    Leader: 1       Replicas: 1,2   Isr: 1,2
        Topic: topic1   Partition: 1    Leader: 2       Replicas: 2,3   Isr: 2,3
        Topic: topic1   Partition: 2    Leader: 3       Replicas: 3,1   Isr: 1,3
        Topic: topic1   Partition: 3    Leader: 1       Replicas: 1,3   Isr: 1,3
        Topic: topic1   Partition: 4    Leader: 2       Replicas: 2,1   Isr: 1,2
        Topic: topic1   Partition: 5    Leader: 3       Replicas: 3,2   Isr: 2,3

Now we start cluster modification operation and broker #1 is shut down for some time. Now distribution is as follows:

Topic: topic1   PartitionCount: 6       ReplicationFactor: 2    Configs: compression.type=snappy,min.insync.replicas=1
        Topic: topic1   Partition: 0    Leader: 2       Replicas: 1,2   Isr: 2
        Topic: topic1   Partition: 1    Leader: 2       Replicas: 2,3   Isr: 2,3
        Topic: topic1   Partition: 2    Leader: 3       Replicas: 3,1   Isr: 3
        Topic: topic1   Partition: 3    Leader: 3       Replicas: 1,3   Isr: 3
        Topic: topic1   Partition: 4    Leader: 2       Replicas: 2,1   Isr: 2
        Topic: topic1   Partition: 5    Leader: 3       Replicas: 3,2   Isr: 2,3

After update of broker #1 is finished we start updating broker #2.
But partition #0 has up-to-date copy on broker #2 only. So if we turn off broker #2 for maintenance right away
we will end up with partition #0 unavailable for write operations.

That is why we have to wait until replicas that are hosted on broker #1 will catch up with their partition leaders.
"""

import argparse
import sys
import time

import salt.client
from confluent_kafka import Producer


API_TIMEOUT = 60 * 1000


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-w', '--wait', type=int, default=60, help='Time to wait')
    args = parser.parse_args()

    producer = create_producer()
    stop_time = time.time() + args.wait
    while time.time() < stop_time:
        if synced(producer):
            return
        time.sleep(1)
    print('Timed out waiting for data to be properly replicated')
    sys.exit(1)


def create_producer():
    salt_caller = salt.client.Caller()
    password = salt_caller.cmd('pillar.get', 'data:kafka:admin_password')
    servers = salt_caller.cmd('pillar.get', 'data:kafka:nodes').keys()
    servers = ','.join((s + ':9091' for s in servers))
    params = {
        'bootstrap.servers': servers,
        'request.timeout.ms': API_TIMEOUT,
        'security.protocol': 'SASL_SSL',
        'ssl.ca.location': '/etc/kafka/ssl/cert-ca.pem',
        'sasl.mechanism': 'SCRAM-SHA-512',
        'sasl.username': 'mdb_admin',
        'sasl.password': password,
    }
    if salt_caller.cmd('dbaas.is_aws'):
        params['broker.address.family'] = 'v6'
    return Producer(params)


def synced(producer):
    topics = producer.list_topics(timeout=10).topics
    for topic_name, topic_metadata in topics.items():
        for partition_id, partition_metadata in topic_metadata.partitions.items():
            if len(partition_metadata.replicas) == 1:
                continue
            # Currently we just wait until all replicas become in-sync.
            # In the future we may want to apply some more advanced check here, eg. ensure that
            # len(partition_metadata.isrs) > topic_config['min.insync.replicas']
            if len(partition_metadata.isrs) < len(partition_metadata.replicas):
                print("Number of ISRs for partition {} of topic {} equals {} and is lower than replication factor {}. "
                      "Waiting for all replicas to catch up."
                      .format(partition_id, topic_name, len(partition_metadata.isrs), len(partition_metadata.replicas)))
                return False
    return True


if __name__ == '__main__':
    main()

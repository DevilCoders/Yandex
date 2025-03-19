import argparse
import logging
import socket
import sys


from elasticsearch import Elasticsearch


# Jinja here is used to avoid exposing passwords in command line
USER = "{{ salt.pillar.get('data:elasticsearch:users:mdb_monitor:name') }}"
PASSWORD = "{{ salt.pillar.get('data:elasticsearch:users:mdb_monitor:password') }}"

SELF_FQDN = socket.gethostname()

logging.getLogger('elasticsearch').setLevel(logging.FATAL)


def parse_args():
    """
    Parse arguments from command line
    """
    args_parser = argparse.ArgumentParser()
    args_parser.add_argument(
        '-n', '--number',
        type=int,
        default=10,
        help='The max number of retries')
    args_parser.add_argument(
        '-u', '--user',
        type=str,
        default=USER,
        help='Username')
    args_parser.add_argument(
        '-p', '--password',
        type=str,
        default=PASSWORD,
        help='Password')

    return args_parser.parse_args()


def get_client(user, password):
    myself = {
        'host': SELF_FQDN,
        'port': 9200,
    }
    return Elasticsearch(
        [myself],
        use_ssl=True,
        verify_certs=True,
        http_auth=(user, password),
        ca_certs='/etc/ssl/certs/ca-certificates.crt')


def die(status, message):
    """
    Emit status and exit.
    """
    print('%s;%s' % (status, message))
    sys.exit(0)

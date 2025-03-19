"""
MDB DNS updater for Elasticsearch

Update primary host fqdn in cluster in mdb-dns-server
"""
import json

from tenacity import retry, stop_after_attempt, stop_after_delay
from mdbdns_upper import MDBDNSUpper, init_logger
import elasticsearch as es


UPPER = None
# Jinja here is used to avoid exposing passwords in command line
USER = "{{ salt.pillar.get('data:elasticsearch:users:mdb_admin:name') }}"
PASSWORD = "{{ salt.pillar.get('data:elasticsearch:users:mdb_admin:password') }}"
DNS_INDEX = '.mdb_dns_upper'
RETRY_DELAY = 9
RETRY_ATTEMPTS = 10
MY_STATE_PATH = '/tmp/.elasticsearch_state.cache'

# ES client does not skip faulty hosts; instead it returns an error immediately.
# However the hosts order is random, so given enough retries we may end up with a legit topology.
@retry(reraise=True, stop=(stop_after_attempt(RETRY_ATTEMPTS)|stop_after_delay(RETRY_DELAY)))
def _get_elasticsearch_master(hosts):

    def get_index(index_name):
        return client.indices.get(index_name, params={'ignore_unavailable': 'true'})

    def create_index(index_name):
        client.indices.create(
            index_name,
            body={
                'settings': {
                    'index': {
                        'number_of_shards': 1,
                        'auto_expand_replicas': '0-1'
                    }
                }
            },
            params={
                'wait_for_active_shards': 'all',
                'timeout': '60s'
            })

    client = es.Elasticsearch(
        hosts,
        use_ssl=True,
        verify_certs=True,
        timeout=5,
        http_auth=(USER, PASSWORD),
        ca_certs='/etc/ssl/certs/ca-certificates.crt')

    if not get_index(DNS_INDEX):
        create_index(DNS_INDEX)

    # 1. Get a mapping between UUID of host and a hostname
    host_uuids = {}
    state = client.cluster.state(metric='nodes,routing_table', index=DNS_INDEX)
    for uuid, props in state.get('nodes', {}).items():
        host_uuids[uuid] = props['name']

    # 2. Using that mapping resolve index allocation
    #  'routing_table': {'indices': {'.kibana_1': {'shards': {'0': [{'allocation_id': {'id': 't1eHHITQT8iGEpBYVNLFSw'},
    #      ...
    #      'primary': True,
    #      ...
    #      'state': 'STARTED'},
    #     {'allocation_id': {'id': 'UAA8_w6IQD2qK6zLSL26SQ'},
    #      ...
    #      'state': 'STARTED'}]}}}},

    for allocation in state['routing_table']['indices'][DNS_INDEX]['shards']['0']:
        if allocation['primary'] and allocation['state'] == 'STARTED':
            return host_uuids[allocation['node']]

    raise RuntimeError(f'no masters for index "{DNS_INDEX}" were detected')


class MDBDNSUpperElasticsearch(MDBDNSUpper):

    def get_state(self):
        conf = self.get_dbaas_conf()

        master_fqdn = _get_elasticsearch_master([conf.get('fqdn')])
        state = {
            'role': 'master' if master_fqdn == self._fqdn else 'replica'
        }
        with open(MY_STATE_PATH, 'w') as fobj:
            json.dump(state, fobj)

        return state

    def check_and_up(self):
        self._logger.debug('checking...')
        dbc = self.get_dbaas_conf()

        cid = dbc.get('cluster_id')
        if not cid or cid != self._cid:
            self._logger.error('invalid cid in dbaas conf')
            return

        state = self.get_state()
        if state.get('role') != 'master':
            self._logger.info('not a master host')
            return

        self.update_dns()


def mdbdns_elasticsearch(log_file, rotate_size, params, backup_count=1):
    """
    Run mdb DNS update
    """
    global UPPER
    if not UPPER:
        log = init_logger(__name__, log_file, rotate_size, backup_count)
        log.info('Initialization MDB DNS upper')
        try:
            UPPER = MDBDNSUpperElasticsearch(log, params)
        except Exception as exc:
            log.error('failed to init MDB DNS upper failed by exception: %s', repr(exc))
            raise exc

    try:
        UPPER.check_and_up()
    except Exception as exc:
        UPPER._logger.error('mdbdns upper failed by exception: %s', repr(exc))
        raise exc

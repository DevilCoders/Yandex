"""
MDB DNS updater for MySQL

Update primary host fqdn in cluster in mdb-dns-server
"""
import datetime
import json
import operator
import os
import socket

import dateutil.parser

from mdbdns_upper import MDBDNSUpper, init_logger

UPPER = None
MYCNF_PATH = os.path.expanduser('~monitor/.my.cnf')
MYSYNC_INFO_PATH = '/var/run/mysync/mysync.info'
HEALTH_EXPIRATION = datetime.timedelta(seconds=20)


class MDBDNSUpperMySQL(MDBDNSUpper):

    def __init__(self, log, params):
        super().__init__(log, params)
        self._fqdn = socket.getfqdn()

    def _get_mysync_info(self):
        with open(MYSYNC_INFO_PATH, 'r') as f:
            return json.loads(f.read())

    def get_state(self):
        """
        Get last fresh MySQL state
        """
        result = {
            'role': 'unknown',
        }
        info = self._get_mysync_info()
        now = datetime.datetime.now(datetime.timezone.utc)

        if info.get('master') != self._fqdn:
            result['role'] = 'replica'
        else:
            result['role'] = 'master'
            result['replicas'] = []
            for host, health in info.get('health', {}).items():
                check_at = health.get('check_at')
                if check_at is None or now - dateutil.parser.parse(check_at) > HEALTH_EXPIRATION:
                    # health status is stale
                    continue
                sstate = health.get('slave_state')
                if sstate is None or sstate.get('master_host') != self._fqdn:
                    # all cascade replicas are more laggy than direct ones
                    continue
                lag_seconds = int(sstate.get('replication_lag', 100500))
                lag_seconds = (0 if lag_seconds < 5 else -lag_seconds)
                result['replicas'].append({
                    'fqdn': host,
                    'lag': lag_seconds,
                })
        return result

    def choice_best_secondary(self, state):
        if not ('replicas' in state and state['replicas']):
            return
        self._logger.info('My state is {state}'.format(state=state))
        replica = sorted(state['replicas'], key=operator.itemgetter('lag', 'fqdn'), reverse=True)
        self._logger.info('Best replica is "{replica}"'.format(replica=replica[0]['fqdn']))
        return replica[0]['fqdn']


def mdbdns_mysql(log_file, rotate_size, params, backup_count=1):
    """
    Run mdb DNS update
    """
    global UPPER
    if not UPPER:
        log = init_logger(__name__, log_file, rotate_size, backup_count)
        log.info("Initialization MDB DNS upper")
        try:
            UPPER = MDBDNSUpperMySQL(log, params)
        except Exception as exc:
            log.error("failed to init MDB DNS upper failed by exception: %s", repr(exc))
            raise exc

    try:
        UPPER.check_and_up()
    except Exception as exc:
        UPPER._logger.error("mdbdns upper failed by exception: %s", repr(exc))
        raise exc

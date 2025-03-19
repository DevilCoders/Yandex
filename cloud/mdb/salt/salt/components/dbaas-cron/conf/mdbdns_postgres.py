"""
MDB DNS updater for PostgreSQL

Update primary host fqdn in cluster in mdb-dns-server
"""
import json
import os
import time
from mdbdns_upper import MDBDNSUpper, init_logger


UPPER = None
PG_STATE_PATH = '/tmp/.pgsync_db_state.cache'
DBAAS_CONF    = '/etc/dbaas.conf'
STATUS_TIMEFRESH_SEC = 15
UNSIGNIFICANT_BITS = 12
RELOAD_COUNT = 3
RELOAD_TIMEOUT = 1


class MDBDNSUpperPostgreSQL(MDBDNSUpper):
    def _get_dbaas_conf(self):
        """
        Get actual common cluster configuration
        """
        state = {}
        with open(DBAAS_CONF) as fobj:
            try:
                state = json.load(fobj)
            except Exception as e:
                self._logger.warning('failed to load dbaas.conf file due exception: {}'.format(e))

        return state

    def _parse_ha_hosts_from_state(self, state):
        return state.get('cluster_nodes', {}).get('ha', [])

    def get_state(self):
        """
        Get last fresh pg state
        """
        path = PG_STATE_PATH
        try:
            for i in range(RELOAD_COUNT):
                mtime = os.stat(path).st_mtime
                if time.time() - mtime > STATUS_TIMEFRESH_SEC:
                    self._logger.error('too old pg state file "%s"', path)
                    return dict()
                with open(path) as fobj:
                    try:
                        state = json.load(fobj)
                        return state
                    except ValueError:
                        self._logger.warning('failed to parse json, try reload after bit delay: %s', path)
                        time.sleep(RELOAD_TIMEOUT)
                        continue
            self._logger.error('failed to read pg state file %s, reload %d times', path, RELOAD_COUNT)
            return dict()
        except Exception as e:
            self._logger.error('failed to load pg state file "%s", error: %s', path, repr(e))
            raise e

    def choice_best_secondary(self, pgstate):
        ri = pgstate["replics_info"]
        if not ri:
            self._logger.warn('no replics info')

            # MDB-8431 : turn RO cname to master in 2 hosts cluster configuration
            state = self._get_dbaas_conf()
            ha_hosts = self._parse_ha_hosts_from_state(state)
            if len(ha_hosts) == 2:
                # we have 2 ha hosts in our cluster and one replica is down
                # and we are on master - so just return local fqdn as best secondary
                return state['fqdn']
            return ""
        def sortKey(item):
            ld = int(float(item["replay_lag_msec"])) >> UNSIGNIFICANT_BITS
            is_async = item["sync_state"] == "async"
            if is_async:
                ld += 1
            # If client_hostname is None (null) use empty string for comparison
            hostname = item["client_hostname"] or ''
            return (-ld, hostname)

        try:
            return sorted(ri, key=sortKey, reverse=True)[0]["client_hostname"]
        except Exception as exc:
            self._logger.warn('exception on choice best of replics info hosts: %s', repr(exc))
            raise exc


def mdbdns_postgres(log_file, rotate_size, params, backup_count=1):
    """
    Run mdb DNS update
    """
    global UPPER
    if not UPPER:
        log = init_logger(__name__, log_file, rotate_size, backup_count)
        log.info("Initialization MDB DNS upper")
        try:
            UPPER = MDBDNSUpperPostgreSQL(log, params)
        except Exception as exc:
            log.error("failed to init MDB DNS upper failed by exception: %s", repr(exc))
            raise exc

    try:
        UPPER.check_and_up()
    except Exception as exc:
        UPPER._logger.error("mdbdns upper failed by exception: %s", repr(exc))
        raise exc

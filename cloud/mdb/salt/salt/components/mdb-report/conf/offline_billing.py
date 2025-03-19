"""
DBaaS Offline Instance Billing reporter
"""

import calendar
import json
import os
import socket
import time
import traceback
import uuid
from contextlib import closing, suppress
from copy import deepcopy
from logging import DEBUG, Formatter, getLogger
from logging.handlers import RotatingFileHandler

from kazoo.client import KazooClient, KazooState
from psycopg2 import connect
from psycopg2.extras import RealDictCursor

OFFLINE_QUERY = """
SELECT
    cl.cloud_ext_id as cloud_id,
    f.folder_ext_id as folder_id,
    c.cid as cluster_id,
    c.type as cluster_type,
    sc.roles::text[] as roles,
    h.fqdn as fqdn,
    h.assign_public_ip as assign_public_ip,
    h.space_limit as disk_size,
    h.vtype_id as compute_instance_id,
    dt.disk_type_ext_id as disk_type_id,
    fl.name as resource_preset_id,
    fl.platform_id,
    fl.cpu_limit::int AS cores,
    (fl.cpu_guarantee * 100 / fl.cpu_limit)::int AS core_fraction,
    fl.memory_limit AS memory,
    fl.io_cores_limit,
    coalesce(cardinality(c.host_group_ids), 0) > 0 AS on_dedicated_host,
    CASE
        WHEN c.type = 'sqlserver_cluster'
        THEN (
            SELECT value#> '{data,sqlserver,version,edition}'
              FROM dbaas.pillar p
             WHERE p.cid = c.cid)
        ELSE NULL
    END AS edition,
    extract(epoch from max(q.end_ts)) as stop_time
FROM
    dbaas.clusters c
    JOIN dbaas.subclusters sc ON (sc.cid = c.cid)
    JOIN dbaas.hosts h ON (sc.subcid = h.subcid)
    JOIN dbaas.flavors fl ON (h.flavor = fl.id)
    JOIN dbaas.disk_type dt ON (dt.disk_type_id = h.disk_type_id)
    JOIN dbaas.folders f ON (c.folder_id = f.folder_id)
    JOIN dbaas.clouds cl ON (f.cloud_id = cl.cloud_id)
    JOIN dbaas.worker_queue q ON (q.cid = c.cid)
WHERE
    c.status IN ('STOPPED', 'RESTORING-OFFLINE', 'RESTORE-OFFLINE-ERROR', 'MAINTAINING-OFFLINE', 'MAINTAIN-OFFLINE-ERROR')
    AND c.type IN %s
    AND c.host_group_ids IS NULL -- exclude clusters on dedicated hosts
    AND code.managed(c)
    AND q.task_type ~ '.*cluster_stop$'
GROUP BY
    h.fqdn,
    cl.cloud_ext_id,
    f.folder_ext_id,
    c.cid,
    c.type,
    sc.roles,
    h.assign_public_ip,
    h.space_limit,
    dt.disk_type_ext_id,
    fl.name,
    fl.platform_id,
    fl.cpu_limit,
    fl.cpu_guarantee * 100 / fl.cpu_limit,
    fl.memory_limit,
    fl.io_cores_limit
ORDER BY
    cluster_id,
    roles,
    fqdn
"""

ONLINE_QUERY = """
SELECT
    extract(epoch from start_ts) as timestamp
FROM
    dbaas.worker_queue
WHERE
    cid = %(cid)s
    AND task_type = %(task_type)s
    AND start_ts > to_timestamp(%(timestamp)s)
ORDER BY
    timestamp
"""


class BillingFormatter(Formatter):
    """
    Billing record formatter
    """

    def format(self, record):
        """
        Format record to billing-specific format
        """
        fields = list(vars(record))
        data = {name: getattr(record, name, None) for name in fields}

        usage = {
            'type': 'delta',
            'quantity': data['ts'] - data['last_ts'],
            'unit': 'seconds',
            'start': data['last_ts'],
            'finish': data['ts'],
        }

        tags = dict(
            resource_preset_id=data['params']['resource_preset_id'],
            platform_id=data['params']['platform_id'],
            cores=data['params']['cores'],
            core_fraction=data['params']['core_fraction'],
            memory=data['params']['memory'],
            software_accelerated_network_cores=data['params']['io_cores_limit'],
            cluster_id=data['params']['cluster_id'],
            cluster_type=data['params']['cluster_type'],
            disk_type_id=data['params']['disk_type_id'],
            disk_size=data['params']['disk_size'],
            public_ip=1 if data['params']['assign_public_ip'] else 0,
            roles=data['params']['roles'],
            compute_instance_id=data['params']['compute_instance_id'],
            online=0,
            on_dedicated_host=1 if data['params']['on_dedicated_host'] else 0,
        )
        if data.get('edition'):
            tags['edition'] = data.get('edition')

        ret = dict(
            schema='mdb.db.generic.v1',
            source_wt=data['ts'],
            source_id=data['params']['fqdn'],
            resource_id=data['params']['fqdn'],
            folder_id=data['params']['folder_id'],
            cloud_id=data['params']['cloud_id'],
            usage=usage,
            tags=tags,
            id=str(uuid.uuid4()),
        )
        return json.dumps(ret)


class BillingReporter:
    """
    Offline billing reporter
    """

    def __init__(self):
        self.config = None
        self.billing_log = None
        self.log = None
        self.pg_conn = None
        self.zk_conn = None
        self.lock = None
        self._fqdn = socket.getfqdn()
        self._initialized = False

    def pg_connect(self):
        """
        (Re-)init connection with metadb
        """
        if self.pg_conn is not None:
            with suppress(Exception):
                self.pg_conn.close()
        self.pg_conn = connect(self.config['conn_string'], cursor_factory=RealDictCursor)

    def _zk_listener(self, state):
        if state == KazooState.LOST:
            self.log.error('Connection to ZK lost')
            self.lock = None
        if state == KazooState.SUSPENDED:
            self.log.warning('Being disconnected from ZK')
        if state == KazooState.CONNECTED:
            self.log.info('Connected to ZK')

    def zk_connect(self):
        """
        (Re-)init connection with zk
        """
        if self.zk_conn is not None:
            with suppress(Exception):
                self.lock.cancel()
            with suppress(Exception):
                self.lock.release()
            with suppress(Exception):
                self.zk_conn.stop()
        self.zk_conn = KazooClient(
            self.config['zk_hosts'],
            connection_retry=self.config['zk_retries'],
            command_retry=self.config['zk_retries'],
            timeout=self.config['zk_timeout'])
        self.zk_conn.add_listener(self._zk_listener)
        self.zk_conn.start()
        self.lock = self.zk_conn.Lock(self.config['zk_lock_path'], self._fqdn)
        self.zk_conn.ensure_path(self.config['zk_state_path'])
        self.lock.acquire(blocking=False)

    def initialize(self, config):
        """
        Init loggers and required connections
        """
        if not self._initialized:
            self.config = config
            self.billing_log = getLogger(__name__ + '.billing')
            self.billing_log.setLevel(DEBUG)
            handler = RotatingFileHandler(
                self.config['log_file'], maxBytes=self.config['rotate_size'], backupCount=self.config['backup_count'])
            formatter = BillingFormatter()
            handler.setFormatter(formatter)
            self.billing_log.addHandler(handler)
            self.log = getLogger(__name__ + '.service')
            self.log.setLevel(DEBUG)
            service_handler = RotatingFileHandler(
                self.config['service_log_file'],
                maxBytes=self.config['rotate_size'],
                backupCount=self.config['backup_count'])
            service_handler.setFormatter(Formatter(fmt='%(asctime)s [%(levelname)s]: %(message)s'))
            self.log.addHandler(service_handler)
        if not self.pg_conn:
            self.pg_connect()
        if not self.zk_conn:
            self.zk_connect()
        self._initialized = True

    def write_billing_log(self, last_ts, current_ts, params):
        """
        Write billing log.

        Split onto 2 messages if hour changed within billing interval
        """
        computed_last_ts = last_ts
        while (current_ts - computed_last_ts) > 1800:
            self.log.error('Unexpected report timestamp diff: current: %s last: %s', current_ts, computed_last_ts)
            self.write_billing_log(computed_last_ts, computed_last_ts + 1800, params)
            computed_last_ts += 1800
        last_tuple = time.gmtime(computed_last_ts)
        current_tuple = time.gmtime(current_ts)
        if last_tuple.tm_hour != current_tuple.tm_hour:
            split_ts = calendar.timegm(
                (current_tuple.tm_year, current_tuple.tm_mon, current_tuple.tm_mday, current_tuple.tm_hour, 0, 0,
                 current_tuple.tm_wday, current_tuple.tm_yday, current_tuple.tm_isdst))
            self.billing_log.info('', extra=dict(params=params, last_ts=computed_last_ts, ts=split_ts))
            computed_last_ts = split_ts
        self.billing_log.info('', extra=dict(params=params, last_ts=computed_last_ts, ts=current_ts))

    def check_logbroker_conn(self):
        """
        Check tcp connection to logbroker master slb
        """
        try:
            with closing(
                    socket.create_connection((self.config['lb_master_addr'], self.config['lb_master_port']),
                                             timeout=self.config['lb_master_timeout'])):
                self.log.debug('LB master conn established')
                return True
        except Exception as exc:
            self.log.error('Unable to connect to logbroker: %s', repr(exc))
            return False

    def has_lock(self):
        """
        Check if we are holding lock in zk
        """
        try:
            if not self.check_logbroker_conn():
                if self.lock.is_acquired:
                    self.log.debug('Releasing lock due to no conn to lb')
                    self.lock.release()
                return False
            if self._fqdn not in self.lock.contenders():
                self.log.debug('Trying to acquire lock as %s', self._fqdn)
                self.lock.acquire(blocking=False)
                time.sleep(1)
            return self.lock.is_acquired
        except Exception:
            self.zk_connect()

    def report(self):
        """
        Query metadb/zk and write log
        """
        try:
            with self.pg_conn as txn:
                cursor = txn.cursor()
                # psycopg2 adapts tuple to IN-LIST
                cursor.execute(OFFLINE_QUERY, (tuple(self.config['billed_cluster_types']),))
                metadb_hosts = {x['fqdn']: x for x in cursor.fetchall()}
                state_hosts = set(self.zk_conn.get_children(self.config['zk_state_path'])) - set(['last_report'])
                current_ts = time.time()
                for host, data in metadb_hosts.items():
                    state_path = os.path.join(self.config['zk_state_path'], host)
                    if host in state_hosts:
                        try:
                            last_data = json.loads(self.zk_conn.get(state_path)[0].decode('utf-8'))
                        except Exception as exc:
                            self.log.error('Unable to get last state for %s: %s', host, repr(exc))
                            last_data = {}
                        if last_data.get('stop_time', data['stop_time']) >= data['stop_time']:
                            last_ts = last_data['report_ts']
                        else:
                            last_ts = data['stop_time']
                    else:
                        last_ts = data['stop_time']
                    last_data = deepcopy(data)
                    last_data['report_ts'] = current_ts
                    self.zk_conn.ensure_path(state_path)
                    self.zk_conn.set(state_path, json.dumps(last_data).encode())
                    self.write_billing_log(int(last_ts), int(current_ts), data)
                for host in state_hosts:
                    if host in metadb_hosts:
                        continue
                    state_path = os.path.join(self.config['zk_state_path'], host)
                    result = None
                    try:
                        last_data = json.loads(self.zk_conn.get(state_path)[0].decode('utf-8'))
                        cursor.execute(
                            ONLINE_QUERY, {
                                'task_type': '{type}_start'.format(type=last_data['cluster_type']),
                                'cid': last_data['cluster_id'],
                                'timestamp': last_data['report_ts'],
                            })
                        result = cursor.fetchall()
                    except Exception as exc:
                        self.log.error('Unable to get stale host %s: %s', host, repr(exc))
                    self.zk_conn.delete(state_path)
                    if result:
                        self.write_billing_log(int(last_data['report_ts']), int(result[0]['timestamp']), last_data)
            self.zk_conn.ensure_path(os.path.join(self.config['zk_state_path'], 'last_report'))
            self.zk_conn.set(os.path.join(self.config['zk_state_path'], 'last_report'), str(time.time()).encode())
        except Exception:
            self.log.error('Unable to run report: %s', traceback.format_exc())
            self.pg_connect()
            self.zk_connect()


REPORTER = BillingReporter()


def offline_billing(config):
    """
    Run billing report (use this as dbaas-cron target function)
    """
    REPORTER.initialize(config)

    if REPORTER.has_lock():
        REPORTER.log.info('Have lock. Starting report')
        REPORTER.report()
    else:
        REPORTER.log.info('No lock. Skipping report')

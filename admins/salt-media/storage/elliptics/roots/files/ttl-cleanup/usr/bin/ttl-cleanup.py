{%- set env =  grains['yandex-environment'] -%}
{%- set prod =  ['prestable', 'production'] -%}
#!/usr/bin/pymds

import os
import logging
import sys
import time
import datetime
from msgpack import packb
from kazoo.client import KazooClient
from kazoo.exceptions import NodeExistsError
from socket import getfqdn
import json
import random

from mds.admin.library.python.sa_scripts import mm
from mastermind.errors import MastermindError

log = logging.getLogger('ttl-cleanup')
CURRENT_LEVEL = 0
CURRENT_MSG = ''
if os.path.isfile('/etc/yandex/environment.type'):
    f = open('/etc/yandex/environment.type', 'r')
    environment = f.read().split('\n')[0]
    f.close()
else:
    environment = 'testing'


class StreamToLogger(object):
    """Fake file-like stream object that redirects writes to logger"""

    def __init__(self, logger, log_level=logging.INFO):
        self.logger = logger
        self.log_level = log_level
        self.linebuf = ''

    def write(self, buf):
        """method to writing stream lines to logger"""
        for line in buf.rstrip().splitlines():
            self.logger.log(self.log_level, line.rstrip())

    def flush(self):
        """fake method, that can be called by sys"""
        pass


def init_kazoo_client(conf):
    """Setup kazoo client params and establish connection to zookeeper."""
    attempts = conf['attempts']
    delay = float(conf['attempts_delay'])
    hosts = conf['hosts']
    timeout = float(conf['timeout'])

    # http://kazoo.readthedocs.io/en/latest/api/retry.html#kazoo.retry.KazooRetry
    retry_params = dict(max_tries=attempts, delay=delay,)
    # http://kazoo.readthedocs.io/en/latest/api/client.html#kazoo.client.KazooClient
    zk = KazooClient(
        hosts=hosts, timeout=timeout, connection_retry=retry_params, command_retry=retry_params,
    )
    try:
        log.debug("connecting to zookeeper")
        zk.start()
    except Exception as e:
        msg = "zookeeper connection was failed: {}".format(repr(e))
        log.error(msg)
        monitor(msg)
        raise
    else:
        log.info("connected to zookeeper")
    return zk


def get_lock(use, conf):
    """Acquiring zookeeper lock."""
    locked = False
    if use:
        lock_path = conf['lock_path']
        lock_ttl = conf['lock_ttl']

        zk = init_kazoo_client(conf)

        timestamp = int(time.time())
        fqdn = getfqdn()
        lock_value = json.dumps(dict(fqdn=fqdn, timestamp=timestamp))
        try:
            # http://kazoo.readthedocs.io/en/latest/api/client.html#kazoo.client.KazooClient.retry
            lock_node = zk.retry(
                # http://kazoo.readthedocs.io/en/latest/api/client.html#kazoo.client.KazooClient.create
                zk.create,
                lock_path,
                lock_value,
                None,
                False,
                False,
                True,
            )
            if not lock_node == lock_path:
                raise ValueError("unexpected lock path {}, must be {}".format(lock_node, lock_path))
        except NodeExistsError:
            log.info("lock %s already exists", lock_path)
            try:
                # http://kazoo.readthedocs.io/en/latest/api/client.html#kazoo.client.KazooClient.get
                existing_lock_data = zk.retry(zk.get, lock_path)[0]
                existing_lock = json.loads(existing_lock_data)
            except Exception as get_existing_lock_exception:
                msg = "getting content of existing lock was failed: {}".format(
                    repr(get_existing_lock_exception)
                )
                log.error(msg)
                monitor(msg)
                raise get_existing_lock_exception
            else:
                if existing_lock['fqdn'] == fqdn and existing_lock['timestamp'] == timestamp:
                    log.info(
                        "lock successfully acquired: %s, ts=%d, fqdn=%s", lock_path, timestamp, fqdn
                    )
                    locked = True
                elif existing_lock['timestamp'] <= (timestamp - lock_ttl):
                    human_existing_lock_ts = to_human_readable_ts(existing_lock['timestamp'])
                    msg = "lock is expired, previous launch by {} at {} was failed".format(
                        existing_lock['fqdn'], human_existing_lock_ts
                    )
                    log.error(msg)
                    monitor(msg)
                    raise RuntimeError(msg)
                else:
                    human_existing_lock_ts = to_human_readable_ts(existing_lock['timestamp'])
                    msg = "already locked by {} since {}".format(
                        existing_lock['fqdn'], human_existing_lock_ts
                    )
                    log.info(msg)
                    monitor(msg, 0)
                    raise SystemExit(0)
            finally:
                deinit_kazoo_client(zk)
        except Exception as other_error:
            msg = "creating lock was failed: {}".format(repr(other_error))
            log.error(msg)
            monitor(msg)
            raise other_error
        else:
            log.info("lock successfully acquired: %s, ts=%d, fqdn=%s", lock_node, timestamp, fqdn)
            locked = True
        finally:
            deinit_kazoo_client(zk)
    return locked


def deinit_kazoo_client(zk):
    """Disconnect from zookeeper and cleanup kazoo client."""
    try:
        zk.stop()
        zk.close()
    except Exception as e:
        log.error("stopping kazoo client was failed: %s", repr(e))


def release_lock(lock, conf):
    """Release zookeeper lock."""
    if lock:
        lock_path = conf['lock_path']
        try:
            log.info("releasing lock %s", lock_path)

            zk = init_kazoo_client(conf)

            zk.retry(zk.delete, lock_path)
        except Exception as e:
            log.error("releasing lock was failed: %s", repr(e))
        else:
            log.info("lock successfully released")
        finally:
            deinit_kazoo_client(zk)


def release_lock_option(rel_lock, conf):
    """Release lock and exit"""
    if rel_lock:
        log.info("release lock mode")
        release_lock(True, conf)
        raise SystemExit(0)


def load_logger():
    """Create logger with path and loglevel from config."""
    path = '/var/log/ttl-cleanup.log'
    level = 'debug'
    try:
        logdir = os.path.dirname(path)
        if not os.path.exists(logdir):
            os.makedirs(logdir, 0755)
    except Exception as e:
        print "can't create log directory {}: {}".format(logdir, repr(e))
        raise
    try:
        fh = logging.FileHandler(path)
        formatter = logging.Formatter('%(asctime)s %(levelname)s %(message)s')
        fh.setFormatter(formatter)
        log.addHandler(fh)
        numeric_level = getattr(logging, level.upper(), None)
        if not isinstance(numeric_level, int):
            raise ValueError("invalid log level: {}".format(level))
        log.setLevel(numeric_level)
        logging.getLogger('http').handlers = [fh]
        # sys.stdout = StreamToLogger(log, logging.INFO)
        # sys.stderr = StreamToLogger(log, logging.ERROR)
    except Exception as e:
        print "can't initialize file logger: {}".format(repr(e))
        raise


def monitor(message, level=2, use=None, path=None, end=False):
    """Dump code and message to file for monrun monitoring script."""
    if use is None:
        use = True
    if path is None:
        path = '/var/tmp/ttl-cleanup'
    if use:
        try:
            mondir = os.path.dirname(path)
            if not os.path.exists(mondir):
                os.makedirs(mondir, 0755)
        except Exception as e:
            log.error("can't create monitor directory %s: %s", mondir, repr(e))
            raise

        global CURRENT_LEVEL
        global CURRENT_MSG
        if level > CURRENT_LEVEL:
            CURRENT_LEVEL = level
        if message:
            CURRENT_MSG += '{}; '.format(message)
            log.debug("monitoring: {}; {}".format(CURRENT_LEVEL, CURRENT_MSG))
        if end:
            try:
                monitor_message = '{}; {}'.format(CURRENT_LEVEL, CURRENT_MSG)
                with open(path, 'w') as monitor_file:
                    monitor_file.write(monitor_message)
            except Exception as e:
                msg = "can't write message into monitoring file: {}".format(repr(e))
                log.error(msg)
                print msg
                raise


@mm.mm_retry()
def get_ns_couples(mm_client, ns):
    return mm_client.couples.filter(**{'namespace': ns})


@mm.mm_retry()
def get_couples(mm_client, ns, config, default_config):
    couples = get_ns_couples(mm_client, ns)
    day = datetime.datetime.now().timetuple().tm_yday
    if 'delay' not in config:
        delay = default_config['delay']
    else:
        delay = config['delay']

    if 'min_used_space' not in config:
        min_used_space = default_config['min_used_space']
    else:
        min_used_space = config['min_used_space']

    i = day % delay

    cleanup_couples = []
    skip = 0
    for couple in couples:
        groupsets = couple.groupsets
        if len(groupsets) != 1 and 'replicas' not in groupsets:
            msg = 'Couple {0} have > 1 groupset'.format(couple.id)
            log.error(msg)
            monitor(msg)
            raise

        groupset = groupsets['replicas']
        if abs(hash(groupset.id)) % delay != i:
            continue
        else:
            try:
                used_space = (
                    groupset._data['effective_space'] - groupset._data['free_effective_space']
                )
                used_space = used_space / 1024.0 / 1024 / 1024
                if used_space < min_used_space:
                    msg = '{}: Used space < {}. Skip'.format(couple.id, min_used_space)
                    log.debug(msg)
                    skip += 1
                    continue
                else:
                    cleanup_couples.append(couple)
            except Exception as e:
                msg = e
                log.error(e)
                monitor(e)
                raise
    return cleanup_couples, skip


@mm.mm_retry()
def create_ttl_job(mm_client, params):
    return mm_client.request('ttl_cleanup', params)


@mm.mm_retry()
def create_karl_rm_job(mm_client, params):
    return mm_client.request('create_job', ['karl_rm_job', params])


def get_job_params(couple, ns, job_config):
        if not job_config['use_karl']:
            params = {
                'couple': couple.id,
                'namespace': ns,
                'iter_group': couple.id,
                'batch_size': job_config['batch_size'],
                'attempts': job_config['attempts'],
                'nproc': job_config['nproc'],
                'wait_timeout': job_config['wait_timeout'],
                'dry_run': job_config['dry_run'],
                'need_approving': False,
                'allow_orm': True,
            }
        else:
            params = {
                'couple_id': couple.id,
                'remove_expired': True,  # set default mode
                'iterate_group': random.choice(couple.groupsets['replicas'].group_ids),
                'batch_size': job_config['batch_size'],
                'attempts': job_config['attempts'],
                'wait_timeout': job_config['wait_timeout'],
                'lookup_timeout': job_config['lookup_timeout'],
                'remove_timeout': job_config['remove_timeout'],
                'dry_run': job_config['dry_run'],
                'mix_groups': job_config['mix_groups'],  # spread removing load to all groups
                'need_approving': False,
            }
        return params


def create_mastermind_jobs(mm_client, ns, couples, config, default_config, skip):
    """Create mastermind ttl_cleanup jobs for couples from argument"""
    attempts = range(1, 4)

    if 'crit_percent' not in config:
        crit_percent = default_config['crit_percent']
    else:
        crit_percent = config['crit_percent']

    if 'warn_percent' not in config:
        warn_percent = default_config['warn_percent']
    else:
        warn_percent = config['warn_percent']

    if 'use_karl' not in config:
        use_karl = default_config.get('use_karl', False)
    else:
        use_karl = config['use_karl']

    delay = 2
    job_config = {
        "batch_size": 100,
        "attempts": 1,
        "nproc": 10,
        "wait_timeout": 60,
        "dry_run": False,
        "lookup_timeout": 1000,
        "remove_timeout": 1000,
        "use_karl": use_karl,
        "mix_groups" : config.get('mix_groups', False),
    }

    log.info("start creating ttl_cleanup jobs for {} with karl {}".format(ns, use_karl))
    created_counter = 0
    try:
        for couple in couples:
            params = get_job_params(couple, ns, job_config)
            created = False
            job_id = None

            for attempt in attempts:
                try:
                    log.debug("creating job for couple %d: attempt=%d", couple.id, attempt)
                    if not job_config['use_karl']:
                        job = create_ttl_job(mm_client, params)
                    else:
                        job = create_karl_rm_job(mm_client, params)
                    job_id = job['id']
                except MastermindError as e:
                    log.exception(
                        "can't create job for couple %d %s attempt=%d", couple.id, repr(e), attempt
                    )
                    if ' is already acquired by' in e.message:
                        break
                else:
                    created = True
                    log.info(
                        "job %s for couple %d created with attempt=%d", job_id, couple.id, attempt
                    )
                    break
            else:
                log.error(
                    "creating job for couple %d was failed: all attempts was reached", couple.id
                )

            msg = "couple={} job_id={} created={}".format(couple.id, job_id, str(created))
            log.info(msg)

            created_counter += int(created)
    except Exception as e:
        msg = "{}: can't create ttl_cleanup jobs: {}".format(ns, repr(e))
        log.error(msg)
        monitor(msg)
        raise

    couples_counter = len(couples)
    if created_counter < couples_counter:
        p = float(couples_counter - created_counter) / couples_counter * 100
        msg = "{}: ttl_cleanup jobs was partially created: total={} created={} skip={}".format(
            ns, couples_counter, created_counter, skip
        )
        log.error(msg)
        if p >= crit_percent:
            monitor(msg, 2)
        elif p >= warn_percent:
            monitor(msg, 1)
    else:
        log.info(
            "{}: ttl_cleanup jobs was created successfully {}/{} skip {}".format(
                ns, created_counter, couples_counter, skip
            )
        )
        # monitor("{}: {}/{}".format(ns, created_counter, couples_counter), 0)


def to_human_readable_ts(timestamp):
    """Convert timestamp from unixtime to human readable format."""
    return datetime.datetime.fromtimestamp(int(timestamp)).strftime('%Y-%m-%d %H:%M:%S')


@mm.mm_retry()
def get_ttl_namespaces(mm_client):
    ns_list = []
    for x in mm_client.namespaces.filter():
        if 'attributes' in x.settings:
            attributes = x.settings['attributes']
            if (
                'ttl' in attributes
                and 'enable' in attributes['ttl']
                and attributes['ttl']['enable'] == True
            ):
                ns_list.append(x.id)
    return ns_list


def main():
    if environment in ['production', 'prestable']:
        default_config = {
            'min_used_space': 100,  # G
            'delay': 7,
            'warn_percent': 10,
            'crit_percent': 20,
        }
    else:
        default_config = {
            'min_used_space': 100,  # G
            'delay': 7,
            'warn_percent': 10,
            'crit_percent': 20,
            'use_karl': True
        }

    if environment in ['production', 'prestable']:
        config = {
            'mail-tmp': {
                'delay': 7,
                'min_used_space': 100,  # G
                'warn_percent': 10,
                'crit_percent': 20,
            },
            'antivir': {'delay': 30,},
            'avatars-pds-yml': {'delay': 14,},
            'distbuild': {'delay': 7,},
            'avatars-zen_doc': {'delay': 14,},
            'speechbase': {'delay': 90,},
            'turbo-commodity-feed': {'delay': 3,},
            'cid-offline': {'delay': 4,},
            'scutum': {'crit_percent': 75,},
            'avatars-ultra-images': {'crit_percent': 75, 'delay': 30,},
            'avatars-images-cbir': {'delay': 30,},
            'avatars-main-images': {'delay': 14,},
            'avatars-fast-images': {'delay': 4,},
            'avatars-auto-av': {'delay': 100500,},
            'matrix-router-result': {'delay': 3,},
            'matrix-router-result-testing': {'delay': 2,},
            'matrix-router-osm-result': {'delay': 3,},
            'alet': {'delay': 100500,},
            'qdm': {'delay': 100500,},
            "zk": {
                "hosts": "zk01vla.mds.yandex.net:2181,zk01myt.mds.yandex.net:2181,zk01sas.mds.yandex.net:2181",
                "lock_path": "/ttl-cleanup/lock",
                "attempts": 5,
                "attempts_delay": 1,
                "timeout": 10,
                "lock_ttl": 10800,
            },
            "use_lock": True,
        }
    else:
        config = {
            "zk": {
                "hosts": "zk01man.mdst.yandex.net:2181,zk01myt.mdst.yandex.net:2181,zk01sas.mdst.yandex.net:2181",
                "lock_path": "/ttl-cleanup/lock",
                "attempts": 5,
                "attempts_delay": 1,
                "timeout": 10,
                "lock_ttl": 10800,
            },
            'avatars-autoru-vos': {'delay': 3,},
            'curiosity-karl-only': {'use_karl': True},
            'curiosity-fed-2': {'use_karl': True, 'mix_groups': True},
            'matrix-router-result': {'delay': 2,},
            'matrix-router-result-testing': {'delay': 2,},
            'matrix-router-osm-result': {'delay': 2,},
            "use_lock": True,
        }

    try:
        load_logger()
        log.info("=====start ttl-cleanup-launcher=====")
        release_lock_option(False, config['zk'])

        fqdn = getfqdn()
        if environment in ['testing']:
            d = 1800
        else:
            d = 800
        delay = abs(hash(fqdn)) % (d)
        log.debug("sleep %d", delay)
        time.sleep(delay)

        lock = get_lock(config['use_lock'], config['zk'])

        mm_client = mm.mastermind_service_client()

        ttl_namespaces = get_ttl_namespaces(mm_client)

        for ns in ttl_namespaces:
            c = config.get(ns, {})
            couples, skip = get_couples(mm_client, ns, c, default_config)

            if couples:
                create_mastermind_jobs(mm_client, ns, couples, c, default_config, skip)

        release_lock(config['use_lock'], config['zk'])
    except KeyboardInterrupt:
        print "exiting by keyboard interrupt"
        release_lock(config['use_lock'], config['zk'])
        sys.exit(0)
    except Exception as e:
        log.exception(e)
        monitor(e, 2, end=True)
        release_lock(config['use_lock'], config['zk'])
        sys.exit(1)
    finally:
        monitor('', 0, end=True)
        log.info("=====stop ttl-cleanup=====")
        logging.shutdown()


if __name__ == '__main__':
    main()

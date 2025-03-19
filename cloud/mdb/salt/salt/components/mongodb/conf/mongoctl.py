#!/usr/bin/python3.6

import argparse
import grp
import enum
import logging
import os
import pwd
import signal
import socket
import ssl
import subprocess
import sys
import time

import json
import pymongo


CONFIG_FILE = '/etc/yandex/mongoctl.conf'
DBAAS_CONF = '/etc/dbaas.conf'
CONFIG = {
    'mongopass': '/root/.mongopass',
    'numactl_bin': '/usr/bin/numactl',
    # Real timeout is MAX_RS_TIMEOUT * tryN, i.e. with each try it will be longer
    'rs_timeout': 600,
    'max_tries': 10,
    'stop_timeout': 600,
    'use_sigkill_on_stop': True,
    'socket_timeout': 60,
    'shard_nodes': [],
    'graceful_stop': True,
    'check_on_start': True,
    'check_log_on_start': True,
    'check_port_on_start': True,
    'check_port_on_localhost': True,
    'check_port_timeout': 60 * 60 * 1,  # One hour
    # Port is needed as we can hame more than one service on host
    'port': None,
    'use_ssl': True,
    'ssl_ca_certs': '/etc/mongodb/ssl/allCAs.pem',
    'mongo_log_file': '/var/log/mongodb/mongod.log',
    'log_lines_treshold': 10,
    'log_read_timeout': 60,
}


logging.basicConfig(
    filename='/var/log/mongodb/mdb-mongoctl.log',
    format='%(asctime)s [%(levelname)s] %(process)d %(module)s:\t%(message)s',
    level=logging.DEBUG,
)
log = logging.getLogger("mongoctl")

syslogHandler = logging.StreamHandler()
syslogHandler.setLevel(logging.WARNING)
syslogHandler.setFormatter(
    logging.Formatter(
        '[%(levelname)s] %(module)s:\t%(message)s'
    )
)
log.addHandler(syslogHandler)


def str2bool(v):
    return v.lower() in ("yes", "true", "t", "1")


class MongoStatus(enum.Enum):
    # https://docs.mongodb.com/manual/reference/replica-states/
    # Not yet an active member of any set. All members start up in this state.
    # The mongod parses the replica set configuration document while in STARTUP.
    STARTUP = 0
    # The member in state primary is the only member that can accept write operations.
    # Eligible to vote.
    PRIMARY = 1
    # A member in state secondary is replicating the data store. Eligible to vote.
    SECONDARY = 2
    # Members either perform startup self-checks, or transition from completing a
    # rollback or resync. Eligible to vote.
    RECOVERING = 3
    # The member has joined the set and is running an initial sync. Eligible to vote.
    STARTUP2 = 5
    # The member’s state, as seen from another member of the set, is not yet known.
    UNKNOWN = 6
    # Arbiters do not replicate data and exist solely to participate in elections.
    # Eligible to vote.
    ARBITER = 7
    # The member, as seen from another member of the set, is unreachable.
    DOWN = 8
    # This member is actively performing a rollback. Eligible to vote.
    # Data is not available for reads from this member.
    # Starting in version 4.2, MongoDB kills all in-progress user operations
    #  when a member enters the ROLLBACK state.
    ROLLBACK = 9
    # This member was once in a replica set but was subsequently removed.
    REMOVED = 10


GOOD_MONGO_STATUS_LIST = [MongoStatus.PRIMARY, MongoStatus.SECONDARY, MongoStatus.RECOVERING,
                          MongoStatus.STARTUP2, MongoStatus.ARBITER, MongoStatus.ROLLBACK]


def readJSON(fname):
    with open(fname) as f:
        return json.load(f)


def readConfig(args=None):
    global CONFIG

    if args is None:
        args = CONFIG['args']
    else:
        CONFIG['args'] = args

    newConfig = {}
    newConfig.update(CONFIG)
    try:
        newConfig.update(readJSON(args.config))

        dbaas_conf = readJSON(DBAAS_CONF)
        newConfig['shard_nodes'] = dbaas_conf.get('shard_hosts', [])
    except Exception as exc:
        log.error(exc, exc_info=True)

    if args.timeout is not None:
        newConfig['stop_timeout'] = args.timeout
        newConfig['socket_timeout'] = args.timeout
    if args.rs_timeout is not None:
        newConfig['rs_timeout'] = args.rs_timeout
    if args.tries is not None:
        newConfig['max_tries'] = args.tries

    if len(newConfig['shard_nodes']) == 1:
        # If it's one-node RS, no need to try stepdown
        newConfig['gracefull_stop'] = False

    if args.port is not None:
        newConfig['port'] = int(args.port)

    if args.mongo_log_file is not None:
        newConfig['mongo_log_file'] = args.mongo_log_file
    if args.log_lines_treshold is not None:
        newConfig['log_lines_treshold'] = args.log_lines_treshold
    if args.log_read_timeout is not None:
        newConfig['log_read_timeout'] = args.mongo_log_file

    if newConfig != CONFIG:
        CONFIG.update(newConfig)
        log.debug("Config changed: %s", CONFIG)


class MongoDBAuthFailed(Exception):
    pass


class MongoDBHaveNoRS(Exception):
    pass


def get_host_port_from_mongopass():
    with open(CONFIG['mongopass']) as mongopass:
        for line in mongopass:
            host, port, db, user, password = line.split(':', 4)
            if (CONFIG['port'] is None or int(port) == CONFIG['port']):
                return (host, int(port))

    log.error("Unable to get mongodb host and port")
    raise Exception("Unable to get mongodb host and port")


def get_connection_opts(**kwargs):
    with open(CONFIG['mongopass']) as mongopass:
        for line in mongopass:
            host, port, db, user, password = line.split(':', 4)
            password = password.rstrip('\n')
            # rc1b-xn55d1njt97viusm.mdb.cloud-preprod.yandex.net:27018:admin:admin:PASSWORD
            if user == 'admin' and (CONFIG['port'] is None or int(port) == CONFIG['port']):
                log.debug(
                    "Connection string is: mongodb://%s:<PASSWORD>@%s:%s/%s",
                    user,
                    host,
                    port,
                    db,
                )
                res = {
                        'host': host,
                        'port': int(port),
                        'username': user,
                        'password': password,
                        'authSource': db,
                        'appname': 'mongoctl',
                        'serverSelectionTimeoutMS': 5000,
                        'socketTimeoutMS': CONFIG['socket_timeout'] * 1000,
                }
                if CONFIG['use_ssl']:
                    res.update({
                        'ssl': True,
                        'ssl_ca_certs': CONFIG['ssl_ca_certs'],
                        'ssl_cert_reqs': ssl.CERT_NONE,
                    })
                if kwargs is not None:
                    res.update(kwargs)

                logRes = res.copy()
                logRes['password'] = '***'
                log.debug("Connection options: %s", logRes)
                return res

    log.error("Unable to get connection options")
    raise Exception("Unable to get connection options")


def make_mongo_connection(**kwargs):
    return pymongo.MongoClient(**get_connection_opts(**kwargs))


def check_pymongo_exception(exc):
    if exc.code == 18:
        raise MongoDBAuthFailed()

    raise exc


def get_rs_status():
    try:
        with make_mongo_connection() as conn:
            res = conn['admin'].command('replSetGetStatus')
            log.debug('rs.status() = %s', res)
            return res
    except pymongo.errors.OperationFailure as exc:
        log.debug("%s", exc, exc_info=True)
        check_pymongo_exception(exc)


def get_mongo_hostname():
    try:
        with make_mongo_connection() as conn:
            res = conn['admin'].command('serverStatus').get('host')
            log.debug("Current MongoDB hostname is: %s", res)
            return res
    except pymongo.errors.OperationFailure as exc:
        log.debug("%s", exc, exc_info=True)
        check_pymongo_exception(exc)


def get_numactl():
    numactl = [CONFIG['numactl_bin'], '--interleave=all']

    # Check what numactl is working on current host
    ret = subprocess.run(numactl + ['ls', '/'], stdout=subprocess.DEVNULL)
    if (ret.returncode != 0):
        return []
    return numactl


def sudo(user_uid, user_gid):
    def result():
        os.setgid(user_gid)
        os.setuid(user_uid)

    return result


def run_mongo(user='mongodb', group='mongodb', argv=['/usr/bin/mongod', '--config', '/etc/mongodb/mongodb.conf']):
    numactl = get_numactl()
    args = numactl + argv

    log.debug("Will start {args}".format(args=' '.join(args)))

    pw_record = pwd.getpwnam(user)
    sudo_uid = pw_record.pw_uid
    sudo_gid = pw_record.pw_gid

    if (group is not None):
        grp_record = grp.getgrnam(group)
        sudo_gid = grp_record.gr_gid

    env = os.environ.copy()
    env.update({
        'HOME': pw_record.pw_dir,
        'LOGNAME': pw_record.pw_name,
        'PWD': pw_record.pw_dir,
        'USER': pw_record.pw_name,
        })

    log.debug(
        "Will execute subprocess.Popen(args=%s, preexec_fn=sudo(%s, %s), env=%s)",
        args,
        sudo_uid,
        sudo_gid,
        env,
    )
    os.umask(int('0037', 8))
    return subprocess.Popen(
        args,
        preexec_fn=sudo(sudo_uid, sudo_gid),
        env=env
    )


def get_my_rs_status(host):
    assert host is not None

    rsstatus = get_rs_status()

    if rsstatus.get('ok', False) is False:
        raise MongoDBHaveNoRS()

    for member in rsstatus.get('members', []):
        if member.get('name') == host:
            return member

    raise Exception("Host {} wasn't found in rs.status() response: {}".format(
        host,
        repr(rsstatus),
    ))


def tailF(f, timeout):
    f.seek(0, 2)
    timeout_ts = time.time() + timeout
    while time.time() < timeout_ts:
        line = f.readline()
        if not line:
            time.sleep(1)  # One second. not like we really in hurry here
            continue
        yield line
        timeout_ts = time.time() + timeout

    log.debug("tailF(): log read timeout (%s) reached", timeout)


def wait_mongodb_log_initialized():
    try:
        initLinesCount = 0
        nonInitLinesCount = 0
        with open(CONFIG['mongo_log_file']) as f:
            logLines = tailF(f, CONFIG['log_read_timeout'])
            for line in logLines:
                isInitLine = any((
                    line.find(' [initandlisten] ') > 0,
                    line.find('"ctx":"initandlisten"') > 0,
                ))
                if isInitLine:
                    nonInitLinesCount = 0
                    initLinesCount += 1
                else:
                    initLinesCount = 0
                    nonInitLinesCount += 1

                if nonInitLinesCount >= CONFIG['log_lines_treshold']:
                    return

    except Exception as exc:
        # In case of error assume, that everything is Ok
        log.error("%s", exc, exc_info=True)


def wait_mongodb_port_open(host, port):
    '''
    Wait for MongoD port to became open
    '''
    start_ts = time.time()
    max_ts = start_ts + CONFIG['check_port_timeout']
    last_exc = None

    while time.time() < max_ts:
        # We can't be sure if ipv6 or ipv4 addres we should use, so try both
        try:
            s = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
            s.connect((host, port))
            s.close()
            return True
        except Exception as exc:
            last_exc = exc
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            s.connect((host, port))
            s.close()
            return True
        except Exception as exc:
            last_exc = exc

    log.error('%s', repr(last_exc), exc_info=True)
    return False


def start_and_wait_mongo(user, rs_timeout, tryN, args, pid_file=None):
    mongo = run_mongo(user=user, argv=args)
    pid = mongo.pid
    start_ts = time.time()
    host = None
    extra_wait = 0

    time.sleep(5)
    while time.time() - start_ts <= rs_timeout * tryN + extra_wait:
        mongo.poll()
        if mongo.returncode is not None:
            log.error("MongoDB exited with error code %s", mongo.returncode)
            # Stop mongo just in case it was started already not by systemd
            _stop_mongo(pid_file=pid_file)
            return False

        readConfig()
        ts_before_prechecks = time.time()
        if not CONFIG['check_log_on_start']:
            log.debug("check_log_on_start configuration parameter is set to False, skip log checks")
        else:
            wait_mongodb_log_initialized()

        if not CONFIG['check_port_on_start']:
            log.debug("check_port_on_start is set to False, skip port checks")
        else:
            (shost, port) = get_host_port_from_mongopass()
            if CONFIG['check_port_on_localhost']:
                shost = 'localhost'
            wait_mongodb_port_open(shost, port)
        # To our wait time add time of waiting mongodb to log+port initialisation
        extra_wait += time.time() - ts_before_prechecks

        if not CONFIG['check_on_start']:
            log.debug("check_on_start configuration parameter is set to False, skip checks")
            return True

        log.debug(
            'Checking if we are in RS, ts: %d of %d',
            time.time() - start_ts,
            rs_timeout * tryN + extra_wait,
        )
        try:
            if host is None:
                host = get_mongo_hostname()

            status = get_my_rs_status(host)
            state = status.get('state', -1)
            if MongoStatus(state) in GOOD_MONGO_STATUS_LIST:
                log.debug(
                    "We are in RS an our status is %s(%s)",
                    status.get('stateStr', ''),
                    state,
                )
                return True
            else:
                log.debug(
                    "Looks like we aren't in rs yet(?), our state is: %s(%s)",
                    status.get('stateStr', ''),
                    state,
                )
        except MongoDBAuthFailed:
            # Special case for when authentication failed
            # It, probably, means that we aren't initialized at all yet
            # For example cluster\host creation, resetup or something else
            # (anyway if auth was failed we won't be able to check mongo at all,
            # so it's better to start it and hope it'll works)
            log.info("Looks like authentication failed, assume everything is Ok")
            return True
        except MongoDBHaveNoRS:
            log.info(
                "RS status.ok is False, assume, that we are in initialisation status," +
                " exiting successfully"
            )
            return True
        except Exception as exc:
            log.error('%s', repr(exc), exc_info=True)
            log.debug("Wasn't been able to get RS status, waiting...")

        time.sleep(5)

    log.error(
        "After %d ms we are still have wrong rs status, restart needed",
        time.time() - start_ts,
    )
    _stop_mongo(pid=pid, popen_obj=mongo)
    return False


def check_pid(pid):
    """ Check For the existence of a unix pid. """
    try:
        os.kill(pid, 0)
    except OSError:
        return False
    else:
        return True


def mongo_send_shutdown_command(serverSelectionTimeoutMS=1000, **kwargs):
    timeout = CONFIG['stop_timeout']
    shutdownKWArgs = {
        'timeoutSecs': timeout,
    }
    shutdownKWArgs.update(kwargs)

    try:
        with make_mongo_connection(serverSelectionTimeoutMS=serverSelectionTimeoutMS) as conn:
            log.debug("Shutting down MongoDB with db.shutDown({})....".format(shutdownKWArgs))
            # shutDown command don't return anything
            conn['admin'].command('shutdown', 1, **shutdownKWArgs)

    except pymongo.errors.ServerSelectionTimeoutError:
        # Can't connect to MongoDB because gracefull shutdown was successfull
        pass
    except pymongo.errors.AutoReconnect:
        # It means that server successfully shutted down and reconnect failed
        pass
    except Exception as exc:
        log.error("Got exception while trying to do db.shutdownServer: %s", exc, exc_info=True)


def _stop_mongo(pid=None, pid_file=None, popen_obj=None):
    log.debug("Will stop MongoDB, pid: %s, pid_file: %s", pid, pid_file)
    timeout = CONFIG['stop_timeout']

    if pid_file is not None:
        try:
            with open(pid_file) as pid_file:
                pid = int(pid_file.read().replace('\n', ''))
                log.debug("pid_file found, got pid: %s", pid)
        except Exception as exc:
            log.error("Wasnt been able to read PID File: %s", exc, exc_info=True)

    # Try to gracefully shut down mongo server
    if CONFIG['graceful_stop'] and not str2bool(os.environ.get('MONGOCTL_SKIP_GRACEFUL_STOP', 'False')):
        mongo_send_shutdown_command()

    # If server still running, shutdown it with force
    mongo_send_shutdown_command(force=1)

    if pid is None:
        log.debug("MongoDB PID is None, assume it's stopped already")
        return True

    start_ts = time.time()
    while check_pid(pid) and ((time.time() - start_ts) < timeout):
        log.debug("Process with PID %s still exists, sending SIGTERM", pid)
        try:
            os.kill(pid, signal.SIGTERM)
            if popen_obj is not None:
                popen_obj.poll()
        except OSError:
            # Process sudenly exited, no need to kill it anymore
            break
        except Exception as exc:
            log.debug(
                "Got exception while trying to send TERM signal to %s: %s",
                pid,
                exc, exc_info=True
            )
        time.sleep(1)

    if check_pid(pid) and CONFIG['use_sigkill_on_stop']:
        log.warning('Looks like we was not been able to stop mongodb server')
        try:
            os.kill(pid, signal.SIGKILL)
            if popen_obj is not None:
                popen_obj.poll()
        except OSError:
            # Process sudenly exited, no need to kill it anymore
            pass
        except Exception as exc:
            log.debug(
                "Got exception while trying to send KILL signal to %s: %s",
                pid,
                exc, exc_info=True
            )
        time.sleep(1)

    if check_pid(pid):
        log.error('Looks like we was not been able to stop mongodb server')
        return False

    log.debug("MongoDB successfully stopped")
    return True


def stop_mongo(*args, **kwargs):
    if _stop_mongo(*args, **kwargs):
        sys.exit(0)
    else:
        sys.exit(1)


def start_mongo(user, argv, pid_file=None):
    max_tries = CONFIG['max_tries']
    rs_timeout = CONFIG['rs_timeout']

    if user is None:
        raise ValueError("user can't be None in case of start action")
    if argv is None:
        raise ValueError("argv can't be None in case of start action")

    log.debug("Try to start ['%s'] under user %s", "', '".join(argv), user)
    for i in range(max_tries):
        log.debug('Try #%d of %d', i+1, max_tries)
        if start_and_wait_mongo(user, i+1, rs_timeout, argv, pid_file=pid_file):
            log.info("MongoDB server started successfully")
            sys.exit(0)

    log.error("Unable to start MongoDB daemon after %d tries", max_tries)
    sys.exit(1)


def _main():
    global CONFIG
    parser = argparse.ArgumentParser(description='MongoDB management script')
    parser.add_argument(
        '--action',
        help="Action to perform start|stop",
        required=True,
        choices=['start', 'stop']
    )
    # Stop Args
    parser.add_argument(
        '--pid',
        help="PID of MongoDB process",
        type=int,
        default=None
    )
    parser.add_argument(
        '--pid-file',
        help="PID file of MongoDB process",
        default=None
    )
    parser.add_argument(
        '--timeout',
        help="Stepdown and shutdown timeouts",
        type=int,
        default=None
    )
    # Start Args
    parser.add_argument(
        '--rs-timeout',
        help="How long we need to wait for RS to initialise",
        type=int,
        default=None
    )
    parser.add_argument(
        '--tries',
        help="How much times we need to try to restart mongo",
        type=int,
        default=None
    )
    parser.add_argument(
        '--user',
        help="User to use to start MongoDB",
        default=None
    )
    parser.add_argument(
        '--config',
        help="User to use to start MongoDB",
        default=CONFIG_FILE
    )
    parser.add_argument(
        '--port',
        help="Port to connect to MongoDB",
        type=int,
        default=None
    )
    parser.add_argument(
        '--mongo-log-file',
        help="Path to MongoDB log file to watch for mongod initialisation",
        default=None
    )
    parser.add_argument(
        '--log-lines-treshold',
        help="How long sequence of initialization unrelated lines we should find in log '+\
        'to assume, that mongo is initialised",
        type=int,
        default=None
    )
    parser.add_argument(
        '--log-read-timeout',
        help="If log file have no new lines for more than this amount of seconds, stop to read it",
        type=int,
        default=None
    )
    parser.add_argument(
        'args',
        help="Path to mongod|mongos and list of arguments to pass to it",
        default=None,
        nargs=argparse.REMAINDER
    )
    args = parser.parse_args()

    readConfig(args)

    log.info("Started mongoctl with args %s", args)
    log.info("Config: %s", CONFIG)

    if args.action == 'start':
        start_mongo(args.user, args.args, pid_file=args.pid_file)
    elif args.action == 'stop':
        stop_mongo(pid=args.pid, pid_file=args.pid_file)
    else:
        raise ValueError('--action should be "start" or "stop"')


_main()

"""
MongoDB network communication
"""

import copy
import logging
import time
from enum import Enum
from typing import Any, Dict, List, NamedTuple, Optional

import bson
import pymongo
from pymongo.errors import (AutoReconnect, ConnectionFailure, OperationFailure, ServerSelectionTimeoutError)

from mdb_mongo_tools.exceptions import (ReplSetInfoGatheringError, ReplSetInfoMultiplePrimariesFound,
                                        ReplSetInfoPrimaryNotFound, StepdownTimeoutExceeded, StepdownUnexpectedError)
from mdb_mongo_tools.util import retry

mongo_retry_common_opts = dict(
    exception_types=(AutoReconnect, ConnectionFailure, OperationFailure, ServerSelectionTimeoutError), max_wait=30)

mongo_retry_connect = retry(
    exception_types=(AutoReconnect, ConnectionFailure, OperationFailure, ServerSelectionTimeoutError), max_wait=60)


class ReplSetMemberStates(Enum):
    """
    Replset state str to num map
    """
    STARTUP = 0
    PRIMARY = 1
    SECONDARY = 2
    RECOVERING = 3
    STARTUP2 = 5
    UNKNOWN = 6
    ARBITER = 7
    DOWN = 8
    ROLLBACK = 9
    REMOVED = 10


REPLSET_READABLE_STATES = (ReplSetMemberStates.PRIMARY, ReplSetMemberStates.SECONDARY)

DATADIR_ROOT = '/var/lib/mongodb'

MONGO_DEFAULT_CONNECT_TIMEOUT_MS = 5000
MONGO_DEFAULT_SOCKET_TIMEOUT_MS = 30000
MONGO_DEFAULT_SERVER_SELECTION_TIMEOUT_MS = 15000


def get_conn_options_string(options: dict) -> str:
    """
    Get connection timeout options
    """
    return '&'.join('{opt}={val}'.format(opt=opt, val=val) for opt, val in options.items())


class MongoConnection:
    """
    MongoDB connection wrapper
    """

    def __init__(self, conf: dict):
        self._conf = conf
        self._conn = self._connect()

    @mongo_retry_connect
    def _connect(self) -> pymongo.MongoClient:
        """
        Connect to mongodb
        """
        conn_opts = get_conn_options_string(self._conf['options'])

        auth = '' if not self._conf['auth'] else \
            '{username}:{password}@'.format(username=self._conf['username'], password=self._conf['password'])

        conn_str = 'mongodb://{auth}{host}:{port}/{database}?{options}'.format(
            auth=auth,
            host=self._conf['host'],
            port=int(self._conf['port']),
            database=self._conf['database'],
            options=conn_opts)
        return pymongo.MongoClient(conn_str)

    def close(self) -> None:
        """
        Close connection
        """
        self._conn.close()

    @property
    def is_alive(self) -> bool:
        """
        Ping connection
        """
        try:
            return self.exec_command(cmd='ping', retry_opts={})['ok'] == 1
        except pymongo.errors.ConnectionFailure as exc:
            logging.debug("ping failed: %s", exc)
            return False

    @property
    def host_port(self):
        """
        Get host:port
        """
        return '{host}:{port}'.format(host=self._conn.address[0], port=self._conn.address[1])

    def exec_command(self, cmd: str, dbname='admin', check: bool = True, retry_opts: Optional[dict] = None) -> Dict:
        """
        Run given command
        """
        if retry_opts is None:
            retry_opts = mongo_retry_common_opts

        if retry_opts:
            return retry(**retry_opts)(self._conn[dbname].command)(cmd, check=check)

        return self._conn[dbname].command(cmd, check=check)

    def _find(self, db: str, coll: str, filt: dict = None, limit: int = 0, sort: list = None, retry_opts: dict = None):
        if retry_opts is None:
            retry_opts = mongo_retry_common_opts

        if retry_opts:
            return retry(**retry_opts)(self._conn[db][coll].find)(filter=filt, limit=limit, sort=sort)

        return self._conn[db][coll].find(filter=filt, limit=limit, sort=sort)

    @property
    def oldest_oplog_ts(self) -> int:
        """
        Get timestamp of oldest oplog record
        """
        oplog_cur = self._find(db='local', coll='oplog.rs', sort=[('$natural', 1)], limit=1)
        return next(iter(oplog_cur))['ts'].time

    @property
    def newest_oplog_ts(self) -> int:
        """
        Get timestamp of newest oplog record
        """
        oplog_cur = self._find(db='local', coll='oplog.rs', sort=[('$natural', -1)], limit=1)
        return next(iter(oplog_cur))['ts'].time

    @property
    def dbpath(self) -> str:
        """
        Get configured dbPath
        """
        return self.exec_command('getCmdLineOpts')['parsed']['storage']['dbPath']

    @property
    def is_readable(self) -> bool:
        """
        Check if role is primary or secondary
        """
        cmd = self.exec_command('isMaster')
        return cmd['ismaster'] or cmd['secondary']

    @property
    def server_selection_timeout(self) -> int:
        """
        Server selection timeout for this instance in seconds
        """
        return self._conn.server_selection_timeout


class ReplSetMemberInfo(NamedTuple):
    """
    ReplicaSet member information
    """
    host_port: str
    state: ReplSetMemberStates
    votes: int
    optime_ts: Optional[int]
    oldest_oplog_ts: Optional[int]
    newest_oplog_ts: Optional[int]


class ReplSetConfigSettings(NamedTuple):
    """
    Replset config settings
    """
    heartbeat_timeout_secs: int


class ReplSetInfo:
    """
    ReplicaSet information
    """

    def __init__(self, conf: dict, conn: MongoConnection):
        self._conf = conf
        self._conn = conn

        self._rs_config_settings: Any = None

        self._members: List[ReplSetMemberInfo] = []
        self._rs_newest_oplog_ts: int = 0
        self._rs_last_oplog_ts: int = 0
        self._primary_dbs_size: int = 0

    def update(self, strict: bool = True):
        """
        Collect replica set data
        """
        self._members = []
        rs_status = self._conn.exec_command(cmd='replSetGetStatus')
        rs_config = self._conn.exec_command(cmd='replSetGetConfig')['config']
        self._rs_config_settings = ReplSetConfigSettings(
            heartbeat_timeout_secs=rs_config['settings']['heartbeatTimeoutSecs'])

        # TODO: move logging to upper level
        logging.debug('Fetched replSetGetStatus: %s', rs_status)
        logging.debug('Fetched replSetGetConfig[config]: %s', rs_config)

        per_host_conf = copy.deepcopy(self._conf)
        for host_rs_status, host_rs_config in zip(
                sorted(rs_status['members'], key=lambda x: x['_id']),
                sorted(rs_config['members'], key=lambda x: x['_id'])):

            host = host_rs_status['name'].split(':')[0]
            per_host_conf['host'] = host
            conn = MongoConnection(per_host_conf)
            host_rs_state = ReplSetMemberStates(host_rs_status['state'])

            rs_member_info_args = dict(
                host_port=host_rs_status['name'],
                state=host_rs_state,
                votes=host_rs_config['votes'],
                optime_ts=None,
                oldest_oplog_ts=None,
                newest_oplog_ts=None,
            )
            if host_rs_state not in [
                    ReplSetMemberStates.DOWN,
                    ReplSetMemberStates.ROLLBACK,
                    ReplSetMemberStates.STARTUP2,
            ]:
                try:
                    rs_member_info_args.update({
                        'oldest_oplog_ts': conn.oldest_oplog_ts,
                        'newest_oplog_ts': conn.newest_oplog_ts,
                        'optime_ts': host_rs_status['optime']['ts'].time,
                    })
                except pymongo.errors.ServerSelectionTimeoutError as exc:  # TODO: replace pymongo exception
                    logging.error('Marking %s as DOWN due to connection failure: %s.', host_rs_status['name'], exc)
                    rs_member_info_args['state'] = ReplSetMemberStates.DOWN

            if host_rs_state == ReplSetMemberStates.PRIMARY:  # TODO: fallback if primary is down
                cmd = conn.exec_command(cmd='listDatabases')
                self._primary_dbs_size = cmd['totalSize']

            self._members.append(ReplSetMemberInfo(**rs_member_info_args))
            conn.close()

        if strict:
            self.validate()

        valid_sync_members = [m for m in self._members if m.state in REPLSET_READABLE_STATES]
        if valid_sync_members:
            self._rs_newest_oplog_ts = min(
                m.newest_oplog_ts for m in valid_sync_members if m.newest_oplog_ts is not None)

    def validate(self) -> None:
        """
        Validate replSet state
        """
        valid_sync_members = [m for m in self._members if m.state in REPLSET_READABLE_STATES]
        if not valid_sync_members:
            raise ReplSetInfoGatheringError(
                'ReplicaSet is in crashed state: no readable members were found. Will not continue')

        primaries = [m for m in self._members if m.state == ReplSetMemberStates.PRIMARY]
        if len(primaries) > 1:
            raise ReplSetInfoMultiplePrimariesFound(
                'Primaries: {nodes}'.format(nodes=','.join([m.host_port for m in primaries])))
        elif len(primaries) < 1:
            raise ReplSetInfoPrimaryNotFound('Primary was not found')

    @property
    def primary_dbs_size(self) -> int:
        """
        Summary of all dbs size on primary
        """
        self.validate()
        return self._primary_dbs_size

    @property
    def config_settings(self) -> ReplSetConfigSettings:
        """
        Get replset config settings
        """
        return self._rs_config_settings

    @property
    def primary_host_port(self) -> str:
        """
        Primary hostname:port
        """
        self.validate()
        primaries = self._get_members_by_state(ReplSetMemberStates.PRIMARY)
        return next(iter(primaries)).host_port

    @property
    def votes_sum(self) -> int:
        """
        Summary of all replicaset votes
        """
        return sum(m.votes for m in self._members)

    @property
    def unreach_votes_sum(self) -> int:
        """
        Summary of all unreachable hosts votes
        """
        unreach_votes = [m.votes for m in self._members if m.state == ReplSetMemberStates.DOWN]
        return sum(unreach_votes) if unreach_votes else 0

    @property
    def rs_oplog_window(self) -> int:
        """
        Oplog window

        valid if oplog is full
        """
        self.validate()
        return int(time.time()) - self._rs_newest_oplog_ts

    def members_host_port(self, state: Optional[ReplSetMemberStates] = None) -> List[str]:
        """
        Get members host:port
        """
        if state is not None:
            return [h.host_port for h in self._members if h.state == state]

        return [m.host_port for m in self._members]

    def host_is_stale(self, host_port: str) -> bool:
        """
        Check if host_port is stale
        """
        rs_member = self._get_member_by_name(host_port)
        assert isinstance(rs_member.newest_oplog_ts, int)

        is_stale = rs_member.state == ReplSetMemberStates.RECOVERING and \
            rs_member.newest_oplog_ts < self._rs_newest_oplog_ts

        logging.debug('Last known oplog_ts is %s, last available in replset is %s', rs_member.newest_oplog_ts,
                      self._rs_newest_oplog_ts)
        return is_stale

    def host_is_in_startup2_state(self, host_port: str) -> bool:
        """
        Check if host_port is in STARTUP2 state
        """
        rs_member = self._get_member_by_name(host_port)
        return rs_member.state == ReplSetMemberStates.STARTUP2

    def host_is_readable(self, host_port: str) -> bool:
        """
        Check if host_port is primary or secondary
        """
        rs_member = self._get_member_by_name(host_port)
        return rs_member.state in REPLSET_READABLE_STATES

    def get_host_votes(self, host_port: str) -> int:
        """
        Return number of host votes
        """
        rs_member = self._get_member_by_name(host_port)
        return rs_member.votes

    def get_host_state(self, host_port: str) -> ReplSetMemberStates:
        """
        Return host role
        """
        rs_member = self._get_member_by_name(host_port)
        return rs_member.state

    def get_host_lag(self, host_port: str) -> int:
        """
        Return host lag in seconds
        """
        primary = next(iter(self._get_members_by_state(ReplSetMemberStates.PRIMARY)))
        rs_member = self._get_member_by_name(host_port)
        assert primary.optime_ts is not None
        assert rs_member.optime_ts is not None

        return primary.optime_ts - rs_member.optime_ts

    def _get_member_by_name(self, host_port: str) -> ReplSetMemberInfo:
        """
        Get member by host_port
        """
        for member in self._members:
            if member.host_port == host_port:
                return member
        raise KeyError('Host {host_port} was not found in replset members'.format(host_port=host_port))

    def _get_members_by_state(self, state: ReplSetMemberStates) -> List[ReplSetMemberInfo]:
        """
        Get members by state
        """
        return [m for m in self._members if m.state == state]


class ReplSetMember(NamedTuple):
    """
    ReplicaSet member
    """
    host_port: str
    conn: MongoConnection


class ReplSetCtl:
    """
    ReplicaSet control
    """

    def __init__(self, conf: dict, conn: MongoConnection):
        self._conf = conf
        self._conn = conn
        self._info = ReplSetInfo(conf, conn)
        self._members: List[ReplSetMember] = []
        self._discover()

    def _discover(self) -> None:
        self._members = []
        self._info.update()
        for host_port in self._info.members_host_port():
            per_host_conf = copy.deepcopy(self._conf)
            host, port = host_port.split(':')
            per_host_conf['host'] = host
            per_host_conf['port'] = port
            self._members.append(ReplSetMember(
                host_port=host_port,
                conn=MongoConnection(per_host_conf),
            ))

    def get_info(self) -> ReplSetInfo:
        """
        Get replset info
        """
        return self._info

    def stepdown_host(self, host_port: str, step_down_secs: int, secondary_catch_up_period_secs: int) -> None:
        """
        Run mongodb stepdown
        """
        cmd = bson.son.SON([('replSetStepDown', step_down_secs),
                            ('secondaryCatchUpPeriodSecs', secondary_catch_up_period_secs)])

        conn = self._get_member(host_port).conn
        try:
            result = conn.exec_command(cmd, check=False, retry_opts={})
            if result['ok'] == 1:
                return

            logging.error('Stepdown operation has failed: %s', result, exc_info=True)
            if result['code'] == 262:  # ExceededTimeLimit
                raise StepdownTimeoutExceeded(result['errmsg'])

            raise StepdownUnexpectedError(str(result))
        except pymongo.errors.AutoReconnect:
            pass

    def _get_member(self, host_port: str) -> ReplSetMember:
        """
        Get member by host:port
        """
        for member in self._members:
            if member.host_port == host_port:
                return member
        raise KeyError('Host {host_port} was not found in replset members'.format(host_port=host_port))

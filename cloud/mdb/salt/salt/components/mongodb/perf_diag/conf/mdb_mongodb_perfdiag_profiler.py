"""
MongoDB Profiler reporter
"""

from collections import OrderedDict
import json
import pymongo
import socket
from logging import DEBUG, Formatter, getLogger, FileHandler
from logging.handlers import MemoryHandler
from uuid import UUID
from bson.objectid import ObjectId
from bson.timestamp import Timestamp
import datetime
import timeout


_initialized = False


def MurMur3_32(key, seed=0x0):
    """
    MurMur3 Hash
    copied from mongo/src/third_party/murmurhash3/MurmurHash3.cpp
    """

    def rotl32(x, y):
        return (x << y | x >> (32 - y)) | 0xFFFFFFFF

    def fmix(x):
        x ^= x >> 16
        x = (x * 0x85ebca6b) & 0xFFFFFFFF
        x ^= x >> 13
        x = (x * 0xc2b2ae35) & 0xFFFFFFFF
        x ^= x >> 16
        return x

    key = bytearray(key)

    key_len = len(key)
    nblocks = key_len // 4

    h1 = seed

    c1 = 0xcc9e2d51
    c2 = 0x1b873593

    for i in range(0, nblocks * 4, 4):
        k1 = key[i + 3] << 24 | \
             key[i + 2] << 16 | \
             key[i + 1] << 8 | \
             key[i + 0]

        k1 = (k1 * c1) & 0xFFFFFFFF
        k1 = rotl32(k1, 15)
        k1 = (k1 * c2) & 0xFFFFFFFF

        h1 ^= k1
        h1 = rotl32(h1, 13)
        h1 = (h1*5+0xe6546b64) & 0xFFFFFFFF

    # tail
    tail_index = nblocks * 4
    k1 = 0
    tail_size = key_len & 3
    if tail_size == 3:
        k1 ^= key[tail_index + 2] << 16
    if tail_size >= 2:
        k1 ^= key[tail_index + 1] << 8
    if tail_size >= 1:
        k1 ^= key[tail_index]
        k1 = (k1 * c1) & 0xFFFFFFFF
        k1 = rotl32(k1, 15)
        k1 = (k1 * c2) & 0xFFFFFFFF
        h1 ^= k1

    # finalization

    h1 ^= key_len
    h1 = fmix(h1)
    return h1


def MurMur3_32_hex(key, seed=0x0):
    ret = MurMur3_32(key, seed)
    return '%08X' % ret


class MongodbJsonEncoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, ObjectId):
            return "ObjectId(\"{}\")".format(str(obj))
        if isinstance(obj, UUID):
            # if the obj is uuid, we simply return the value of uuid
            return "UUID({})".format(obj.hex)
        if isinstance(obj, Timestamp):
            return "Timestamp({}.{})".format(obj.time, obj.inc)
        if isinstance(obj, bytes):
            try:
                return obj.decode('utf-8')
            except UnicodeDecodeError:
                return obj.hex()
        if isinstance(obj, datetime.datetime):
            return "Datetime(\"{}\")".format(obj.replace(tzinfo=datetime.timezone.utc).isoformat())
        return json.JSONEncoder.default(self, obj)


class MongodbProfilerFormatter(Formatter):
    """
    MongoDB system.profile record formatter
    """

    def __init__(self, metadata=None):
        self.metadata = {}
        if metadata is not None:
            self.metadata = metadata.copy()

    def update_metadata(self, new_metadata, rewrite=True):
        if rewrite:
            self.metadata = new_metadata.copy()
        else:
            self.metadata.update(new_metadata)

    def format(self, record):
        """
        Format profiler record for pushclient
        """
        data = record.query
        # We do + [''] here to be sure that ns array has at least 2 items
        ns = data.get('ns', '.').split('.', 1) + ['']
        form_hash = ''
        form = OrderedDict(
            op=data.get('op', ''),
            ns=data.get('ns', ''),
        )

        if data.get('op', '') == 'query' and data.get('command', {}).get('find', None):
            form.update({
                'op': 'find',
                'filter': {},
            })
        elif data.get('op', '') == 'query' and data.get('command', {}).get('count', None):
            form.update({
                'op': 'count',
                'filter': {},
            })

        # Check if we have filter, and add filter form to request form
        if isinstance(data['command'].get('filter', None), dict):
            filter_form = OrderedDict()
            filter_keys = list(data['command'].get('filter', {}).keys())
            filter_keys.sort()  # Do sort so we get deterministic sequence
            for key in filter_keys:
                filter_form[key] = 1
            form['filter'] = filter_form

        if data.get('queryHash', None):
            form_hash = data.get('queryHash', None)
        else:
            form_hash = MurMur3_32_hex(json.dumps(form, sort_keys=False).encode('utf-8'))

        raw = '{}'
        try:
            raw = json.dumps(data, cls=MongodbJsonEncoder)
        except TypeError as exc:
            getLogger(__name__ + '.service').error(exc, exc_info=True)
            # For debugging purposes put exception to raw field, we'll be able to find it
            # in CH later
            raw = json.dumps({'exception': str(exc)})

        # https://st.yandex-team.ru/MDB-10509#602268fdbbe61e40a80dece9
        ret = dict(
            ts=int(data['ts'].replace(tzinfo=datetime.timezone.utc).timestamp()),

            user=data.get('user', ''),

            ns=data.get('ns', ''),
            database=ns[0],
            collection=ns[1],
            op=data['op'],
            form_hash=form_hash,
            form=json.dumps(form, sort_keys=False),

            duration=data['millis'],
            plan_summary=data.get('planSummary', ''),
            response_length=data.get('responseLength', 0),
            keys_examined=data.get('keysExamined', 0),
            docs_examined=data.get('docsExamined', 0),
            docs_returned=data.get('nreturned', 0),

            raw=raw,
        )
        ret.update(self.metadata)
        return json.dumps(ret, cls=MongodbJsonEncoder)


class MongodbProfilerReporter:
    """
    MongoDB system.profile reporter
    """

    def __init__(self):
        self.config = None
        self.profiler_log = None
        self.profiler_handler = None
        self.profiler_formatter = None
        self.log = None
        self._conn = None
        self._cluster_id = None
        self._shard = None
        self._hostname = socket.getfqdn()
        self._last_ts = {}
        self._cursors = {}
        self.databases = []
        self.all_databases = True
        self.exclude_dbs = ['local', 'config', 'admin', 'mdb_internal']

    def connection(self):
        """
        (Re-)init connection with database and return connection object
        """
        if self._conn:
            try:
                if self._conn.list_database_names():
                    return self._conn

                self.log.warning("MongoDB connection broken, reinitialization needed")
                self._conn.close()
            except pymongo.errors.PyMongoError as exc:
                self.log.warning(
                    "Exception during check MongoDB connection: %s",
                    exc,
                    exc_info=True,
                )
                self._conn = None

        self.log.debug("Reinitializing MongoDB connection")
        self._conn = pymongo.MongoClient(self.config['mongodb_uri'])
        return self._conn

    def get_mongo_hostname(self):
        """
        Get Hostname as MongoDB thinks about it
        """
        conn = self.connection()
        res = conn['admin'].command('isMaster').get('me')
        self.log.debug("Current MongoDB hostname is: %s", res)
        return res

    def initialize(self, config):
        """
        Init loggers and required connections
        """
        global _initialized  # pylint: disable=global-statement

        if _initialized:
            return  # pragma: nocover

        self.config = config
        self._cluster_id = self.config['cluster_id']
        self._shard = self.config['shard']

        self.all_databases = self.config.get('all_databases', True)
        self.databases = self.config.get('databases', [])
        self.exclude_dbs = self.config.get('exclude_databases', self.exclude_dbs)

        # Common logger
        self.log = getLogger(__name__ + '.service')
        self.log.propagate = False
        self.log.setLevel(DEBUG)
        service_handler = FileHandler(
            self.config['log_file'],
        )
        service_handler.setFormatter(Formatter(fmt='%(asctime)s [%(levelname)s]: %(message)s'))
        self.log.addHandler(service_handler)

        self._hostname = self.get_mongo_hostname()
        # Logger for profiler
        self.profiler_log = getLogger(__name__ + '.profiler')
        self.profiler_log.propagate = False
        self.profiler_log.setLevel(DEBUG)
        profiler_file_handler = FileHandler(
            self.config['profiler_log_file'],
        )
        self.profiler_formatter = MongodbProfilerFormatter({
            'cluster_id': self._cluster_id,
            'shard': self._shard,
            'hostname': self._hostname,
        })
        profiler_file_handler.setFormatter(self.profiler_formatter)
        self.profiler_handler = MemoryHandler(1000, 50, profiler_file_handler)
        self.profiler_log.addHandler(self.profiler_handler)
        _initialized = True

    def report(self):
        """
        Read all new entries from <db_name>.system.profile and report them
        """

        conn = self.connection()

        databases = self.databases[:]
        if self.all_databases:
            databases = [db for db in conn.database_names() if db not in self.exclude_dbs]

        for db in databases:
            self.report_db(db)

        self.profiler_handler.flush()

    def report_db(self, db):
        try:
            conn = self.connection()

            # Get Last TS first
            if db not in self._last_ts:
                last_rec = conn[db]['system.profile'].find_one(
                    projection={'ts': True},
                    limit=1,  # Not like limit really need here, but just ask MongoDB to not return
                              #  other records except of first [actually - last] one
                    sort=[('$natural', -1)],
                )
                if not last_rec or 'ts' not in last_rec:
                    # system.profile is empty or not present in this database, skip it
                    self.log.debug('Skip db %s', db)
                    return
                self._last_ts[db] = last_rec['ts']
            else:
                self._fix_rollover(db)

            # If cursor is broken, delete it (to recreate later)
            if db in self._cursors and not self._cursors[db].alive:
                self._close_cursor(db)

            # Create cursor if needed
            if db not in self._cursors:
                # Create TAILABLE cursor
                # see https://pymongo.readthedocs.io/en/stable/api/pymongo/cursor.html#pymongo.cursor.CursorType
                # and https://pymongo.readthedocs.io/en/stable/api/pymongo/collection.html#pymongo.collection.Collection.find
                # For more details
                self._cursors[db] = conn[db]['system.profile'].find(
                    filter={'ts': {'$gt': self._last_ts[db]}},
                    cursor_type=pymongo.cursor.CursorType.TAILABLE,
                )

            # For each new record, send it to log
            for rec in self._cursors[db]:
                # self.log.debug('Bup, %s', rec['ts'])
                self.profiler_log.info('', extra={'query': rec})
                if rec['ts'] > self._last_ts[db]:
                    self._last_ts[db] = rec['ts']

        except pymongo.errors.PyMongoError as exc:
            self.log.error("PyMongo Exception during read system.profile for db %s: %s", db, exc, exc_info=True)
        except timeout.FunctionTimeout as exc:
            self.log.error("Timeout reached during read system.profile for db %s: %s", db, exc, exc_info=True)

    def _close_cursor(self, db):
        if db not in self._cursors:
            return

        try:
            self._cursors[db].close()
        except pymongo.errors.PyMongoError as exc:
            self.log.warning('%s', exc, exc_info=True)
        self._cursors.pop(db, None)

    def _fix_rollover(self, db):
        now_ts = int(datetime.datetime.utcnow().timestamp())
        last_ts = int(self._last_ts[db].replace(tzinfo=datetime.timezone.utc).timestamp())

        if (now_ts - last_ts) > 60:
            conn = self.connection()
            last_rec = conn[db]['system.profile'].find_one(
                projection={'ts': True},
                limit=1,
                sort=[('$natural', 1)],
            )
            if last_rec and 'ts' in last_rec and last_rec['ts'] > self._last_ts[db]:
                # looks like there was profile rollover, just recreate cursor
                self.log.debug(
                    'Profiler rollover detected for db %s: last known ts is %s last found ts is %s',
                    db,
                    self._last_ts[db],
                    last_rec['ts'],
                )
                self._close_cursor(db)
            else:
                # no new entries in profile, just bump last_ts so we don't have to recheck it for a minute
                self._last_ts[db] = datetime.datetime.utcnow()


REPORTER = MongodbProfilerReporter()


def mdb_mongodb_perfdiag_profiler(config):
    """
    Run MongoDB system.profile reporter (use this as dbaas-cron target function)
    """
    if not _initialized:
        REPORTER.initialize(config)

    REPORTER.report()

    REPORTER.log.info('DB profiler dump successfully')

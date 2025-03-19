#! /usr/bin/env python3
"""Mongodb and mongos stats module for yandex-graphite-check-system"""

import os.path
import subprocess
import json
import time
import fcntl
from urllib.parse import quote_plus
import pymongo
import pymongo.errors

GLOBAL_LOCK_FILE = '/var/tmp/mongo-graphite-check-system.lock'

class GraphRenderer:
    """GraphRenderer"""
    CONF_FILE = '/etc/mongo-monitor.conf'
    STATS_TEMP_FILE = '/tmp/mongo-process-stats.json'
    STATS_TEMP_LOCK = '/tmp/mongo-process-stats.lock'

    def __init__(self):
        # determine username and password
        with open(self.CONF_FILE) as fin:
            username = fin.readline().strip()
            password = fin.readline().strip()

        # determine connect port
        if os.path.exists('/etc/mongodb.conf'):
            port = 27018
            self.dumpname = 'mongodb'
        elif os.path.exists('/etc/mongodbcfg.conf'):
            port = 27019
            self.dumpname = 'mongodb'
        else:
            port = 27017
            self.dumpname = 'mongos'

        uri = 'mongodb://{}:{}@127.0.0.1:{}'.format(quote_plus(username),
                                                    quote_plus(password),
                                                    port)
        self.client = pymongo.MongoClient(uri)
        self.database = self.client['admin']

        self.totals = {
            'indexSize': 0,
            'fileSize': 0,
            'storageSize': 0,
        }

    def dump(self, key: str, value: int, dumpname=True):
        """ Dumps data to the console """
        if isinstance(value, str):
            return
        if dumpname:
            print('{}.{} {}'.format(self.dumpname, key.replace(' ', '_'), value))
        else:
            print('{} {}'.format(key.replace(' ', '_'), value))

    def dump_recurse(self, key_prefix: str, data: dict, dumpname=True):
        """ Recursively dumps all subkeys """
        for key, val in data.items():
            if isinstance(val, dict):
                self.dump_recurse(key_prefix + '.' + key, val, dumpname)
            else:
                self.dump(key_prefix + '.' + key, val, dumpname)

    def dblist(self):
        """ Returns the database list except the ignored and empty ones """
        ignore_list = {'local', 'admin', 'test', 'system'}
        full_list = self.database.command('listDatabases')['databases']
        return tuple(
            x['name'] for x in full_list
            if x['name'] not in ignore_list and not x['empty']
        )

    def process_dbstats(self, dbname: str):
        """ Dumps database statistics to the output and update totals """
        stat_list = ('indexSize', 'storageSize', 'fileSize', 'dataSize', 'objects')
        database = self.client[dbname]
        stats = database.command('dbstats')
        for stat in stat_list:
            if stat in stats:
                self.dump(dbname + '.' + stat, stats[stat])

        # update total statistics
        for key in self.totals:
            if key in stats:
                self.totals[key] += stats[key]

    def process_collstats(self, dbname: str):
        """ Dump collection statistics to the output """
        stat_list = ('totalIndexSize', 'storageSize', 'size', 'count', 'avgObjSize')
        database = self.client[dbname]
        for coll in database.collection_names():
            try:
                stats = database.command({'collstats': coll})
                for stat in stat_list:
                    if stat in stats:
                        self.dump(dbname + '.' + coll + '.' + stat, stats[stat])

            except pymongo.errors.OperationFailure:
                pass

    def process_replication_info(self):
        ''' Find replication lag and headroom '''
        # find oplog size in seconds
        try:
            database = self.client['local']
            oplog = database['oplog.rs']
            min_item = oplog.find().sort('$natural', 1).limit(1)[0]
            max_item = oplog.find().sort('$natural', -1).limit(1)[0]
            oplog_size = (
                max_item['ts'].as_datetime() - min_item['ts'].as_datetime() # pylint: disable=no-member
            ).total_seconds()
        except IndexError:
            oplog_size = 0

        status = self.database.command('replSetGetStatus')
        this_member = None
        primary_member = None
        # find the lag and the headroom
        try:
            for member in status['members']:
                if 'self' in member and member['self']:
                    this_member = member
                if member['stateStr'] == 'PRIMARY':
                    primary_member = member
            if this_member and primary_member:
                lag = (this_member['optimeDate'] - primary_member['optimeDate']).total_seconds()
                self.dump('replication.lag', lag)
                self.dump('replication.headroom', oplog_size - lag)
        except IndexError:
            pass

    def is_master(self):
        """ Send metric if current host is master """
        admin = self.client["admin"]
        members = admin.command('replSetGetStatus')['members']
        is_master = 0
        for member in members:
            try:
                if member['self'] and member['stateStr'] == "PRIMARY":
                    is_master = 1
            except KeyError:
                pass
        self.dump("is_master", is_master)

    def process_usage(self):
        """ Dumps the mongodb process usage """
        # Кажется, что скрипт иногда выполняется очень долго
        # и поэтому 2 инстанса рендера запустившиеся в разное время
        # перетирают файл STATS_TEMP_FILE, поэтому нужен лок на запись
        # скрипт не запущен как демон и лок освободится после завершения скрипта
        # Поэтому тут нет логики unlock-а, и логики закрытия файлов
        with open(self.STATS_TEMP_LOCK, "w") as lock_file:
            fcntl.lockf(lock_file.fileno(), fcntl.LOCK_EX)
            try:
                with open(self.STATS_TEMP_FILE, "r") as stat_file:
                    cache = json.load(stat_file)
            except Exception: # pylint: disable=broad-except
                cache = {'time': 0.0}

            pids = []
            try:
                pids = subprocess.check_output(['pgrep', 'mongo[sd]']).decode('utf-8').split()
            except subprocess.CalledProcessError:
                pass


            total_time = 0
            new_cache = {'time': time.time()}

            for pid in pids:
                try:
                    stat = open('/proc/{}/stat'.format(pid)).read().strip().split()
                    utime = int(stat[13])
                    stime = int(stat[14])
                    new_cache[pid] = utime + stime

                    if pid in cache:
                        # calculate time @ 100 clock ticks per second
                        total_time += (
                            new_cache[pid] - cache[pid]
                        ) * 0.01 / (new_cache['time'] - cache['time'])
                except FileNotFoundError:
                    pass

            json.dump(new_cache, open(self.STATS_TEMP_FILE, "w"))
        self.dump('cpuusage', total_time)

    def process_stats(self):
        """ Dumps generic stats, returns process name """
        stats = self.database.command('serverStatus')
        process_name = stats['process']
        self.dump('connections', stats['connections']['current'])
        self.dump('pagefaults', stats['extra_info']['page_faults'])
        self.dump('size.file', self.totals['fileSize'])
        self.dump('size.storage', self.totals['storageSize'])
        self.dump('size.index', self.totals['indexSize'])
        self.dump_recurse('opcounters', stats['opcounters'])
        if process_name != 'mongos':
            self.dump('queue.read', stats['globalLock']['currentQueue']['readers'])
            self.dump('queue.write', stats['globalLock']['currentQueue']['writers'])
            self.dump('active.readers', stats['globalLock']['activeClients']['readers'])
            self.dump('active.writers', stats['globalLock']['activeClients']['writers'])
            self.dump_recurse('wiredTiger', stats['wiredTiger'], False)
        return process_name

    def run(self):
        """run render"""
        dblist = self.dblist()
        for dbname in dblist:
            self.process_dbstats(dbname)
            self.process_collstats(dbname)
        self.process_usage()
        name = self.process_stats()
        if name != 'mongos':
            self.process_replication_info()
            self.is_master()


if __name__ == '__main__':
    with open(GLOBAL_LOCK_FILE, "w") as LOCK:
        try:
            # во время восстановления монги в QA этот скрипт висит
            # бесконечно долго на подключении к монге, если реплика в STARTUP2
            # Из-за этого крон начинает спавнить новые таски и в итоге
            # их спавнится 100500 и ресурсы немного заканчиваются.
            # Если предыдущий скрипт не отработал этот лок зафейлит запуск нового
            fcntl.lockf(LOCK.fileno(), fcntl.LOCK_EX | fcntl.LOCK_NB)
            GraphRenderer().run()
        except OSError:
            pass

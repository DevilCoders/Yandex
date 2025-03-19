from pymongo import MongoClient, errors, ReadPreference, read_preferences, version_tuple
import sys
import os.path
import re
from bson.json_util import loads as bson_json_loads
import socket

pymongo_version = int("{0}000".format(''.join([str(i) for i in version_tuple]))[:3])


def find_port(path):
    port = None
    PATTERN = re.compile("\s*port")

    if path == 'mongodb':
        MONGODB_PATH = '/etc/mongodb.conf'
        MONGOCFG_PATH = '/etc/mongodbcfg.conf'
        MONGODB_DEF_PATH = '/etc/mongodb-default.conf'
        port = 27017
        if os.path.exists(MONGODB_PATH):
            path = MONGODB_PATH
        elif os.path.exists(MONGODB_DEF_PATH):
            path = MONGODB_DEF_PATH
        elif os.path.exists(MONGOCFG_PATH):
            path = MONGOCFG_PATH
            port = 27019
        else:
            return None
        SHARDSVR = re.compile("shardsvr")
        for line in open(path, 'r'):
            if SHARDSVR.search(line):
                port = 27018

    elif path == 'mongos':
        MONGOS_PATH = '/etc/mongos.conf'
        if os.path.exists(MONGOS_PATH):
            path = MONGOS_PATH
            port = 27017
        else:
            return None
    elif path == 'mongocfg':
        MONGOCFG_PATH = '/etc/mongodbcfg.conf'
        if os.path.exists(MONGOCFG_PATH):
            path = MONGOCFG_PATH
            port = 27019
        else:
            return None
    else:
        raise Exception("Wrong script parameter: " + path)
    for line in open(path):
        matcher = PATTERN.match(line)
        if matcher:
            line = line.strip()
            PORT_PATTERN = re.compile("\d+")
            port = PORT_PATTERN.search(line).group()
            break
    return int(port)


def connect(port = 27017, host = 'localhost', critical = False, code = None, slaveOk = False):
    if code == None:
        if critical:
            error_code = 2
        else:
            error_code = 0
    else:
        error_code = code

    # creating dict for connection options and adding serverSelectionTimeoutMS in case of pymongo >= 3
    opts = {}
    if version_tuple[0] >= 3:
        pymongo_base_opts = {'serverSelectionTimeoutMS': 5000}
    else:
        pymongo_base_opts = {}
    opts.update(pymongo_base_opts)

    try:
        connection = None

        if slaveOk:
            pymongo_slave_opts = {'read_preference': read_preferences.ReadPreference.SECONDARY}
        else:
            pymongo_slave_opts = {}

        opts.update(pymongo_slave_opts)
        connection = MongoClient(["{host}:{port}".format(host=host, port=port)], **opts)
        # getting info from server to check if it is alive
        connection.server_info()

        return connection['admin']

    except:
        exception = sys.exc_info()
        print("%d;Failed;Exception %s: %s" % (error_code, exception[0].__name__, exception[1]))
        raise Exception("Couldn't connect")


def check_alive(port):
    try:
        connect(port=port, critical = True)
        print("0;Ok;Ok")
    except:
        return


def get_sibling_db(db, name):
    if pymongo_version >= 281:
        sibling_database = db.client[name]
    else:
        sibling_database = db.connection[name]
    return sibling_database

def recovery_check(port, user, password):
    try:
        db = connect(port = port)
    except:
        return
    (status, master, self) = getRSStatus(host='localhost', port=port, user=user, password=password)
    if status == 'no_rs':
        print "0;Ok; standalone server"
        exit()

    if self['state'] == 3 and self['uptime'] > 3600:
        print "2;Error;Mongo state is %s too long" % self['stateStr']
        exit()
    else:
        print "0;Ok;Mongo state is %s" % self['stateStr']
        exit()

def check_pool(port, user, password):

    (status, master, self) = getRSStatus(host='localhost', port=port, user=user, password=password)
    if status == 'error':
        exit()
    if status != 'no_rs' and self['state'] in (0, 3, 5, 9):
        print "1;Warning;Mongo state is %s, check, whether it is expected behaviour" % self['stateStr']
        exit()

    try:
        db = connect(port = port)
    except:
        return
    if user and password:
        db.authenticate(user, password)
    try:
        command = db.command('serverStatus')
        connections = command['connections']['available']
        """ensure we can get the databases list(CADMIN-4564)"""
        db.client.database_names()
    except errors.OperationFailure as e:
        print "2;Failed;" + e[0] + "; maybe you have lost a config file /etc/mongo-monitor.conf with user and password information"
        return

    if connections > 100:
        print "0;Ok;%d" % connections
    else:
        print "2;Failed;FreePool = %d" % connections


def getRSStatus(host = 'localhost', port = 27017, user= None, password = None):
    try:
        db = connect(port = port, host = host)
    except:
        return ('error', None, None)
    if user and password:
        db.authenticate(user, password)
    try:
        command = db.command('replSetGetStatus')
    except errors.OperationFailure as e:
        if re.compile("(not running with --replSet|through mongos|no replset config has been received)").search(e[0]):
            return('no_rs', None, None)
        else:
            return ('error', None, None)

    master = None
    self = None
    for host in command['members']:
        if host['stateStr'] == 'PRIMARY':
            master = host

        if 'self' in host and host['self']:
            self = host
    return ('ok', master, self)

def check_rs(port, user, password, maxLag):


    (result, master, self) = getRSStatus(host='localhost', port=port, user=user, password=password)
    if result == 'error':
        return
    if result == 'no_rs':
        print "0;Ok;Replica set is not configured"
        return

    if not master:
        print "2;Failed;Could not find a PRIMARY node"
        return

    if not self:
        print "2;Failed;Could not find self"
        return
    state = self['state']
    if state in (1, 2, 7):
        dt = (master['optimeDate'] - self['optimeDate'])
        lag = dt.days * 86400 + dt.seconds
        if int(lag) < int(maxLag):
            print "0;Ok;Lag=%d" % lag
        else:
            print "2;Failed;Instance is lagging, lag=%d seconds" % lag
        return
    elif state in (0, 3, 5, 9):
        print "1;Warning;Mongo state is %s, check, whether it is expected behaviour" % self['stateStr']
        return
    else:
        print "2;Failed;Mongo state is %s, unexpected behaviour" % self['stateStr']


def getServerStatus(host = 'localhost', port = '27017', user = None,  password = None, instance = None):

    ret = {}
    serverStatus = None
    serverCmdLine = None

    try:
        db = connect(port=port, host=host, slaveOk=True)
    except:
        exit(0)
    if user and password:
        db.authenticate(user, password)

    try:
        serverStatus = db.command('serverStatus')
        serverCmdLine = db.command('getCmdLineOpts')
    except errors.OperationFailure as e:
        print "2;Failed to get server status;" + e[0]
        exit()

    def getParams(*args, **kvargs):
        name = kvargs.get('origname') or "_".join(args)

        try:
            tmp = serverStatus
            for key in args:
                tmp = tmp[key]

            ret[name] = tmp
        except:
            # Set missing metric to 0
            ret[name] = 0

    def getParamsForStat(stat):
        try:
            ret[stat] = serverStatus[stat]
        except:
            pass

    if instance == "mongodb":
        rsStatus = None
        totalIndexSize = 0
        totalFileSize = 0
        totalDataSize = 0
        totalMemory = 0
        ret['index_count'] = {}

        (status, master, self) = getRSStatus(host=host, port=port, user=user, password=password)
        if status == 'error':
            exit()
        if status != 'no_rs' and self['state'] in (0, 3, 5, 9):
            print "1;Warning;Mongo state is %s, check, whether it is expected behaviour" % self['stateStr']
            exit()



        #disk usage:
        #while true; do a1=$(a); sleep 1; a2=$(a); echo $[a2-a1]; done^C
        #function a() { iostat -d -k | grep md2 | awk '{print $5}'; }
        meminfo = open('/proc/meminfo', 'r')
        for s in meminfo:
            [key, value] = s.split(':')
            if key == 'MemTotal':
                totalMemory = int(value.strip().split(' ')[0])*1024

        meminfo.close()



        if ('replSet' in serverCmdLine['parsed'] or 'replication' in serverCmdLine['parsed']):
            try:
                rsStatus = db.command('replSetGetStatus')
            except errors.OperationFailure as e:
                print "2;Failed to get RS status;" + e[0]
                exit()
        else:
            rsStatus = None

        ret['replSet'] = (rsStatus if rsStatus else {})
        ret['fqdn'] = (socket.getfqdn() if host == 'localhost' else host)
        ret['databases'] = {}

        try:
            dbs = db.command('listDatabases')['databases']
            for d in dbs:
                sibling_db = get_sibling_db(db, d['name'])
                dbstats = sibling_db.command('dbstats')
                ret['databases'][d['name']] = dbstats
                ret['index_count'][d['name']] = dbstats['indexes']
                #print "db {} indexSize {} FileSize {}".format(d['name'], dbstats['indexSize'], dbstats['fileSize'])
                totalIndexSize += dbstats['indexSize']
                if 'fileSize' not in dbstats:
                    totalFileSize = None
                elif totalFileSize is not None:
                    totalFileSize += dbstats['fileSize']
                totalDataSize += dbstats['dataSize']
        except errors.OperationFailure as e:
            print "2;Failed to get DB stats;" + e[0]
            exit()

        ret['is_master'] = int(db.command('isMaster')['ismaster'])
        ret['master_present'] = 0
        ret['indexes_to_memory_ratio'] = float(totalIndexSize) / float(totalMemory)

        if totalFileSize and totalDataSize:
            ret['fragmentation'] = float(totalFileSize) / float(totalDataSize)
        else:
            ret['fragmentation'] = 0

        mongo26 = re.match('^2\.6', serverStatus['version']) != None
        mongo_modern = True if serverStatus['version'].split('.') >= 3 else False
        if mongo26 or mongo_modern:
            ret['dbpath'] = serverCmdLine['parsed']['storage']['dbPath']
        else:
            ret['dbpath'] = serverCmdLine['parsed']['dbpath']

        if rsStatus:
            for member in rsStatus['members']:
                if member['state'] == 1:
                    ret['master_present'] = 1
        else:
            ret['master_present'] = 1


        getParamsForStat('wiredTiger')


        getParams('asserts','msg',origname='asserts_msg')
        getParams('asserts','regular',origname='asserts_regular')
        getParams('asserts','rollovers',origname='asserts_rollovers')
        getParams('asserts','user',origname='asserts_user')
        getParams('asserts','warning',origname='asserts_warning')
        getParams('backgroundFlushing','average_ms',origname='backgroundFlushing_average_ms')
        getParams('backgroundFlushing','flushes',origname='backgroundFlushing_flushes')
        getParams('backgroundFlushing','last_ms',origname='backgroundFlushing_last_ms')
        getParams('backgroundFlushing','total_ms',origname='backgroundFlushing_total_ms')
        getParams('connections','available',origname='connections_available')
        getParams('connections','current',origname='connections_current')
        getParams('cursors','clientCursors_size',origname='cursors_clientCursors_size')
        getParams('cursors','timedOut',origname='cursors_timedOut')
        getParams('cursors','totalOpen',origname='cursors_totalOpen')
        getParams('dur','commits',origname='logging_commits')
        getParams('dur','commitsInWriteLock',origname='logging_commits_in_writelock')
        getParams('dur','earlyCommits',origname='logging_early_commits')
        getParams('dur','journaledMB',origname='logging_journal_writes_mb')
        getParams('dur','timeMs','prepLogBuffer',origname='logging_log_buffer_prep_time_ms')
        getParams('dur','timeMs','writeToDataFiles',origname='logging_datafile_write_time_ms')
        getParams('dur','timeMs','writeToJournal',origname='logging_journal_write_time_ms')
        getParams('dur','writeToDataFilesMB',origname='logging_datafile_writes_mb')
        getParams('extra_info', 'heap_usage_bytes', origname='extra_info_heap_usage')
        getParams('extra_info','page_faults',origname='extra_info_page_faults')
        getParams('globalLock','activeClients','readers',origname='globalLock_activeClients_readers')
        getParams('globalLock','activeClients','total',origname='globalLock_activeClients_total')
        getParams('globalLock','activeClients','writers',origname='globalLock_activeClients_writers')
        getParams('globalLock','currentQueue','readers',origname='globalLock_currentQueue_readers')
        getParams('globalLock','currentQueue','total',origname='globalLock_currentQueue_total')
        getParams('globalLock','currentQueue','writers',origname='globalLock_currentQueue_writers')
        getParams('globalLock','lockTime',origname='globalLock_lockTime')
        getParams('indexCounters','accesses',origname='indexCounters_btree_accesses')
        getParams('indexCounters','hits',origname='indexCounters_btree_hits')
        getParams('indexCounters','missRatio',origname='indexCounters_btree_missRatio')
        getParams('indexCounters','misses',origname='indexCounters_btree_misses')
        getParams('indexCounters','resets',origname='indexCounters_btree_resets')
        getParams('mem','bits',origname='mem_bits')
        getParams('mem','resident',origname='mem_resident')
        getParams('mem','virtual',origname='mem_virtual')
        getParams('network','bytesIn',origname='network_inbound_traffic')
        getParams('network','bytesOut',origname='network_outbound_traffic')
        getParams('network','numRequests',origname='network_requests')
        getParams('opcounters','command',origname='opcounters_command')
        getParams('opcounters','delete',origname='opcounters_delete')
        getParams('opcounters','getmore',origname='opcounters_getmore')
        getParams('opcounters','insert',origname='opcounters_insert')
        getParams('opcounters','query',origname='opcounters_query')
        getParams('opcounters','update',origname='opcounters_update')
        getParams('uptime',origname='uptime')
        getParams('version',origname='mongo_version')
        getParams('writeBacksQueued',origname='write_backs_queued');
        ret['resident_to_total_memory_ratio'] = float(ret['mem_resident']*1024*1024)/float(totalMemory)
        ret['extra_info_heap_usage'] = round(ret['extra_info_heap_usage']/(1024*1024), 2) ;


        return ret

    elif instance == "mongos":
        getParams('uptime',origname='uptime')
        getParams('mem','bits',origname='mem_bits')
        getParams('mem','resident',origname='mem_resident')
        getParams('mem','virtual',origname='mem_virtual')
        getParams('opcounters','insert',origname='opcounters_insert')
        getParams('opcounters','query',origname='opcounters_query')
        getParams('opcounters','update',origname='opcounters_update')
        getParams('opcounters','delete',origname='opcounters_delete')
        getParams('opcounters','getmore',origname='opcounters_getmore')
        getParams('opcounters','command',origname='opcounters_command')
        getParams('connections','current',origname='connections_current')
        getParams('connections','available',origname='connections_available')
        getParams('extra_info','page_faults',origname='extra_info_page_faults')
        getParams('sharding','lastSeenConfigServerOpTime','t',origname='lastseenconfigserveroptime_t')
        return ret

def loadServerStatus(file):
    try:
        f = open(file, 'r')
        serverStatus = bson_json_loads(f.read())
    except:
        f.close()
        return {}

    f.close()
    return serverStatus

def flushServerStatus(port, user, password, instance, state_file):
    if os.path.exists(state_file):
        os.unlink(state_file)
    serverStatus = {}
    serverStatus['current'] = getServerStatus(port=port, user=user, password=password, instance=instance)
    serverStatus['prev'] = {}
    for p in serverStatus['current']:
        serverStatus['prev'][p] = serverStatus['current'][p]
    return serverStatus

def outputResult(param, current, prev, threshold, critical='True'):
    failed_msg = 'Failed' if critical else 'Warning'
    if threshold == 'diff':
        if current != prev:
            print "2;Failed;{0} changed from {1} to {2}".format(param, current, prev)
            exit(0)
        else:
            print "0;OK;{0} not changed".format(param)
    else:
        op = threshold[0]
        value = float(threshold.strip('<=>'))
        if op == '>' and current <= value:
            print "2;{0};{1} = {2} < {3}".format(failed_msg, param, current, value)
        elif op == '<' and current >= value:
            print "2;{0};{1} = {2} > {3}".format(failed_msg, param, current, value)
        elif op == '=' and current != value:
            print "2;{0};{1} = {2} != {3}".format(failed_msg, param, current, value)
        else:
            print "0;OK;{0} is {1}".format(param, current)

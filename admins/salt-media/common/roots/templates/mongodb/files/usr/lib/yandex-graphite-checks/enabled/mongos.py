#!/usr/bin/env python

import pymongo
from bson.json_util import loads as bson_json_loads
import pprint
import sys
import lockfile
import time
import stat
import socket

sys.path.append("/usr/lib")
from mongodb_health.basecheck import *

# mute mongodb.py:76: UserWarning: listdatabases does not support SECONDARY
sys.stderr = open("/dev/null")

lock = lockfile.FileLock("/tmp/mongos-basecheck-graphite")
try:
    lock.acquire(timeout=10)
except lockfile.AlreadyLocked:
    sys.exit(1)
except lockfile.LockTimeout:
    mtime = os.stat(lock.lock_file).st_mtime
    if mtime < (time.time() - 600):
        lock.break_lock()
        lock.acquire(timeout=10)
    else:
        sys.exit(1)
except lockfile.LockFailed:
    sys.exit(1)

__MONGO_MONITOR = "/etc/mongo-monitor.conf"
user = None
password = None
prefix = None
if os.path.exists(__MONGO_MONITOR):
    filestat = os.stat(__MONGO_MONITOR)
    if stat.S_IMODE(filestat.st_mode) == 384 and filestat.st_gid == 0 and filestat.st_uid == 0:
        file = open(__MONGO_MONITOR)
        user = file.readline().strip()
        password = file.readline().strip()
        prefix = file.readline().strip()
    else:
        sys.stderr.write("Failed;set root:root ownership and 600 permissions to file /etc/mongo-monitor.conf\n")
        lock.release()
        exit()

with open('/var/spool/monrun/mongos-health.state') as stat_file:
    stat = bson_json_loads(stat_file.read())


port = find_port('mongos')
mongos_db = connect(port = port)
if user and password:
	mongos_db.authenticate(user, password)


data = {}

data['mongos.uptime'] = stat['current']['uptime']
data['mongos.extra_info.page_faults'] = stat['current']['extra_info_page_faults']
data['mongos.mem.bits'] = stat['current']['mem_bits']
data['mongos.mem.resident'] = stat['current']['mem_resident']
data['mongos.mem.virtual'] = stat['current']['mem_virtual']
data['mongos.opcounters.insert'] = stat['current']['opcounters_insert']
data['mongos.opcounters.query'] = stat['current']['opcounters_query']
data['mongos.opcounters.update'] = stat['current']['opcounters_update']
data['mongos.opcounters.delete'] = stat['current']['opcounters_delete']
data['mongos.opcounters.getmore'] = stat['current']['opcounters_getmore']
data['mongos.opcounters.command'] = stat['current']['opcounters_command']
data['mongos.connections.available'] = stat['current']['connections_available']
data['mongos.connections.current'] = stat['current']['connections_current']
data['mongos.opcounters.command'] = stat['current']['opcounters_command']

if prefix:
    timestamp = int(time.time())
    hostname = socket.getfqdn().replace(".", "_")
    for (k,v) in data.items():
        print "{3}.{4}.{0} {1} {2}".format(k,v,timestamp,prefix,hostname)
else:
    for (k,v) in data.items():
        print "{0} {1}".format(k,v)

lock.release()

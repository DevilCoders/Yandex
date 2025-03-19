# -*- coding: utf-8 -*-

import sys
import pymongo
from pymongo import errors
import basecheck

def getDBIndexes(db):
    ret = {}
    for d in db.command('listDatabases')['databases']:
        if ['config', 'local', 'test', 'admin'].count(d['name']) != 0:
            continue
        sibling_db = basecheck.get_sibling_db(db, d['name'])
        try:
            dbstats = sibling_db.command('dbstats')
            if dbstats['collections'] == 0:
                # skip empty DBs
                continue

            if not sibling_db.collection_names(False):
                # skip DBs without user collections
                continue

        except errors.OperationFailure as e:
            print "2;Failed to get DB stats;" + e[0]
            sys.exit(0)
        ret[d['name']] = dbstats['indexes']
    return ret


def check(databases, port = 27017, user = None, password = None):
    indexes = {}

    try:
        db = basecheck.connect(port, code = 1, slaveOk = True)
    except:
        return None

    if user and password:
        try:
            db.authenticate(user, password)
        except:
            return "1;Auth failed"

    try:
        serverCmdLine = db.command('getCmdLineOpts')
    except errors.OperationFailure as e:
        return "2;Failed to get server status; {}".format(e[0])

    if not ('replSet' in serverCmdLine['parsed'] or 'replication' in serverCmdLine['parsed']):
        return "0; Replica set is not configured"

    try:
        rs = db.command('replSetGetStatus')
    except pymongo.errors.OperationFailure as e:
        return "2;Failed to get RS staus; {}".format(e[0])

    # 1 -- PRIMARY, 2 -- SECONDARY, 7 -- ARBITER
    # for other states check https://docs.mongodb.com/manual/reference/replica-states/
    if rs['myState'] == 1:
        return "0;OK;localhost is the primary in replica set"
    elif rs['myState'] == 7:
        return "0;OK;localhost is an arbiter in replica set"
    elif rs['myState'] != 2:
        return "1;Failed;localhost is in state {0}".format(rs['myState'])

    indexes['localhost'] = getDBIndexes(db)

    try:
        master = filter(lambda x: x['stateStr'] == 'PRIMARY', rs['members'])[0]
        master_url = master['name'].split(':')
    except IndexError:
        return "1;Failed;Replica set has no PRIMARY"

    try:
        master_db = basecheck.connect(port=int(master_url[1]), host=master_url[0], code=1, slaveOk=True)
    except:
        return "1;Failed to connect to the primary"

    if user and password:
        try:
            master_db.authenticate(user, password)
        except:
          return "1;Auth failed"

    indexes[master_url[0]] = getDBIndexes(master_db)

    if databases == '__all__':
        dbs = indexes[master_url[0]].keys()
    else:
        dbs = [databases]

    try:
        if not reduce(lambda ok,db: ok and indexes['localhost'][db] == indexes[master_url[0]][db], dbs, True):
            return "2;Failed;indexes count mismatch at localhost"
    except KeyError as e:
        return "2;Failed;Missing database {} at localhost".format(e)

    return "0;OK;indexes count match"

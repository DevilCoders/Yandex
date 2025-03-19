# -*- coding: utf-8 -*-

import logging
from collections import Counter
import pymongo
from pymongo import errors
import basecheck


def get_rs_status(port = 27017, user = None, password = None):
    try:
        db = basecheck.connect(port, code = 1, slaveOk = True)
    except:
        return None

    if user and password:
        try:
            db.authenticate(user, password)
        except:
            print "1;Auth failed"
            exit()

    try:
        serverCmdLine = db.command('getCmdLineOpts')
    except errors.OperationFailure as e:
        print "2;Failed to get server status;" + e[0]
        exit()

    if not ('replSet' in serverCmdLine['parsed'] or 'replication' in serverCmdLine['parsed']):
        print "0; Replica set is not configured"
        exit()

    try:
        rs = db.command('replSetGetStatus')
    except pymongo.errors.OperationFailure as e:
        print "2;Failed to get RS status;" + e[0]
        exit()

    return rs


def get_rs_conf(port = 27017, user = None, password = None):
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
        print "2;Failed to get server status;" + e[0]
        exit()

    if not ('replSet' in serverCmdLine['parsed'] or 'replication' in serverCmdLine['parsed']):
        print "0; Replica set is not configured"
        exit()

    try:
        rs = db.command('replSetGetConfig')
    except pymongo.errors.OperationFailure as e:
        print "2;Failed to get RS conf;" + e[0]
        exit()

    return rs


def rs_secondary_alive(port = 27017, user = None, password = None, secondary_alive_conf=None):
    rs_status = get_rs_status(port, user, password)
    rs_conf = get_rs_conf(port, user, password)
    rs_hidden = list()
    for node in rs_conf['config']['members']:
        if node['hidden'] is True or node['arbiterOnly'] is True:
            rs_hidden.append(node['host'])

    secondary_alive = 0            
    for node in rs_status['members']:
        if node['stateStr'] == 'SECONDARY' and node['health'] == 1 and node['name'] not in rs_hidden:
            secondary_alive += 1

    if secondary_alive < secondary_alive_conf:
        return "2; Failed; Secondary alive count: {}".format(secondary_alive)
    else:
        return "0; OK; Secondary alive count: {}".format(secondary_alive)


def rs_secondary_fresh(port = 27017, user = None, password = None, secondary_fresh_conf = None, secondary_lag_conf = None):

    (status, master, self) = basecheck.getRSStatus(host='localhost', port=port, user=user, password=password)
    if status == 'error':
        return '1;Warning;Replica status: error'
    if status != 'no_rs' and self['state'] in (0, 3, 5, 9):
        return "1;Warning;Mongo state is %s, check, whether it is expected behaviour" % self['stateStr']

    rs_status = get_rs_status(port, user, password)

    secondary_fresh = 0
    for node in rs_status['members']:
        if node['stateStr'] == 'SECONDARY' and node['health'] == 1: 
            dt = (master['optimeDate'] - node['optimeDate'])
            lag = dt.days * 86400 + dt.seconds
    
            if lag < secondary_lag_conf:
                secondary_fresh += 1

    if secondary_fresh < secondary_fresh_conf:
        return "2; Failed; Secondary fresh count: {}".format(secondary_fresh)
    else:
        return "0; OK; Secondary fresh count: {}".format(secondary_fresh)


if __name__ == '__main__':
    REPLICASET_CFG = "/etc/monitoring/mongodb_replicaset.conf"
    logging.basicConfig(level=logging.DEBUG, format='%(levelname)s %(message)s')
    print 'rs_secondary_alive: ' + rs_secondary_alive(port = 27018, secondary_alive_conf = 1)
    print 'rs_secondary_fresh: ' + rs_secondary_fresh(port = 27018, secondary_fresh_conf = 1, secondary_lag_conf = 15)


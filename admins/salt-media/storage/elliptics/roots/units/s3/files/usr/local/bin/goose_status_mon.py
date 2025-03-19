#!/usr/bin/env python

import argparse
import sys
import json
import time
import urllib2
import dateutil.parser as dt

parser = argparse.ArgumentParser()
parser.add_argument('--type', dest='stat_type',
                    default='postgres', type=str)
parser.add_argument('--port', dest='tcp_port',
                    default=3355, type=int)
parser.add_argument('--location', dest='location',
                    default='stats/postgres', type=str)
args = parser.parse_args()

URI = "http://localhost:{0}/{1}".format(args.tcp_port, args.location)

def get_status_postgres(status):
    result_prefix = "0; OK"
    result = ""
    for cluster in status:
        for subcluster in cluster['clusters']:
            have_master = False
            lagged = True
            lag = 0
            nodes = subcluster['nodes']
            hostlist = ""
            for node in nodes:
                hostlist = "{0},{1}".format(hostlist, node['host'])
                if node['is_master']:
                    have_master = True
                if node['lag'] < 10:
                    lagged = False
                else:
                    if node['lag'] > lag:
                        lag = node['lag']
            if have_master  and lagged:
                result_prefix = "2; Crit,"
                result = "{1}, {0}".format(
                    result,
                    "replic in cluster {0} with cluster_id {1} at hosts {2} have too high lag: {3}".format(
                        cluster['cluster_name'],
                        subcluster['cluster_id'],
                        hostlist.strip(","),
                        lag
                    )
                )
            elif not have_master:
                result_prefix = "2; Crit,"
                result = "{1}, {0}".format(
                    result,
                    "no master in cluster {0} with cluster_id {1} at hosts {2}".format(
                        cluster['cluster_name'],
                        subcluster['cluster_id'],
                        hostlist.strip(",")
                    )
                )

            stat_sections = ("db_stats_list_consistent", "db_stats_list", "db_stats_rw", "db_stats_ro")
            for stat_section in stat_sections:
                if subcluster[stat_section]["InUse"] == subcluster[stat_section]["MaxOpenConnections"]:
                    result_prefix = "2; Crit,"
                    result = "{1}, {0}".format(
                        result,
                        "connection pool exhausted for {0} with cluster_id {1}: WaitCount is {2}".format(
                            stat_section,
                            subcluster['cluster_id'],
                            subcluster[stat_section]["WaitCount"]
                        )
                    )
    return "{0} {1}".format(result_prefix, result)

def get_status_default(status, crit_time):
    result = "0; OK"
    CRIT_TIME = crit_time
    for service in status:
        state = service['status']['state'].upper()
        if 'CRIT' in state:
            result = "2; Crit, {0}, jobid {1} have error {2}".format(service['status_name'], service['status']['job_id'], service['status']['error'])
            return result
        elif 'WARN' in state:
            result = "1; Warn, {0} jobid {1} have error {2}".format(service['status_name'], service['status']['job_id'], service['status']['error'])
        st = int(dt.parse(service['status']['reported']).strftime('%s'))
        ct = int(time.time())
        if (ct - st) > CRIT_TIME:
            result = "2; Crit, {0}, state not updated too long, last update {1}".format(service['status_name'], service['status']['reported'])
            return result
    return result

def get_status_lc(status):
    result = "0; OK"
    CRIT_TIME = 1800
    result = get_status_default(status, CRIT_TIME)
    return result

def get_status_billing(status):
    result = "0; OK"
    CRIT_TIME = 5400
    result = get_status_default(status, CRIT_TIME)
    return result

def get_status_cleanup(status):
    result = "0; OK"
    CRIT_TIME = 7200
    result = get_status_default(status, CRIT_TIME)
    return result


GET_STATUS = {
    'postgres': get_status_postgres,
    'lc': get_status_lc,
    'billing': get_status_billing,
    'cleanup': get_status_cleanup,
}

try:
    status = json.load(urllib2.urlopen(URI))
    result = GET_STATUS[args.stat_type](status)
    print(result)
    sys.exit(0)
except Exception as e:
    print("2;crit, Exception: {0}".format(str(e).replace("\n", " ")))
    sys.exit(0)


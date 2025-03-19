#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
The v4 script for reporting MongoDB slow queries to juggler
"""

import argparse
import subprocess
import sys
import pymongo # pylint: disable=import-error

TMP_FILE = '/tmp/mongodb-slow-queries-v3.count'
LOG_FILE = '/var/log/mongodb/mongodb.log'
CRD_FILE = '/etc/mongo-monitor.conf'

class MongoDBSlowQueries():
    """ The processing class """

    def __init__(self):
        self.args = None
        self.creds = None
        self.conn = None

        self.parse_args()
        self.read_creds()
        self.connect()

        if self.args.print:
            self.print_slow_queries()
            sys.exit()

        total_count = self.total_query_count()
        slow_count, collscan_count = self.slow_query_count()

        slow_limit = self.scale_limit(self.args.slow_limit, total_count)
        collscan_limit = self.scale_limit(self.args.collscan_limit, total_count)

        level = 0
        if (slow_count > slow_limit) \
           or (collscan_limit > 0 and collscan_count > collscan_limit):
            level = 2

        print("{level};{slow} slow, {collscan} collscans out of {total} total"
              .format(level=level,
                      slow=slow_count,
                      collscan=collscan_count,
                      total=total_count))

    @staticmethod
    def scale_limit(limit: str, total: int) -> int:
        """ Returns the limit scaled based on the total percentage """
        if limit and limit[-1] == '%':
            return int(float(limit[:-1]) * total/ 100)
        return int(limit)

    def parse_args(self):
        """ Parses the commandline and returns the arguments """
        parser = argparse.ArgumentParser(
            description='Report MongoDB slow queries')
        parser.add_argument('-t', '--time', default=300, type=int,
                            help='The time in ms after which the query is '
                            + 'considered to be slow')
        parser.add_argument('-p', '--print', dest='print', action='store_true',
                            help='Print slow queries and collscan requests')
        parser.add_argument('-s', '--slow-limit', default=0,
                            help='The number of queries per 5 minutes to be '
                            + 'considered critical. May be specified either as '
                            + 'a raw number or as a percentage (i.e. 0.1%%).')
        parser.add_argument('-c', '--collscan-limit', default=0,
                            help='The number of collection scans per 5 minutes '
                            + 'to be considered critical. Percentage allowed.')
        self.args = parser.parse_args()

    def read_creds(self):
        """ Reads MongoDB access credentials to a dict """
        user = None
        password = None

        try:
            crdfile = open(CRD_FILE, 'r')
            user = crdfile.readline().strip()
            password = crdfile.readline().strip()
            crdfile.close()
        except: # pylint: disable=bare-except
            pass

        self.creds = {
            'user': user,
            'password': password,
            'database': 'admin',
        }

    def connect(self):
        """ Connects to MongoDB and authenticates in it """
        conn = pymongo.MongoClient(port=27018)
        self.conn = conn[self.creds['database']]
        if self.creds['password'] is not None:
            self.conn.authenticate(self.creds['user'], self.creds['password'])

    def is_master(self) -> bool:
        """ Checks whether the connected mongodb is a master """
        return self.conn.command('ismaster')['ismaster']

    def total_query_count(self) -> int:
        """ Returns the query count for the last iteration """

        # get this count
        opcounters = self.conn.command('serverStatus')['opcounters']
        this_count = opcounters['query'] \
                     + opcounters['insert'] \
                     + opcounters['delete'] \
                     + opcounters['update']

        # get prev count
        try:
            with open(TMP_FILE) as fcount:
                prev_count = int(fcount.readline())
        except IOError:
            prev_count = 0

        # save the current count
        with open(TMP_FILE, 'w') as fcount:
            fcount.write('{}\n'.format(this_count))

        return this_count - prev_count

    def awk_command(self, print_queries):
        """
        Returns preformated awk command.
        If print_queries is True, awk will return log line to stdout
        """
        exclude = "moveChunk|command"
        if not self.is_master():
            exclude += "|aggregate"

        command = """
            timetail -t mongodb -n 300 {log_file} | \
            mawk '
            BEGIN {{
                print_slowq = "True";
                collscan = 0;
                slow = 0;
            }}
            /COLLSCAN/ {{
                collscan++;
            }}
            /COMMAND|QUERY/ {{
                if (/({exclude} local.oplog.rs command: getMore)/) {{
                    next;
                }}
                if (/ms$/ && int($NF) > {time}) {{
                    # If print_queries is True, it will print log lines
                    if (print_slowq == "{print_queries}") {{
                        print $0;
                    }} else {{
                        slow++;
                    }};
                }}
            }}
            END  {{
                if (print_slowq != "{print_queries}") {{
                    print slow, collscan;
                }}
            }}
            '
            """.format(log_file=LOG_FILE,
                       exclude=exclude,
                       print_queries="True" if print_queries else "False",
                       time=self.args.time)
        return command


    def print_slow_queries(self):
        """ Prints slow queries to stdout """
        command = self.awk_command(print_queries=True)
        result = subprocess.check_output(command, shell=True)
        print(result.decode('utf-8'))

    def slow_query_count(self) -> (int, int):
        """ Returns the slow query count and collscan counts """
        command = self.awk_command(print_queries=False)
        result = subprocess.check_output(command, shell=True)
        return tuple(int(x) for x in result.split())


if __name__ == '__main__':
    MongoDBSlowQueries()

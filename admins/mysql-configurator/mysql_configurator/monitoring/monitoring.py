# coding: utf8
"""
Main monitoring class
"""
from __future__ import print_function
import logging
import time
import datetime
import re
from collections import Counter, deque
from mysql_configurator.monitoring.juggler import Juggler


class Monitoring(object):
    """ Contains the monitoring logic """
    def __init__(self, conf):
        self.log = logging.getLogger(self.__class__.__name__)
        self.conf = conf
        self.statuses = {}
        self.check_ts = 0
        self.seen_slow_queries = None

        self.juggler = Juggler(conf)

    def set_explicit_dead(self):
        """ Run when lost connection to MySQL """
        diffs = []
        self.update_monitoring(diffs, 'ping',
                               (2, 'MySQL is not alive'))
        self.update_monitoring(diffs, 'connections',
                               (1, "Check disabled because MySQL is not alive"))
        self.update_monitoring(diffs, 'replica',
                               (1, "Check disabled because MySQL is not alive"))
        self.juggler.send(diffs, self.statuses)
        self.check_ts = 0

    def update_monitoring(self, diffs, name, data):
        """ Checks if the current state is different and reports to juggler if it is """
        if name not in self.statuses.keys():
            self.statuses[name] = data
        if self.statuses[name] != data:
            self.statuses[name] = data
        if self.statuses[name][0] != data[0]:
            diffs.append({
                'name': name,
                'status': data[0],
                'description': data[1]
            })

    def check(self, status, others, seen_crit_queries, seen_warn_queries):
        """ checks the latest data and performs dump on errors """
        conf = self.conf.limits.connections
        maximum = int(others['variables'][-1]['max_connections'])
        running = status['Threads_running'][-1]
        running_ratio = running * 100.0 / maximum

        # criticals
        if running_ratio >= conf.running.crit:
            self.dump_history('many running threads', others)

        # update monitoring state
        diffs = []
        self.update_monitoring(diffs, 'ping', (0, 'MySQL is alive'))
        self.update_monitoring(diffs, 'connections', self.check_connections(status, others))
        self.update_monitoring(diffs, 'replica', self.check_replica(others))
        if seen_crit_queries:
            self.update_monitoring(diffs,
                                   'slow_queries',
                                   (2, ', '.join(seen_crit_queries)))
        elif seen_warn_queries:
            self.update_monitoring(diffs,
                                   'slow_queries',
                                   (1, ', '.join(seen_warn_queries)))
        else:
            self.update_monitoring(diffs,
                                   'slow_queries',
                                   (0, ''))
        self.juggler.send(diffs, self.statuses)

        self.check_ts = time.time()

    def get(self, what):
        """ Returns actual data for the HTTP handle """
        if time.time() - self.check_ts > 2 * self.conf.live.interval:
            if what == 'ping':
                return (2, "MySQL is not alive")

            return (1, "Check disabled because MySQL is not alive")  # pylint: disable=no-else-return

        if not what in self.statuses:
            return (2, "Unknown check {}".format(what))

        return self.statuses[what]

    def check_connections(self, status, others):  # pylint: disable=R0911,R0914,too-many-branches,too-many-statements
        """ /mon/connections """
        conf = self.conf.limits.connections

        # calculate generic data
        maximum = int(others['variables'][-1]['max_connections'])
        connections = status['Threads_connected'][-1]
        running = status['Threads_running'][-1]

        commands = Counter()
        states = Counter()
        for proc in others['processlist'][-1]:
            if proc['Command'] not in ('Sleep', 'Binlog Dump'):
                if proc['Command'] == 'Query':
                    commands[proc['Info']] += 1
                else:
                    commands[proc['Command']] += 1
                states[proc['State']] += 1

        conn_ratio = connections * 100.0 / maximum
        running_ratio = running * 100.0 / maximum
        common_command = commands.most_common(1)[0]
        common_command_ratio = common_command[1] * 100.0 / maximum
        common_state = states.most_common(1)[0]
        common_state_ratio = common_state[1] * 100.0 / maximum

        try:
            errlog_items = others['error_log']
            assert isinstance(errlog_items, deque)
            error_log_count = len(errlog_items)
            errlog_items.clear()
        except (KeyError, AssertionError):
            error_log_count = 0

        check_statuses = [0]
        description_mapping = {
            0: 'OK',
            1: 'WARN',
            2: 'CRIT'
        }
        messages = []

        if conn_ratio >= conf.connected.crit:
            status_description = description_mapping[2]
            check_statuses.append(2)
        elif conn_ratio >= conf.connected.warn:
            status_description = description_mapping[1]
            check_statuses.append(1)
        else:
            status_description = description_mapping[0]
            check_statuses.append(0)
        messages.append('%s connected (%s)' % (connections, status_description))

        if running_ratio >= conf.running.crit:
            status_description = description_mapping[2]
            check_statuses.append(2)
        elif running_ratio >= conf.running.warn:
            status_description = description_mapping[1]
            check_statuses.append(1)
        else:
            status_description = description_mapping[0]
            check_statuses.append(0)
        messages.append('%s running (%s)' % (running, status_description))

        if common_command_ratio >= conf.paranormal.crit:
            status_description = description_mapping[2]
            check_statuses.append(2)
        elif common_command_ratio >= conf.paranormal.warn:
            status_description = description_mapping[1]
            check_statuses.append(1)
        else:
            status_description = description_mapping[0]
            check_statuses.append(0)
        messages.append('%s equal commands "%s" (%s)' %
                        (common_command[1], common_command[0][:50], status_description))

        if common_state_ratio >= conf.paranormal.crit:
            status_description = description_mapping[2]
            check_statuses.append(2)
        elif common_state_ratio >= conf.paranormal.warn:
            status_description = description_mapping[1]
            check_statuses.append(1)
        else:
            status_description = description_mapping[0]
            check_statuses.append(0)
        messages.append('%s in state %s (%s)' %
                        (common_state[0], common_state[1], status_description))

        if error_log_count >= 2:
            status_description = description_mapping[2]
            check_statuses.append(2)
        elif error_log_count >= 1:
            status_description = description_mapping[1]
            check_statuses.append(1)
        else:
            status_description = description_mapping[0]
            check_statuses.append(0)
        messages.append('%s errors in log (%s)' % (error_log_count, status_description))

        result = ", ".join(messages)
        result = re.sub(r'\s+', ' ', result)

        return (max(check_statuses), result)

    def check_replica(self, others):
        """ /mon/replica """
        status = others['slave'][-1]
        if not status:
            return (0, "This host is a replication master")

        status = status[0]

        if (status['Slave_IO_Running'] != 'Yes') or (status['Slave_SQL_Running'] != 'Yes'):
            return (2, "Replica has stopped")

        sbm = status['Seconds_Behind_Master']
        if sbm >= self.conf.limits.replication.crit:
            return (2, "Replica is lagging for %s seconds" % sbm)
        if sbm >= self.conf.limits.replication.warn:
            return (1, "Replica is lagging for %s seconds" % sbm)
        return (0, "Replica is lagging for %s seconds" % sbm)

    def dump_history(self, reason, others):
        """ Dumps history to the file """
        self.log.warning('Logging history due to %s', reason)

        maxlen = len(others['processlist'])
        if maxlen > 12:
            maxlen = 12
        with open(self.conf.alert_history.file, 'a+') as hist:
            print('===== DUMPING HISTORY (new-to-old) AT {} DUE TO {}'.format(time.ctime(), reason),
                  file=hist)
            for i in range(1, maxlen + 1):
                self.dump_processlist(-i, hist, others)
            print('== INNODB STATUS\n{}\n\n'.format(others['innodb_status'][-1]['Status']),
                  file=hist)

    @staticmethod
    def dump_processlist(offset, hist, others):
        """ Dumps the exact processlist """
        plist = others['processlist'][offset]
        innodb = dict((x['trx_mysql_thread_id'], x) for x in others['innodb_trx'][offset])

        def cutstring(line, maxlen):
            """
            cut string
            :param line:
            :param maxlen:
            :return:
            """
            if not line:
                return ''
            line = line.replace('\n', ' ')
            line = re.sub(r'\s+', ' ', line)
            if len(line) > maxlen:
                return line[:maxlen - 1] + '»'
            return line

        fmt = '{Id:10}  '
        fmt += '{Command:7}  '
        fmt += '{Trx_Time:4}  '
        fmt += '{Time:8}  '
        fmt += '{User:20}  '
        fmt += '{Host:30}  '
        fmt += '{State:30}  '
        fmt += '{Info}  '

        # prepare rows
        for row in plist:
            if row['Id'] in innodb:
                row['Trx_Time'] = int((datetime.datetime.now() -
                                       innodb[row['Id']]['trx_started']).total_seconds())
            else:
                row['Trx_Time'] = 0
            row['State'] = cutstring(row['State'], 30)
            row['Info'] = cutstring(row['Info'], 250)
            pos = row['Host'].rfind(':')
            if pos > -1:
                row['Host'] = row['Host'][:pos]
            if not row['Time']:
                row['Time'] = 0


        # display them
        print('== processlist at offset {}'.format(-offset), file=hist)
        print(fmt.format(Id='id',
                         Command='command',
                         Trx_Time='trxt',
                         Time='time',
                         State='state',
                         Info='info',
                         User='user',
                         Host='host'), file=hist)
        for row in sorted(plist, key=lambda x: -(x['Trx_Time'] * 100000000 + x['Time'])):
            if row['Command'] == 'Sleep' and row['Id'] not in innodb:
                continue
            print(fmt.format(**row), file=hist)
        print('', file=hist)

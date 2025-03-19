# coding: utf8
"""
 Class for watching mysql replication delay and close from SLB on reached limit
 input: root mysql-configurator-4 config
"""
import logging

import eventlet
import greenlet

from ..mysql import MySQL, MySQLError
from .iptruler import ZIPTRuler


def set_defaults_in_config(config):
    """
    Set defaults in replica watcher's config
    Disabled by default
    """
    if not config.rwatcher:
        config.rwatcher.processor.enable = False
    if not config.rwatcher.processor.down_on_stop:
        config.rwatcher.processor.down_on_stop = False
    if not config.rwatcher.processor.open_on_connecting:
        config.rwatcher.processor.open_on_connecting = False
    if not config.rwatcher.processor.ports:
        config.rwatcher.processor.ports = []
    if not config.rwatcher.processor.open_after_broken:
        config.rwatcher.processor.open_after_broken = False

    lvl = {
        'debug': logging.DEBUG,
        'info': logging.INFO,
        'warning': logging.WARNING,
        'error': logging.ERROR,
    }
    if not config.rwatcher.log.level:
        config.rwatcher.log.level = 'info'

    config.rwatcher.log.level = lvl[config.rwatcher.log.level]

    return config


class RWatcher(object):
    """
    Get replication data from mysql and open/close node from traffic
    Config variable monitoring.limits.replication.crit used as threshold for close replica
    Config variable monitoring.limits.replication.warn used as threshold for open replica
    """
    # pylint: disable=too-many-instance-attributes
    def __init__(self, conf):
        self.log = logging.getLogger(self.__class__.__name__)
        self.conf = set_defaults_in_config(conf)
        if not conf.rwatcher.processor.enable:
            return
        self.conn = MySQL()
        self.iptruler = ZIPTRuler(self.conf)
        self.log.setLevel(self.conf.rwatcher.log.level)
        self.close_thr = int(conf.monitoring.limits.replication.crit)
        self.open_thr = int(conf.monitoring.limits.replication.warn)
        self.down_on_stop = conf.rwatcher.processor.down_on_stop
        self.open_on_connecting = conf.rwatcher.processor.open_on_connecting
        if conf.rwatcher.processor.open_after_broken:
            self.open_after_broken = int(conf.rwatcher.processor.open_after_broken)
        else:
            self.open_after_broken = False
        self.log.debug("RWatcher config %s, close threshold: %ssec, open threshold: %ssec",
                       conf.rwatcher.toDict(),
                       conf.monitoring.limits.replication.crit,
                       conf.monitoring.limits.replication.warn)

    def get_data(self):
        """Get slave status from MySQL"""

        if not self.conn:
            try:
                self.conn = MySQL()
            except MySQLError as err:
                self.log.debug(err)
                return False

        if not self.conn.is_connected():
            try:
                if not self.conn.connect():
                    return False
            except MySQLError as err:
                self.log.debug(err)
                return False

        try:
            slave_status = self.conn.query("SHOW SLAVE STATUS")
            try:
                return slave_status[0]
            except IndexError:
                # return just True if slave status is none: slave is not configured
                return {}
        except MySQLError as err:
            self.log.warning('Error getting data: %s', err)
            self.log.info('Disconnected from MySQL')
            self.conn = None
            return None

    def detetect_replica_problem(self):
        """
        Detect 1 of 3 types of problems with replica:
            1. Slave is stopped
            2. Slave is lagging
            3. Replication error
        :return: bool: If replica has problems -> True; else -> False
        """
        # pylint: disable=E0602
        has_problem = 0
        status = self.get_data()
        sec_behind = status['Seconds_Behind_Master']
        is_slave_io = status['Slave_IO_Running']
        is_slave_sql = status['Slave_SQL_Running']
        last_sql_errno = status['Last_SQL_Errno']
        last_io_errno = status['Last_IO_Errno']

        # Slave is stopped
        if sec_behind is None or is_slave_io != 'Yes' or is_slave_sql != 'Yes':
            has_problem = 1
            # Push state about problem to ZK
            self.iptruler.set_node_problems_state(has_problem)
            return
        # Slave has errors
        if last_io_errno != 0 or last_sql_errno != 0:
            has_problem = 1
            # Push state about problem to ZK
            self.iptruler.set_node_problems_state(has_problem)
            return
        # Slave is lagging heavily
        if long(sec_behind) > self.close_thr:
            has_problem = 1
            # Push state about problem to ZK
            self.iptruler.set_node_problems_state(has_problem)
            return
        # No problems
        has_problem = 0
        # Push state about problem to ZK
        self.iptruler.set_node_problems_state(has_problem)

    def detect_cluster_critical(self):
        """
        If cluster is in critical situation (>= $self.open_after_broken replicas have problems),
        return True. Else False.
        """
        self.detetect_replica_problem()
        nodes_with_problems = self.iptruler.get_cluster_problems_state()
        self.log.info('Nodes with problems: %s', str(nodes_with_problems))
        self.log.info('open_after_broken %s', str(self.open_after_broken))
        self.log.info('iptruler.global_cluster_size %s', str(self.iptruler.global_cluster_size))
        max_broken_replicas = int(self.open_after_broken * self.iptruler.global_cluster_size / 100)
        if nodes_with_problems >= max_broken_replicas:
            return True
        return False

    def process(self):
        # pylint: disable=too-many-return-statements, too-many-branches, too-many-statements, E0602
        """
        main processing data function, that decides what to do(close, open, nothing)
        :return: None
        """
        # get all information at once
        status = self.get_data()
        self.log.debug('Slave status: %s', status)

        if status is None:
            self.log.info('Can\'t get slave status; skip processing.')
            return None
        self.iptruler.get_closed_state()
        closed = self.iptruler.is_closed
        can_close = self.iptruler.get_cluster_state()
        if self.iptruler.admin_close:
            self.iptruler.set_node_state()
            self.log.debug('Node closed by iptruler. Skip processing.')
            return None

        # Check for no slave info (slave is not configured)
        if not status:
            # update state even if slave not configured(for example host is master)
            if closed:
                self.iptruler.open()
                self.log.info("Slave is not configured on me. My FW chain flushed.")
            else:
                self.log.info("Slave is not configured on me. Hope that\'s ok")
            return None

        if self.open_after_broken:
            try:
                # if >= $self.open_after_broken replicas have problems, then open everything and
                # do not close until there is < $self.open_after_broken replicas have problems
                if self.detect_cluster_critical():
                    # Force open replica if whole cluster in bad state
                    self.log.info('Cluster situation is CRITICAL, should force open this node.')
                    self.iptruler.open()
                    return
            except MySQLError as err:
                self.log.warning('Error getting data: %s', err)
                self.log.info('Disconnected from MySQL')
                self.conn = None
                return None

        # Check for stopped slave
        sec_behind = status['Seconds_Behind_Master']
        is_slave_io = status['Slave_IO_Running']
        is_slave_sql = status['Slave_SQL_Running']
        slave_io_state = status['Slave_IO_State']
        if sec_behind is None and is_slave_io == 'No' and is_slave_sql == 'No':
            if self.down_on_stop:
                if not closed:
                    if can_close:
                        self.log.info("Slave is stopped. Reject requests by firewall.")
                        self.iptruler.close()
                    else:
                        msg = 'Slave is stopped, but can\'t close more nodes!'
                        self.log.info(msg)
                    return None
                else:
                    if not can_close:
                        msg = 'Slave is stopped, and node closed, but can\'t close more nodes!'
                        self.log.info(msg)
                        self.iptruler.open()
                    return None
            else:
                self.log.info('Slave is stopped and closing is disabled, skip processing...')
                self.iptruler.open()
                return None

        # Check for slave errors
        if is_slave_io != 'Yes' or is_slave_sql != 'Yes':
            if is_slave_sql == 'Yes' and is_slave_io == 'Connecting' and self.open_on_connecting:
                self.log.info("Force open while slive_io in connecting state")
                self.iptruler.open()
                return
            if not closed:
                if can_close:
                    self.log.info("Slave is not running: Slave_IO_State: '%s', "
                                  "Slave_IO_Running: '%s', Slave_SQL_Running: '%s'. "
                                  "Reject requests by firewall.",
                                  slave_io_state, is_slave_io, is_slave_sql)
                    self.iptruler.close()
                else:
                    self.log.info("Slave is not running: Slave_IO_State: '%s', "
                                  "Slave_IO_Running: '%s', Slave_SQL_Running: '%s'. "
                                  "But can't close more nodes!",
                                  slave_io_state, is_slave_io, is_slave_sql)
                return None
            else:
                if not can_close:
                    self.log.info("Slave is not running: Slave_IO_State: '%s', "
                                  "Slave_IO_Running: '%s', Slave_SQL_Running: '%s'. "
                                  "And node closed, but can't close more nodes!",
                                  slave_io_state, is_slave_io, is_slave_sql)
                    self.iptruler.open()
                return None

        # Check for replication delay
        behind = long(status['Seconds_Behind_Master'])
        if behind > self.close_thr:
            if not closed:
                if can_close:
                    self.iptruler.close()
                    msg = "Reached replication delay: {}sec. Slave is closed.".format(behind)
                    self.log.info(msg)
                else:
                    msg = 'Critical replication delay reached, but can\'t close more nodes!'
                    self.iptruler.open()
                    self.log.info(msg)
        elif behind < self.open_thr:
            if closed:
                self.iptruler.open()
                msg = "Reached replication open delay: {}sec. Flush MYSQLCONF chain.".format(behind)
                self.log.info(msg)
        else:
            self.log.debug("Reached replication delay: %ssec. Open threshold: %ss",
                           behind, self.open_thr)

        if not closed and not self.iptruler.admin_close:
            self.iptruler.set_node_state()

        return None

    def run(self):
        """process data while process will not be ended"""
        self.log.info('Start replica watcher...')
        self.log.info(
            'Replica watcher %s in config replica-watcher/main.yaml',
            'enabled' if self.conf.rwatcher.processor.enable else 'disabled'
        )

        try:
            while True:
                if self.conf.rwatcher.processor.enable:
                    self.process()
                eventlet.sleep(self.conf.rwatcher.processor.interval)
        except greenlet.GreenletExit:  # pylint: disable=no-member
            self.log.info('Stop replica watcher.')

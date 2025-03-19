# coding: utf8
"""
Class for working with iptables like iptruler
"""
import os
import re
import logging
from json import loads
from subprocess import check_output, CalledProcessError, STDOUT
from socket import getfqdn
import requests
from kazoo.exceptions import NoNodeError
from ..zookeeper import ZK


def run_cmd(cmd):
    """
    just run comman in shell
    :param cmd: command as str
    :return: return code and message
    """
    try:
        out = check_output(cmd.split(), stderr=STDOUT)
        return 0, out.decode("utf-8")
    except (OSError, CalledProcessError) as err:
        return err.returncode, err.output


def iptruler_dump():
    """
    Get iptruler rules
    :return: str with rules
    """
    data = run_cmd('/usr/sbin/iptruler dump')[1]
    result = re.findall(r'\s(\d+|all)\sdown', str(data))
    return result


class ZIPTRuler:  # pylint: disable=old-style-class, too-many-instance-attributes, C0301
    """
    works with iptables like iptruler but independently
    knows about cluster state through zookeeper
    """
    def __init__(self, config):
        self.log = logging.getLogger(self.__class__.__name__)
        self.ip6tables = '/sbin/ip6tables -w'
        self.iptables = '/sbin/iptables -w'
        self.c_group = None
        self.ports = ['3306']
        self.is_closed = False
        self.admin_close = False
        self.global_cluster_size = 1
        self.myfqdn = getfqdn()
        self.ensure_chain()
        self.ensure_main_rule()
        self.ensure_slb()
        self.get_cluster_hosts()
        self.zk = ZK(config.zookeeper)  # pylint: disable=invalid-name
        self.close_limit = config.rwatcher.processor.close_max
        if not config.rwatcher.processor.open_after_broken:
            config.rwatcher.processor.open_after_broken = False
            self.open_after_broken = False
        else:
            self.open_after_broken = int(config.rwatcher.processor.open_after_broken)
        self.zkroot = '/monitoring/replica-watcher/' + self.c_group
        self.zk.ensure_path(self.zkroot)

        if not config.rwatcher.processor.ports:
            self.get_my_ports()
        else:
            self.ports = config.rwatcher.processor.ports

        self.log.info('Control ports by replica-watcher: %s', self.ports)

        if not config.rwatcher.processor.close_type:
            self.close_type = "slb"
        else:
            self.close_type = config.rwatcher.processor.close_type

        self.log.info("Close type: %s", self.close_type)

        # flush MYSQLCONF to prevent incorrect rules(old version, bug, etc)
        self.open()

    def get_my_ports(self):
        """
        Get mysql server ports from mysql configuration
        :return: None
        """
        my_print = '/usr/bin/my_print_defaults'
        ports = []
        if os.path.isfile(my_print):
            data = run_cmd(my_print + ' mysqld')[1]
            ports = []
            for item in re.findall(r'port\w*\s*=\s*\d+', data):
                port = str(item.split('=')[1])
                ports.append(port)

        if ports:
            self.ports = ports

    def ensure_slb(self):
        """
        iptruler reload chain SLB
        """
        try:
            _, data6 = run_cmd(self.ip6tables + ' -L SLB')
            _, data4 = run_cmd(self.iptables + ' -L SLB')
        except ValueError:
            self.log.warning("Get SLB chain data error. Reload SLB.")
            run_cmd('/usr/sbin/iptruler reload')
            return

        slb_rules6 = re.findall(r'REJECT\s', data6)
        slb_rules4 = re.findall(r'REJECT\s', data4)
        if len(slb_rules6) < 1 or len(slb_rules4) < 1:
            self.log.warning("SLB chain doesn't exists. Reload iptruler tables...")
            if os.path.isfile('/usr/sbin/iptruler'):
                run_cmd('/usr/sbin/iptruler reload')
            else:
                self.log.error("iptruler not found. Can't use SLB chains!")

    def ensure_chain(self):
        """
        ensures the chain MYSQLCONF is exists and creates it if not
        :return: bool
        """

        # now we need only return code
        rcode = run_cmd(self.ip6tables + ' -L MYSQLCONF')[0]

        if rcode == 0:
            return True
        elif rcode == 1:
            status = 0
            check = run_cmd(self.ip6tables + ' -t filter -N MYSQLCONF')[0]
            if check != 0:
                status += 1
                self.log.error('Failed to add chain for ip6tables')
            check = run_cmd(self.iptables + ' -t filter -N MYSQLCONF')[0]
            if check != 0:
                status += 1
                self.log.error('Failed to add chain for iptables')
            # failed to create chain for IP6 and ip4
            if status > 1:
                return False

            self.log.info('Created MYSQLCONF chain')
            return True
        elif rcode == 3:
            self.log.error('Can\'t control iptables: permission denied. Try to run as root.')
            return False

        self.log.error('Unknown error occured. Can\'t control iptables.')
        return False

    def ensure_main_rule(self):
        """
        ensures that MYSQLCONF rule exists in INPUT after IPTRULER
        :return: bool
        """
        rcode = run_cmd(self.ip6tables + ' -C INPUT -j MYSQLCONF')[0]
        status = 0
        if rcode != 0:
            check = run_cmd(self.ip6tables + ' -A INPUT -j MYSQLCONF')[0]
            if check != 0:
                status += 1
                self.log.error('Failed to insert main rule for ipv6')
            else:
                self.log.info('Main rule for ipv6 added.')

        rcode = run_cmd(self.iptables + ' -C INPUT -j MYSQLCONF')[0]
        if rcode != 0:
            check = run_cmd(self.iptables + ' -A INPUT -j MYSQLCONF')[0]
            if check != 0:
                status += 1
                self.log.error('Failed to insert main rule for ipv4')
            else:
                self.log.info('Main rule for ipv4 added.')

        if status > 1:
            return False

        return True

    def check_rule(self, rule, ip_ver='v6'):
        """
        check rule exists in iptables
        :param rule: rule as str
        :param ip_ver: ipv6 or ipv4
        :return: bool
        """

        # remove '-A'
        _rule = " ".join([x for x in rule.split()[1:]])

        if ip_ver == 'v6':
            check_cmd = self.ip6tables + ' -C ' + _rule
        else:
            check_cmd = self.iptables + ' -C ' + _rule

        status = run_cmd(check_cmd)[0]
        if status == 0:
            return True
        return False

    def add_rule(self, rule, ip_ver='v6'):
        """
        add in iptables
        :param rule: rule as str
        :param ip_ver: ipv6 or ipv4
        :return: bool
        """
        if ip_ver == 'v6' and self.check_rule(rule):
            return 0
        elif self.check_rule(rule, ip_ver='v4'):
            return 0

        _rule = " ".join([x for x in rule.split()[1:]])

        if ip_ver == 'v6':
            iptables_cmd = self.ip6tables + ' -A ' + _rule
        else:
            iptables_cmd = self.iptables + ' -A ' + _rule

        status, message = run_cmd(iptables_cmd)
        self.log.debug("Add rule: exec: '%s', with status '%s' and message '%s'",
                       iptables_cmd, status, message.strip())

        return status

    def get_cluster_problems_state(self):
        """
        get whole cluster problems state -- number of nodes with problems
        :return: True if closed nodes rate is not more then limit and false if else
        """
        data = self.zk.get_children(self.zkroot, include_data=True)[0]

        # if not all nodes was registered in zk, count it as if it has problems.
        msg = "global cluster size: {}".format(self.global_cluster_size)
        self.log.debug(msg)

        nodes_with_problems = 0
        if len(data) != self.global_cluster_size:
            nodes_with_problems = self.global_cluster_size - len(data)

        self.log.debug("Cluster hosts: %s, global cluster size: %s",
                       data.__str__(), self.global_cluster_size)
        for node in data:
            try:
                value = self.zk.get(self.zkroot + '/' + node + '/problems')[0]
                self.log.debug('Node %s Got value problems from ZK: %s', node, value)
            except NoNodeError:
                self.log.debug('NoNodeError for node %s (maybe missing open_after_broken in config?)', str(node))
                if self.myfqdn == node:
                    value = self.get_node_problems_state()
                else:
                    value = 0
            msg = 'Node: {} problems: {}'.format(node, value)
            self.log.debug(msg)
            if value == '1':
                nodes_with_problems += 1

        return nodes_with_problems

    def get_cluster_state(self):
        """
        get whole cluster state: number of closed nodes
        :return: True if closed nodes rate is not more then limit and false if else
        """
        data = self.zk.get_children(self.zkroot, include_data=True)[0]
        my_state = 0

        # if not all nodes was registered in zk, count it as closed
        msg = "global cluster size: {}".format(self.global_cluster_size)
        self.log.debug(msg)

        if len(data) != self.global_cluster_size:
            closed = self.global_cluster_size - len(data)
        else:
            closed = 0

        self.log.debug("Cluster hosts: %s, global cluster size: %s",
                       data.__str__(), self.global_cluster_size)
        for node in data:
            value = self.zk.get(self.zkroot + '/' + node)[0]
            msg = 'Node: {} state: {}'.format(node, value)
            self.log.debug(msg)
            if node == self.myfqdn and value == '1':
                my_state = 1
            if value == '1':
                closed += 1

        closed_ratio = closed * 100 / self.global_cluster_size

        # count potentially closed one more node, don't count self if it closed
        if my_state == 1:
            p_closed = closed
        else:
            p_closed = closed + 1
        # count potentially closed nodes count
        p_closed_ratio = p_closed * 100 / self.global_cluster_size

        if p_closed_ratio > self.close_limit:
            # can't use '%' char in logging :( https://github.com/PyCQA/pylint/issues/2614
            msg = "Closed limit {}% percent. Closed: {}%".format(self.close_limit, closed_ratio)
            self.log.debug(msg)
            return False
        if self.open_after_broken:
            msg = "Closed nodes: {}%; limit {}%; Should open after {}% is bad; Can close to {}%".format(
                closed_ratio, self.close_limit, self.open_after_broken, p_closed_ratio)
        else:
            msg = "Closed nodes: {}%; limit {}%; Can close to {}%".format(
                closed_ratio, self.close_limit, p_closed_ratio)
        self.log.debug(msg)
        return True

    def set_node_state(self):
        """
        set node state in zookeeper
        :return: None
        """
        path = self.zkroot + '/' + self.myfqdn

        if self.is_closed:
            state = '1'
        else:
            state = '0'

        if not self.zk.exists(path):
            self.zk.create(path, state)
            self.log.debug("Created node %s with state %s", path, state)
        else:
            current_state = self.get_node_state()
            if current_state != int(state):
                self.zk.set(path, state)
                self.log.debug("Updated node %s with state %s", path, state)
            else:
                self.log.debug("Node %s State is already %s", path, state)

    def get_node_state(self):
        """
        get current node state in zookeeper
        :return: state
        """
        path = self.zkroot + '/' + self.myfqdn
        if self.zk.exists(path):
            data, _ = self.zk.get(path)
            if data:
                state = int(data[0])
            else:
                state = 0
        else:
            state = 0
        return state

    def set_node_problems_state(self, has_problem):
        """
        Pushes state has_problem to ZK. If this state in ZK is the same as current,
        do not overwrite it.

        param: has_problem: int: 1=True 0=False
        set node problems state in zookeeper
        :return: None
        """
        path = self.zkroot + '/' + self.myfqdn + '/problems'

        if not self.zk.exists(path):
            self.zk.create(path, str(has_problem))
            self.log.debug("Created node %s with state problems %s", path, str(has_problem))
        else:
            current_problems_state = self.get_node_problems_state()
            if current_problems_state != has_problem:
                self.zk.set(path, str(has_problem))
                self.log.debug("Updated node %s with state problems %s", path, str(has_problem))
            else:
                self.log.debug("Node %s problem State is already %s", path, str(has_problem))

    def get_node_problems_state(self):
        """
        Get current problems state in ZK
        return: int: 1 -- has problem or 0 -- no problems
        """
        path = self.zkroot + '/' + self.myfqdn + '/problems'
        if self.zk.exists(path):
            data, _ = self.zk.get(path)
            if data:
                problems_state = int(data[0])
            else:
                problems_state = 0
        else:
            problems_state = 0
        return problems_state

    def close(self):
        """
        just exec command to close host for SLB
        :return:
        """

        if self.close_type == "slb":
            # closing rule for ipv6 from slb (tun)
            rule = '-A MYSQLCONF ! -i lo -p tcp -m multiport --dports {} -j SLB'.format(
                ','.join(self.ports))
            if self.add_rule(rule) == 0:
                self.is_closed = True

            # closing rule for ipv6 from slb (non-tun)
            rule = '-A MYSQLCONF -s fd00::/8 -d fd00::/8 ! -i lo -p tcp -m multiport --dports {}' \
                   ' -j REJECT --reject-with icmp6-port-unreachable'.format(','.join(self.ports))
            if self.add_rule(rule) == 0:
                self.is_closed = True

            # closing rule for ipv4 from slb (tun)
            rule = '-A MYSQLCONF ! -i lo -p tcp -m multiport --dports {} -j SLB'.format(
                ','.join(self.ports))
            if self.add_rule(rule, ip_ver='v4') == 0:
                self.is_closed = True

            # closing rule for ipv4 from slb (non-tun)
            rule = '-A MYSQLCONF -s 10.0.0.0/8 -d 10.0.0.0/8 ! -i lo -p tcp -m multiport ' \
                   '--dports {} -j REJECT --reject-with icmp-port-unreachable'.format(
                       ','.join(self.ports))
            if self.add_rule(rule, ip_ver='v4') == 0:
                self.is_closed = True

        elif self.close_type == "full":
            # closing rule for ipv6 from any
            rule = '-A MYSQLCONF ! -i lo -p tcp -m multiport --dports {}' \
                   ' -j REJECT --reject-with icmp6-port-unreachable'.format(','.join(self.ports))
            if self.add_rule(rule) == 0:
                self.is_closed = True

            # closing rule for ipv4 from any
            rule = '-A MYSQLCONF ! -i lo -p tcp -m multiport --dports {}' \
                   ' -j REJECT --reject-with icmp-port-unreachable'.format(','.join(self.ports))
            if self.add_rule(rule, ip_ver='v4') == 0:
                self.is_closed = True

        self.set_node_state()

    def get_closed_state(self):
        """
        check current host fw state and updates is_closed variable
        :return: None
        """

        # prevent incorrect status when chain SLB lost
        self.ensure_slb()
        # run_cmd return tuple(exit_code, message), exit code is not needed now
        _, message4 = run_cmd(self.iptables + ' -L MYSQLCONF')
        _, message6 = run_cmd(self.ip6tables + ' -L MYSQLCONF')

        # Get rules count: split by net line, cut 2 header lines, count the rest lines
        rule4_count = len(filter(None, message4.split('\n')[2:]))
        rule6_count = len(filter(None, message6.split('\n')[2:]))

        if rule6_count > 0 and rule6_count > 0:
            self.log.debug("Watcher state: closed (v4 rules: %s, v6 rules %s). "
                           "Overrides if open by iptruler", rule4_count, rule6_count)
            is_closed = True
        else:
            self.log.debug("Watcher state: opened (v4 rules: %s, v6 rules %s)."
                           "Overrides if open by iptruler", rule4_count, rule6_count)
            is_closed = False

        msg = "My state(watcher) is closed" if is_closed else "My state(watcher) is open"
        self.log.info(msg)

        iptruler_closed = iptruler_dump()
        my_ports = ['all'] + self.ports

        # Reset a variable admin_close
        self.admin_close = False

        # Scan all ports to see if there are any closed by iptruler (admin)
        for port in my_ports:
            if port in iptruler_closed:
                self.log.debug('Closed port by iptruler: %s', port)
                is_closed = True
                self.admin_close = True

        self.is_closed = is_closed

        if self.admin_close:
            msg = "My state is closed by admin! Can't flush iptruler rules!"
        else:
            msg = "My state(admin) is open"
        self.log.debug(msg)

        msg = "My total state is {}".format("closed" if self.is_closed else "open")
        self.log.info(msg)

    def open(self):
        """
        just exec command to open host for SLB
        :return:
        """
        # v6 flush
        open_cmd = self.ip6tables + ' -F MYSQLCONF'
        status = run_cmd(open_cmd)[0]
        if status == 0:
            self.is_closed = False

        # v4 flush
        open_cmd = self.iptables + ' -F MYSQLCONF'
        status, message = run_cmd(open_cmd)
        if status == 0:
            self.is_closed = False
            ret = True
            self.log.info('Rules is flushed. Node is opened.')
        else:
            ret = False
            self.log.warning('Rules is tried to flush, but with error: %s', message)

        self.set_node_state()
        return ret

    def get_cluster_hosts(self):
        """
        Determine cluster hosts by conductor group
        Init class variable self.c_group and self.cluster_size
        """
        c_url = 'https://c.yandex-team.ru/api-cached/hosts/{}?format=json'.format(self.myfqdn)

        try:
            self.c_group = loads(requests.get(c_url).content)[0]['group']
        except (requests.ConnectionError, requests.HTTPError) as err:
            self.log.error("Can't get group from conductor: %s", err)
            self.c_group = None
            return None

        self.log.debug("My group: %s", self.c_group)

        if self.c_group is not None:
            c_url = 'https://c.yandex-team.ru/api-cached/groups2hosts/{}'.format(self.c_group)
            try:
                hosts = requests.get(c_url).content.split()
                self.log.debug("Detected cluster hosts: %s", hosts)
                self.global_cluster_size = len(hosts)
            except (requests.ConnectionError, requests.HTTPError) as err:
                self.log.error("Can't get group from conductor: %s", err)
                self.global_cluster_size = 1
                return None

        else:
            self.global_cluster_size = 1

        self.log.debug("Detected cluster size: %s", self.global_cluster_size)
        return None

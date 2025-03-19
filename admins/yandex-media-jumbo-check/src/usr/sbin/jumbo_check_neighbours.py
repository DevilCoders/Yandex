#!/usr/bin/env python
""" Check for MTU and jumbo pings for hosts in required groups """

import sys
import subprocess
import socket
import re
import requests
import yaml

C_URL = "http://c.yandex-team.ru/api-cached"
MYGROUP_API_URL = C_URL + "/generator/aggregation_group?fqdn="
GROUPS2HOSTS_URL = C_URL + "/groups2hosts/"
TIMEOUT = 5

CHECK_PREFIX = "2a02:6b8::"
REQUIRED_MTU = 8900

MAX_TARGETS = 32


def read_conf(path="/etc/yandex/jumbo_checks.yaml"):
    """read yaml config"""
    try:
        with open(path) as conf_file:
            conf = yaml.safe_load(conf_file)
        return conf
    except IOError:
        return {"hosts": {}, "groups": {}}


def my_fqdn():
    """ Just use system hostname """
    return socket.getfqdn()


def my_group(fqdn=None):
    """ Resolve conductor group """
    if not fqdn:
        fqdn = my_fqdn()
    try:
        r = requests.get(MYGROUP_API_URL + fqdn, timeout=TIMEOUT)
        if r.status_code == requests.codes.ok:
            return r.text
        return None
    except requests.exceptions.RequestException:
        return None


def resolve_group(group_name):
    """ Get addresses of all my neighbors from GROUPS2HOSTS_URL """
    if not group_name:
        return []

    try:
        req = requests.get(GROUPS2HOSTS_URL + group_name, timeout=TIMEOUT)
        if req.status_code != requests.codes.ok:      # pylint: disable=E1101
            return []
        hosts_list = req.text
    except requests.exceptions.RequestException:
        return []

    return hosts_list.splitlines()


def get_target_hosts(conf=None):
    """ Check targets and resolve all groups to hosts """
    if not conf:
        conf = read_conf()

    hostname = my_fqdn()
    group = my_group()
    target_list = []
    if hostname in conf.get('hosts', {}):
        target_list = conf['hosts'][hostname]
    elif group in conf.get('groups', {}):
        target_list = conf['groups'][group]
    else:
        if group:
            target_list.append("%" + group)
        else:
            return []

    target_hosts = []
    for target in target_list:
        if target[0] == "%":
            target_hosts.extend(resolve_group(target[1:]))
        else:
            target_hosts.append(target)

    if len(target_hosts) > MAX_TARGETS:
        target_hosts = target_hosts[:MAX_TARGETS]
    return target_hosts


def parse_fping_out(stdout):
    """ Parse fping output and return dict with loss percentages """
    loss_dict = {}
    for record in stdout.splitlines():
        # print(record)
        (host, results) = record.split(" : ", 1)
        loss = filter(lambda x: x == "-", results.split(" "))
        loss_dict[host] = 100.0*len(list(loss)) / len(results.split(" "))

    return loss_dict


def fping_hosts(hosts, *args):
    """ Run fping for given hosts """
    if not hosts:
        return {}
    cmd = ["fping6", "-C4", "-q"]
    cmd.extend(args)
    cmd.extend(hosts)
    # print(cmd)
    try:
        result = subprocess.check_output(
            cmd, universal_newlines=True, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as err:
        result = err.output
    loss_dict = parse_fping_out(result)
    return loss_dict


def read_sysctl(name):
    """ Read sysctl Linux-style """
    path_name = name.replace(".", "/")
    with open("/proc/sys/" + path_name) as sysctl_file:
        result = sysctl_file.readline()
    return result


def get_yandex_route_mtu():
    """ Get MTU for CHECK_PREFIX """
    cmd = ["ip", "-6", "--oneline", "route", "get", CHECK_PREFIX]
    stdout = subprocess.check_output(cmd, universal_newlines=True)
    if "mtu" in stdout: #pylint: disable=unsupported-membership-test
        mtu = re.search(r"mtu\s+(\d+)", stdout)
        return int(mtu.group(1))
    return 0


def check_jumbo(hosts):
    """ Do all checks """
    yandex_route_mtu = get_yandex_route_mtu()
    if yandex_route_mtu == 0:
        yandex_route_mtu = int(read_sysctl("net.ipv6.conf.eth0.ra_default_route_mtu"))
    if yandex_route_mtu == 0:
        yandex_route_mtu = int(read_sysctl("net.ipv6.conf.eth0.mtu"))

    if yandex_route_mtu < REQUIRED_MTU:
        print("2;Found MTU:" + str(yandex_route_mtu) +
              ", required: " + str(REQUIRED_MTU) +
              " (try `screen -dm sudo ifup eth0` to fix)")
        return False

    # return True
    small_losses = fping_hosts(hosts, "-b56")
    jumbo_losses = fping_hosts(hosts, "-b"+str(REQUIRED_MTU-48), "-M")

    bad_hosts = []
    for (host, small_loss) in small_losses.items():
        if small_loss < 100.0 and jumbo_losses[host] == 100.0:
            bad_hosts.append(host)
    if bad_hosts:
        print("2;Jumbo frames lost for: " + ",".join(bad_hosts))
        return False
    return True


def main():
    """ Main function """
    # requests.packages.urllib3.disable_warnings()
    target_hosts = get_target_hosts()
    if not check_jumbo(target_hosts):
        sys.exit(1)
    print("0;OK")


if __name__ == "__main__":
    main()

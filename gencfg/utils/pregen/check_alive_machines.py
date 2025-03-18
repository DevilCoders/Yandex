#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import socket
from collections import defaultdict
import re

import gencfg
from core.argparse.parser import ArgumentParserExt
from core.db import CURDB
import core.argparse.types as argparse_types
from gaux.aux_multithread import run_in_multi, ERunInMulti
from gaux.aux_utils import prompt, run_command

TIMEOUT_STATUSES = [None, -9, 255]


# ======== Auxliarily stuff =================
def _gen_check_command(command_str, host, params):
    if params['mode'] == ERunInMulti.SKYNET:
        args = ["bash", "-c", command_str]
    else:
        args = ["ssh", "-o", "PasswordAuthentication=false", host, command_str]

    return args


def _show_hosts(hosts, with_groups=False, limit=100):
    if len(hosts) > limit:
        hosts = hosts[:limit]
        extra = ',...'
    else:
        extra = ''

    if with_groups:
        hosts_by_group = defaultdict(list)
        for host in hosts:
            for group in CURDB.groups.get_host_groups(host):
                if group.card.master is not None:
                    continue
                if group.card.properties.background_group:
                    continue
                if group.card.properties.fake_group:
                    continue
                hosts_by_group[group.card.name].append(host.name)
        return ','.join(map(lambda (x, y): '%s(%s)' % (x, ','.join(y) + extra), hosts_by_group.iteritems())) + extra
    else:
        return ','.join(map(lambda x: x.name, hosts)) + extra


# check if ssh port is open on dest machine
def _check_via_proto(host, proto):
    try:
        s = socket.socket(proto, socket.SOCK_STREAM)
        s.settimeout(5)
        s.connect((host, 22))

        r = s.recv(1)
        if len(r) == 1:
            return True
    finally:
        s.settimeout(None)

    return False


def check_open_ssh_port(host, params):
    del params
    try:
        return _check_via_proto(host, socket.AF_INET)
    except:
        try:
            return _check_via_proto(host, socket.AF_INET6)
        except:
            return False


# check if machine frequency is OK (minimal means something broken in rack control system)
def check_lowfreq(host, params):
    args = _gen_check_command("cat /proc/cpuinfo | grep 'cpu MHz' | awk '{print $4}'", host, params)

    status, out, err = run_command(args, sleep_timeout=0.1, timeout=params.get('timeout', 10))
    if status in TIMEOUT_STATUSES:
        raise Exception("Timed out")

    freq_list = sorted(map(lambda x: float(x), out.strip().split('\n')))
    if freq_list[-1] < 1300:
        return False
    if freq_list[len(freq_list) / 2] < 1300:
        return False

    return True


def check_lowfreq_unsafe(host, params):
    args = _gen_check_command(
        "(export HEATUP_DIR=`mktemp -d` && chmod 777 $HEATUP_DIR && cd $HEATUP_DIR && sky get -w rbtorrent:b9fb6467ea74d6bbfa6b405fb6ee3e6ec0547879 && cd heatup && ./heatup.Linux 100000 &); sleep 3; cat /proc/cpuinfo | grep 'cpu MHz' | awk '{print $4}'; killall heatup.Linux",
        host, params)

    status, out, err = run_command(args, sleep_timeout=0.1, timeout=params.get('timeout', 20))
    if status in TIMEOUT_STATUSES:
        raise Exception("Timed out")

    freq_list = sorted(map(lambda x: float(x), out.strip().split('\n')))
    if freq_list[0] < 2000:
        return False

    return True


def check_lowfreq_intel_pstate(host, params):
    args = _gen_check_command("cat /sys/devices/system/cpu/intel_pstate/max_perf_pct", host, params)
    status, out, err = run_command(args, sleep_timeout=0.1, timeout=params.get('timeout', 20))

    percents = int(out)

    if percents < 100:
        return False

    return True


def check_tcapped(host, params):
    args = _gen_check_command(
        "sudo modprobe ipmi_devintf && sudo modprobe ipmi_si && sudo ipmitool -t 0x2c -b 0x06 raw 0x2e 0xd3 0x57 0x01 0x00 0x00 | awk '{print strtonum(\"0x\"$5);}'",
        host, params)

    status, out, err = run_command(args, sleep_timeout=0.1, timeout=params.get('timeout', 10))
    if status in TIMEOUT_STATUSES:
        raise Exception("Timed out")

    if int(out) != 0:
        return False

    return True


def check_pcapped(host, params):
    args = _gen_check_command(
        "sudo modprobe ipmi_devintf && sudo modprobe ipmi_si && sudo ipmitool -t 0x2c -b 0x06 raw 0x2e 0xd3 0x57 0x01 0x00 0x00 | awk '{print strtonum(\"0x\"$4);}'",
        host, params)

    status, out, err = run_command(args, sleep_timeout=0.1, timeout=params.get('timeout', 10))
    if status in TIMEOUT_STATUSES:
        raise Exception("Timed out")

    if int(out) != 0:
        return False

    return True


def check_memleak(host, params):
    args = _gen_check_command(
        "export TOTAL=`cat /proc/meminfo | grep MemTotal | awk '{print $2 * 1024}'`; cat /proc/vmstat  | grep \"nr_inactive_anon \|nr_active_anon \|nr_anon_pages \|nr_shmem\" | xargs | awk -v TOTAL=$TOTAL '{print ($2 + $4 - $6 - $8) * 4096 / TOTAL}'",
        host, params)
    status, out, err = run_command(args, sleep_timeout=0.1, timeout=10)
    if status in TIMEOUT_STATUSES:
        raise Exception("Timed out")

    if float(out) > 0.1:
        return False
    return True


def check_fastbone(host, params):
    args = _gen_check_command("ifconfig | grep -A4 vlan | grep -c 2a02:6b8", host, params)
    status, out, err = run_command(args, raise_failed=False, sleep_timeout=0.1, timeout=10)
    if status in TIMEOUT_STATUSES:
        raise Exception("Timed out")

    return int(out) > 0


def check_xfastbone(host, params):
    args = _gen_check_command("ping6 -c 1 -W 1 kartonka.fb", host, params)
    status, out, err = run_command(args, raise_failed=False, sleep_timeout=0.1, timeout=10)
    if status in TIMEOUT_STATUSES:
        raise Exception("Timed out")

    return status == 0


def check_overheat(host, params):
    args = _gen_check_command(
        "export LAST_THROTTLE=`tail -500 /var/log/mcelog | grep -B 1 'Throttling enabled' | grep TIME | awk '{print $2}' | sort -g | tail -1`; export NOW=`date +%s`; if (( $LAST_THROTTLE + 600 >= $NOW )); then exit 1; fi",
        host, params)
    status, out, err = run_command(args, raise_failed=False, sleep_timeout=0.1, timeout=10)
    if status in TIMEOUT_STATUSES:
        raise Exception("Timed out")

    return status == 0


def check_slownet(host, params):
    args = _gen_check_command(
        r"(ethtool eth0; ethtool eth1; ethtool eth2) 2>/dev/null | grep -E '\s+Speed:\s+[0-9]+Mb\/s' | sed 's/[\t ]*Speed: \(.*\)Mb\/s/\1/'",
        host, params)
    status, out, err = run_command(args, raise_failed=False, sleep_timeout=0.1, timeout=10)
    if status in TIMEOUT_STATUSES:
        raise Exception("Timed out")

    if int(out) >= 1000:
        return True
    return False


def _get_raids_info(host, params):
    """
        Return list of disks for every raid device along with raid type

        :return (dict): raid devices info
    """

    def _to_dict(line):
        parts = re.findall('.*?=".*?"', line)
        parts = map(lambda x: x.lstrip().strip(), parts)
        result = dict(map(lambda x: (x.partition('=')[0], x.partition('=')[2].replace('"', '')), parts))
        return result

    args = _gen_check_command("lsblk -io KNAME,TYPE,SIZE,MODEL,ROTA -b -P", host, params)
    status, out, err = run_command(args, raise_failed=False, sleep_timeout=0.1, timeout=10)
    if status != 0:
        raise Exception("Got status <%s>" % status)

    raids_info = {}
    last_disk = None
    for line in out.strip().split('\n'):
        d = _to_dict(line)
        if d['TYPE'] == 'disk':
            last_disk = {'name': d['KNAME'], 'model': d['MODEL'], 'size': int(d['SIZE']), 'rota': int(d['ROTA'])}
        if d['TYPE'].startswith('raid'):
            if d['KNAME'] not in raids_info:
                raids_info[d['KNAME']] = {'raid_type': d['TYPE'], 'disks': []}

            assert (last_disk is not None)
            assert (d['TYPE'] == raids_info[d['KNAME']]['raid_type'])

            raids_info[d['KNAME']]['disks'].append(last_disk)

    return raids_info


def check_badraid10(host, params):
    raids_info = _get_raids_info(host, params)

    for raid_device in raids_info.itervalues():
        if raid_device['raid_type'] == 'raid10':
            rota_types = set(map(lambda x: x['rota'], raid_device['disks']))
            if len(rota_types) > 1:  # have both ssd and hdd in one raid10 device
                return False

    return True


def check_wrongname(host, params):
    """
        Check if contents of /etc/hosts corresponds to real host ssh address
    """
    args = _gen_check_command("cat /etc/hosts | tail -1 | awk '{print $NF}'", host, params)
    status, out, err = run_command(args, raise_failed=False, sleep_timeout=0.1, timeout=10)
    if status in TIMEOUT_STATUSES:
        raise Exception("Timed out")

    if out.strip() == host.partition('.')[0]:
        return True
    else:
        print "Host <%s> presented as <%s>" % (host, out.strip())
    return False


def check_ssdmirror(host, params):
    raids_info = _get_raids_info(host, params)

    for raid_device in raids_info.itervalues():
        if raid_device['raid_type'] in ['raid1', 'raid10']:
            ssd_count = len(filter(lambda x: x['rota'] == 0, raid_device['disks']))
            if ssd_count > 1:
                return False

    return True


def get_checkers():
    return {
        'sshport': check_open_ssh_port,
        'lowfreq': check_lowfreq,
        'lowfreq_unsafe': check_lowfreq_unsafe,
        'lowfreq_intel_pstate': check_lowfreq_intel_pstate,
        'memleak': check_memleak,
        'fastbone': check_fastbone,
        'xfastbone': check_xfastbone,
        'overheat': check_overheat,
        'slownet': check_slownet,
        'pcapped': check_pcapped,
        'tcapped': check_tcapped,
        'badraid10': check_badraid10,
        'wrongname': check_wrongname,
        'ssdmirror': check_ssdmirror,
    }


def get_parser():
    parser = ArgumentParserExt(
        description="Find working machines (via telnet to 22 port, or sky ping or via some other method)")
    parser.add_argument("-c", "--checker", type=str, dest="checker", required=True,
                        choices=get_checkers().keys(),
                        help="Obligatory. Checker, one of %s" % (','.join(get_checkers().keys())))
    parser.add_argument("-g", "--groups", dest="groups", type=argparse_types.groups,
                        help="Optional. List of groups to process")
    parser.add_argument("-m", "--mode", type=str, default=ERunInMulti.PROCESS,
                        choices=ERunInMulti.ALL,
                        help="Optional. Choose mode to run in parallel")
    parser.add_argument("-u", "--unworking-group", dest="unworking_group", type=argparse_types.group,
                        required=False, default=None,
                        help="Optional. Move unworking machines to special group")
    parser.add_argument("-r", "--add-working-to-reserved", dest="add_working_to_reserved",
                        action="store_true", default=False,
                        help="Optional. Add working hosts to reserved")
    parser.add_argument("--add-working-to-group", type=argparse_types.group, default=None,
                        help="Optional. Added all working machines to specified group")
    parser.add_argument("-n", "--non-interactive", dest="non_interactive", action="store_true", default=False,
                        help="Optional. Answer yes to all prompts")
    parser.add_argument("-w", "--workers", dest="workers", type=int, default=100,
                        help="Optional. Number of workers")
    parser.add_argument("-s", "--hosts", dest="hosts", type=argparse_types.hosts, default=None,
                        help="Optional. Host list")
    parser.add_argument("-f", "--filter", dest="flt", type=argparse_types.pythonlambda, default=None,
                        help="Optinal. Filter on hosts")
    parser.add_argument("-l", "--show-limit", dest="show_limit", type=int, default=100,
                        help="Optional. Show only first <limit> hosts")
    parser.add_argument("-t", "--timeout", type=int, default=10,
                        help="Optional. Timeout on secudtion of subfunc")
    parser.add_argument("-v", "--verbose", dest="verbose_level", action="count", default=0,
                        help="Optional. Verbose mode. Multiple -v options increase the verbosity. The maximum is 2.")

    return parser


def normalize(options):
    if options.unworking_group is not None and options.add_working_to_reserved:
        raise Exception("Options <--unworking_group group> and <--add-working-to-reserved> are mutually exclusive")

    if options.groups is None and options.hosts is None:
        raise Exception("You must specify at least one of --groups, --hosts option")

    if options.unworking_group is None and options.add_working_to_reserved == False:
        options.verbose = True

    if options.add_working_to_reserved == True and options.add_working_to_group is not None:
        raise Exception("Options <--add-working-to-reserved> and <--add-working-to-group> are mutually exclusive")


def main(options, from_cmd=False):
    hosts = []
    if options.hosts:
        hosts.extend(options.hosts)
    if options.groups:
        hosts.extend(list(set(sum(map(lambda x: x.getHosts(), options.groups), []))))
    if options.flt:
        hosts = filter(lambda x: options.flt(x), hosts)

    multi_result = run_in_multi(get_checkers()[options.checker], map(lambda x: x.name, hosts),
                                {'mode': options.mode, 'timeout': options.timeout},
                                workers=options.workers, mode=options.mode, show_progress=(options.verbose_level > 0),
                                finish_on_keyboard_interrupt=True)

    succ_hosts = map(lambda (x, y, z): x, filter(lambda (x, y, z): z is None and y == True, multi_result))
    succ_hosts = map(lambda x: CURDB.hosts.get_host_by_name(x), succ_hosts)

    failed_hosts = map(lambda (x, y, z): x, filter(lambda (x, y, z): z is None and y == False, multi_result))
    failed_hosts = map(lambda x: CURDB.hosts.get_host_by_name(x), failed_hosts)

    error_hosts = map(lambda (x, y, z): x, filter(lambda (x, y, z): z is not None, multi_result))
    error_hosts = map(lambda x: CURDB.hosts.get_host_by_name(x), error_hosts)

    if from_cmd or options.verbose_level > 0:
        print "Working hosts (%d total): %s\nUnworking hosts (%d total): %s\nError hosts (%d total): %s\n" % (
            len(succ_hosts), _show_hosts(succ_hosts, limit=options.show_limit),
            len(failed_hosts), _show_hosts(failed_hosts, limit=options.show_limit),
            len(error_hosts), _show_hosts(error_hosts, limit=options.show_limit),
        )
    if options.verbose_level > 1:
        for host, _, error_msg in filter(lambda (x, y, z): z is not None, multi_result):
            print "Host %s error message: <%s>" % (host, error_msg)

    if options.unworking_group is not None and len(failed_hosts) > 0:
        if options.non_interactive or prompt(
                        "Move hosts %s to ALL_UNWORKING?" % _show_hosts(failed_hosts, with_groups=True)):
            # check for host we can not remove (hosts in intlookups)
            failed_groups = list(set(sum(map(lambda x: CURDB.groups.get_host_groups(x), failed_hosts), [])))
            busy_hosts = list(set(sum(map(lambda x: list(x.get_busy_hosts()), failed_groups), [])) & set(failed_hosts))
            failed_hosts = list(set(failed_hosts) - set(busy_hosts))

            if options.verbose_level > 0:
                if len(busy_hosts):
                    print "Can not move to UNWORKING (busy hosts): %s" % _show_hosts(busy_hosts, with_groups=True)
                print "Move from %s to UNWORKING (%d total): %s" % (",".join(map(lambda x: x.card.name, options.groups)), len(failed_hosts), _show_hosts(failed_hosts, with_groups=True))

            CURDB.groups.move_hosts(failed_hosts, options.unworking_group)
            CURDB.update(smart=True)

    if options.add_working_to_reserved and len(succ_hosts) > 0:
        if options.non_interactive or prompt("Move hosts %s to RESERVED?" % _show_hosts(succ_hosts, with_groups=True)):
            if options.verbose_level > 0:
                print "Move from %s to RESERVED (%d total): %s" % (",".join(map(lambda x: x.card.name, options.groups)), len(succ_hosts), _show_hosts(succ_hosts, with_groups=True))
            for host in succ_hosts:
                CURDB.groups.move_host(host, CURDB.groups.get_group('%s_RESERVED' % (host.location.upper())))
            CURDB.update(smart=True)
    elif options.add_working_to_group is not None and len(succ_hosts) > 0:
        if options.verbose_level > 0:
            print "Move from %s to %s (%d total): %s" % (",".join(map(lambda x: x.card.name, options.groups)), options.add_working_to_group.name, len(succ_hosts), _show_hosts(succ_hosts, with_groups=True))
        CURDB.groups.move_hosts(succ_hosts, options.add_working_to_group)
        CURDB.update(smart=True)

    return {
        'succ_hosts': succ_hosts,
        'failed_hosts': failed_hosts,
        'error_hosts': error_hosts,
    }


def jsmain(d):
    options = get_parser().parse_json(d)
    normalize(options)
    return main(options)


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    normalize(options)
    result = main(options, from_cmd=True)

#!./venv/venv/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
sys.path.append('/skynet')

import re
import urllib2
import socket
import random
import time
import inspect
import copy
from multiprocessing.pool import ThreadPool
import traceback
import datetime
import json
import md5
import threading
from pydoc import locate
import msgpack
import logging
import collections

import api.cqueue
import library.config
from kernel.util.errors import formatException

import gencfg
import config
from gencfg0 import _packages_path

import yt.wrapper as yt
import gaux.aux_yp as aux_yp

try:
    from core.db import CURDB, init_CURDB
    from core.hosts import get_host_fields_info, Host, IS_VM_GUEST_CYTHON
    from core.instances import Instance
    from core.argparse.parser import ArgumentParserExt
    import core.argparse.types
except ImportError: # issue for skynet (can not import local .so file, when executing code on remote host)
    pass


import gaux.aux_utils
import gaux.aux_oops as aux_oops
from gaux.aux_colortext import red_text
from gaux.aux_utils import run_command, memoize_urlopen, retry_urlopen, to_datetime
from gaux.aux_mongo import get_mongo_collection
from gaux.aux_decorators import memoize
from gaux.aux_walle import Walle
import gaux.aux_decorators
import gaux.aux_storages
from config import EINE_PROP_LAN_URL
from core.settings import SETTINGS
import transliterate
import netaddr

TIMEFMT = "%Y-%m-%dT%H:%M:%SZ"
SUPPORT_UPDATE_TO_DEF_VALUE = ('gpu_count', 'gpu_models')

YT_CLUSTER = 'hahn'
HWSTAT_DIRECTORY = '//home/dca/hwstat'

class EStatuses(object):
    SUCCESS = 'SUCCESS'
    SKYNET_FAILED = 'SKYNET_FAILED'
    SKYNET_TIMEDOUT = 'SKYNET_TIMEDOUT'


@gaux.aux_decorators.Singleton
class TL3Ipv6Nets(object):
    __slots__ = ['nets', 'l3switches']
    def __init__(self):
        self.nets = dict()
        self.l3switches = set()
        for line in retry_urlopen(3, SETTINGS.services.racktables.l3nets.url).strip().split('\n'):
            switch, vlan, net_prefix = line.strip().split('\t')
            self.nets[(switch, vlan)] = net_prefix.rpartition('::')[0] + '::'
            self.l3switches.add(switch)


def geturl(url, timeout=10):
    try:
        oldtimeout = socket.getdefaulttimeout()
        socket.setdefaulttimeout(timeout)

        req = urllib2.Request(url, None, headers={ 'User-Agent': 'Python-urllib/2.7' })

        result = urllib2.urlopen(req, timeout=timeout).read().strip()
        if result == "":
            raise Exception("Got empty result while loading %s" % url)
        return result
    except urllib2.HTTPError:
        raise Exception("Got HTTPError while loading %s" % url)
    finally:
        socket.setdefaulttimeout(oldtimeout)


# =============================================================================================
# Detect "device" with biggest disk
# =============================================================================================
def _get_primary_disk():
    data = run_command(["df", "-P", "-B", "1073741824"])[1]
    data = map(lambda x: (re.split(' +', x)[0], int(re.split(' +', x)[1])), data.split('\n')[1:-1])
    data = filter(lambda (name, sz): name.startswith('/dev'), data)
    data.sort(cmp=lambda (name1, sz1), (name2, sz2): cmp(sz2, sz1))
    return data[0][0]


def hivemind_hostname(parent):
    if 'custom_hostname' in parent.params:
        return parent.params['custom_hostname']
    return library.config.detect_hostname()


def detect_name(parent):
    return hivemind_hostname(parent)


def detect_model(parent):
    BOT_MODELS = {
        "OPTERON6172": "AMD6172",
        "OPTERON6176": "AMD6176",
        "OPTERON6274": "AMD6274",
        "XEONE5-2650V2": "E5-2650v2",
        "XEONE5-2660": "E5-2660",
        "XEONE5-2667V": "E5-2667",
        "XEONE5-2667V2": "E5-2667v2",
        "XEONE5-2660V4": "E5-2660v4",
        "XEONE5-2667V4": "E5-2667v4",
        "XEON5440": "E5440",
        "XEON5530": "E5530",
        "XEON5620": "E5620",
        "XEON5645": "E5645",
        "XEON5410": "L5410",
        "XEON5450": "X5450",
        "XEON5670": "X5670",
        "XEON5675": "X5675",
    }

    if "custom_hostname" in parent.params:  # local run for arbitary host:
        response = retry_urlopen(3, "http://bot.yandex-team.ru/api/consistof.php?name=%s" % parent.params[
            "custom_hostname"])
        m = re.search("Model:\s*(.*?)\s+", response)
        bot_model = m.group(1)
        return BOT_MODELS[bot_model]
    else:
        model = re.search('model name\s+: (.*)', run_command(["cat", "/proc/cpuinfo"])[1]).group(1).strip()
        if model in parent.params['models']:
            return parent.params['models'][model]
        else:
            raise Exception("Unknown cpu model <%s>" % model)


def detect_disk(parent):
    del parent
    primary_disk = _get_primary_disk()
    data = run_command(["df"])[1]
    m = re.search('^%s\s+\d+\s+(\d+)\s+(\d+)' % primary_disk, data, flags=re.MULTILINE)
    used = int(m.group(1))
    avail = int(m.group(2))

    return (used + avail) / 1024 / 1024


class LinuxDiskInfo(object):
    MARKERS = [('Host:', 'host'),
               ('Vendor:', 'vendor'),
               ('Model:', 'model'),
               ('Type:', 'atype'),
               ('ANSI  SCSI revision:', 'revision'),
               ]

    def __init__(self, data):
        indexes = map(lambda (marker, field): data.find(marker), self.MARKERS)
        if -1 in indexes:
            raise Exception("No some data in disk description")
        indexes.append(len(data))

        for i in range(len(self.MARKERS)):
            self.__dict__[self.MARKERS[i][1]] = data[indexes[i] + len(self.MARKERS[i][0]):indexes[i + 1]].strip()


def detect_n_disks(parent):
    del parent
    __, n_disks, _ = run_command(["cat", "/proc/scsi/scsi"])
    n_disks = n_disks.split('\n')
    n_disks = [line.strip() for line in n_disks if line.strip()]
    assert (n_disks[0].startswith('Attached devices:'))
    n_disks = n_disks[1:]
    assert (len(n_disks) % 3 == 0)

    disks = map(lambda (x, y, z): LinuxDiskInfo(' '.join([x, y, z])), zip(n_disks[0::3], n_disks[1::3], n_disks[2::3]))

    # filter stuff
    disks = filter(lambda x: not x.model.startswith('Virtual'), disks)
    disks = filter(lambda x: x.atype != 'CD-ROM', disks)

    return len(disks)


def detect_ssd(parent):
    del parent
    _1, output, _2 = run_command(['df', '-P', '-B', '1073741824'])
    lines = output.rstrip().split('\n')[1:]
    lines = [line.split() for line in lines]
    mountpnt_size = dict((line[5], ((int(float(line[1]))), line[0].split('/')[-1])) for line in lines)

    # we have a convention, that if there is ssd disk
    # it will be mounted at '/ssd' or '/fasthd'
    if '/ssd' in mountpnt_size:
        return str(mountpnt_size['/ssd'][0])
    if '/fasthd' in mountpnt_size:
        return str(mountpnt_size['/fasthd'][0])
    sz = 0
    for device in os.listdir('/sys/block'):
        if device.startswith('loop') or device.startswith('ram'):
            continue
        is_rotational = int(open(os.path.join('/sys/block', device, 'queue/rotational')).read())
        if not is_rotational:
            sz += int(open(os.path.join('/sys/block', device, 'size')).read()) * 512 / 1024 / 1024 / 1024
    return sz


def detect_memory(parent):
    del parent
    mem = int(re.search('MemTotal: ([^\n]*)', run_command(["cat", "/proc/meminfo"])[1]).group(1).strip().split(' ')[
                  0]) / 1024 / 1024

    if mem % 8 < 4:
        mem -= mem % 8
    else:
        mem += 8 - mem % 8

    return mem


def detect_queue(parent):
    # Use oops to updated queue
    raise Exception('Get queue from bot instead of golem')


def detect_ipmi(parent):
    ipminame = '%s.ipmi.yandex-team.ru' % hivemind_hostname(parent)
    try:
        result = socket.getaddrinfo(ipminame, None, socket.AF_INET6)
        if len(result) == 0:
            return 0
        else:
            return 1
    except socket.gaierror:
        return 0


def detect_os(parent):
    del parent
    return os.uname()[0]


def is_vm_guest():
    scsi = open('/proc/scsi/scsi').read()
    return scsi.find('QEMU HARDDISK') != -1


def detect_flags(parent):
    del parent
    return IS_VM_GUEST_CYTHON * bool(is_vm_guest())


def detect_unit(parent):
    jsoned = bot_consists_of_as_json(hivemind_hostname(parent))
    return jsoned['data']['Connected'][0]['connected_attribute']


def detect_kernel(parent):
    del parent
    return run_command(["uname", "-r"])[1].strip()


def detect_issue(parent):
    del parent
    return open("/etc/issue").read().strip()


def detect_raid(parent):
    del parent
    diskname = _get_primary_disk()
    shortname = diskname.split('/')[-1]
    data = open('/proc/mdstat').read()
    m = re.search('%s : (.+?) (.+?) ' % shortname, data)
    if m:
        return m.group(2)
    else:
        return "raid0"


def detect_platform(parent):
    del parent
    data = run_command(["sudo", "dmidecode", "-t", "2"])[1]
    data += "\n" + run_command(["sudo", "dmidecode", "-t", "1"])[1]
    return re.search('Product Name: (.*)', data).group(1).strip()


def detect_netcard(parent):
    del parent
    # card quality, higher is better, return card with best quality if have several. Default quality is zero
    DEF_QUALITY = 0
    CARD_QUALITY = {
        '82574L Gigabit Network Connection': -1,
    }

    data = run_command(["lspci", "-vmm"])[1]
    matchers = map(lambda x: re.search('\nClass:\s+Ethernet controller[\s\S]*\nDevice:\s+(.*)', x, re.MULTILINE),
                   re.split('Slot:', data))
    matchers = filter(lambda x: x is not None, matchers)

    cards = list(set(map(lambda x: x.group(1), matchers)))
    cards.sort(cmp=lambda x, y: cmp(CARD_QUALITY.get(y, DEF_QUALITY), CARD_QUALITY.get(x, DEF_QUALITY)))

    if len(cards):
        return cards[0]
    else:
        return 'unknown'


def detect_ipv4addr(parent):
    myname = hivemind_hostname(parent)

    try:
        addrs = socket.getaddrinfo(myname, 80, socket.AF_INET)
        return addrs[0][4][0]
    except socket.gaierror:
        return "unknown"


def detect_ipv6addr(parent):
    myname = hivemind_hostname(parent)

    if CURDB.hosts.has_host(myname) and CURDB.hosts.get_host_by_name(myname).is_ipv6_generated():
        myswitch = detect_switch(parent)
        # ============== NOCDEV-251 START ===============================
        if myswitch == 'sas1-s910':
            myswitch = 'sas1-s909'
        # ============== NOCDEV-251 FINISH ==============================
        myhwaddr = detect_hwaddr(parent)

        mynet = 'unknown'
        for vlan in ['604', '1464', '603', '1496', '688']:
            if mynet == 'unknown':
                mynet = TL3Ipv6Nets.instance().nets.get((myswitch, vlan), 'unknown')

        if myswitch == 'unknown' or myhwaddr == 'unknown' or mynet == 'unknown':
            return 'unknown'

        return str(netaddr.EUI(myhwaddr).ipv6(netaddr.IPNetwork(mynet).ip).ipv6())
    else:
        try:
            addrs = socket.getaddrinfo(myname, 80, socket.AF_INET6)
            return addrs[0][4][0]
        except socket.gaierror:
            PDOMAIN = '.yandex.ru'
            if myname.endswith(PDOMAIN):
                myname = myname[:-len(PDOMAIN)] + '.search.yandex.net'
                addrs = socket.getaddrinfo(myname, 80, socket.AF_INET6)
                return addrs[0][4][0]

    return 'unknown'


def detect_botprj(parent):
    del parent
    return 'unknown'  # can not be detected this way


def detect_vlan688ip(parent):
    status, out, err = run_command(["ifconfig", "vlan688"])
    m = re.search('inet6 addr: (.*)/64 Scope:Global', out)
    if not m:
        return 'unknown'

    return m.group(1)


def detect_vlans(parent):
    result = {}

    for vlan_name in ['vlan688', 'vlan788']: # SETTINGS.constants.hbf.vlans
        status, out, err = run_command(["ifconfig", vlan_name])
        m = re.search('inet6 addr: (.*)/64 Scope:Global', out)
        if m:
            result[vlan_name] = m.group(1)

    return result


def bot_consists_of_as_json(hostname, is_invnum=False):
    if (hostname, is_invnum) not in bot_consists_of_as_json.cached_data:
        boturl = "https://bot.yandex-team.ru/api/consistof.php"  # SETTINGS.services.bot.rest.consistof.url
        if is_invnum:
            url = '%s?inv=%s&format=json' % (boturl, hostname)
        else:
            url = '%s?name=%s&format=json' % (boturl, hostname)
        bot_consists_of_as_json.cached_data[(hostname, is_invnum)] = json.loads(gaux.aux_utils.memoize_urlopen(1, url))
    return bot_consists_of_as_json.cached_data[(hostname, is_invnum)]
bot_consists_of_as_json.cached_data = dict()


def detect_botmem(parent):
    def convert_ram(s):
        m = re.match("(\d+)GB", s)
        if m:
            return int(m.group(1))

        m = re.match("(\d+)MB", s)
        if m:
            return 0

        raise Exception("Could not convert <%s> as valid ram size" % s)

    jsoned = bot_consists_of_as_json(hivemind_hostname(parent))

    total_mem = 0
    for component in jsoned["data"]["Components"]:
        if component["item_segment3"] == "RAM":
            total_mem += convert_ram(component["item_segment2"])

    return total_mem


def detect_botdisk(parent):
    def convert_disk(s):
        if s.find("/SSD/") >= 0:
            return 0

        m = re.match("(\d+)GB.*", s)
        if m:
            return int(m.group(1))

        raise Exception("Could not convert <%s> as valid disk size" % s)

    consistof_params = [(hivemind_hostname(parent), False)] + map(lambda x: (x, True), detect_shelves(parent))

    total_disk = 0
    for jsoned in map(lambda x: bot_consists_of_as_json(*x), consistof_params):
        for component in jsoned["data"]["Components"]:
            if component["item_segment3"] == "DISKDRIVES":
                total_disk += convert_disk(component["item_segment2"])

    return total_disk


def detect_botssd(parent):
    def convert_ssd(s):
        if s.find("/SSD/") == -1:
            return 0

        m = re.match("(\d+)GB.*", s)
        if m:
            return int(m.group(1))

        raise Exception("Could not convert <%s> as valid ssd size" % s)

    consistof_params = [(hivemind_hostname(parent), False)] + map(lambda x: (x, True), detect_shelves(parent))

    total_ssd = 0
    for jsoned in map(lambda x: bot_consists_of_as_json(*x), consistof_params):
        for component in jsoned["data"]["Components"]:
            if component["item_segment3"] == "DISKDRIVES":
                total_ssd += convert_ssd(component["item_segment2"])

    return total_ssd


def detect_shelves(parent):
    invnum = detect_invnum(parent)
    if invnum == 'unknown':
        return []

    boturl = "http://bot.yandex-team.ru/api/storages-by-serverinv.php"  # SETTINGS.services.bot.rest.storages_by_invnum.url
    url = '%s?inv=%s' % (boturl, invnum)
    response = gaux.aux_utils.memoize_urlopen(1, url).split()

    if len(response) > 1 and response[0] == 'YES':
        return response[1:]
    else:
        return []


def detect_ffactor(parent):
    del parent
    return "unknown"  # not supported yet, get this data from oops dump


def detect_hwaddr(parent):
    """
        Function "detects" mac address of machine. Currently we do nothing for real machines and autogenerate address
        for virtual ones ( https://st.yandex-team.ru/GENCFG-301 )
    """

    myname = hivemind_hostname(parent)
    if CURDB.hosts.has_host(myname) and CURDB.hosts.get_host_by_name(myname).is_hwaddr_generated():
        return "06" + md5.new(myname).hexdigest()[:10].upper()

    return "unknown"

def detect_l3enabled(parent):
    """
        Function detects, if host under consideration inside L3 (thus can be used as base for vm group)
    """

    if detect_switch(parent) in TL3Ipv6Nets.instance().l3switches:
        return True
    return False

def detect_vmfor(parent):
    """
        Detecting vmfor is not enabled at the time. This field is filled when
        virtual host is created.
    """
    return ''

def detect_storages(parent):
    """
        Detect, how machine disks divided into independent storages. Can be now processed only
        on remote machine.
    """

    if 'custom_hostname' in parent.params:
        raise Exception, "Could not detect storages with remote host"

    result = gaux.aux_storages.detect_configuration(0)

    return result


def detect_net(parent):
    """Compability func (netio in megabytes)"""
    return 1000


def detect_walle_tags(parent):
    """Compatibility func"""
    raise NotImplemented('Function <detect_walle_tags> is not implemented')


def detect_gpu_count(parent):
    jsoned = bot_consists_of_as_json(hivemind_hostname(parent))

    gpu_count = 0
    for component in jsoned["data"]["Components"]:
        if component["item_segment2"] == "GPU":
            gpu_count += 1

    return gpu_count


def detect_gpu_models(parent):
    jsoned = bot_consists_of_as_json(hivemind_hostname(parent))

    gpu_models = []
    for component in jsoned["data"]["Components"]:
        if component["item_segment2"] == "GPU":
            gpu_model = component['attribute20']
            if gpu_model == 'N/A':
                gpu_model = component['attribute12']
            if gpu_model not in gpu_models:
                gpu_models.append(gpu_model)

    return gpu_models

# see GENCFG-4247, contact fifteen@, eukho@
# table contains joined data from bot's api/view.php and api/consistof.php
def read_hwstat():
    yt.config.set_proxy(YT_CLUSTER)
    allowed_tables = re.compile("\d{4}-\d{2}-\d{2}")

    table = list(
        filter(
            lambda x: allowed_tables.search(x) is not None,
            yt.list(HWSTAT_DIRECTORY)
        )
    )[-1]
    data = yt.read_table(HWSTAT_DIRECTORY + '/' + table, format='json')
    return data


class GetStats(object):
    def __init__(self, models, funcs, user=None, timeout=1, ignore_detect_fail=False):
        self.params = {'models': models,}
        self.funcs = funcs
        self.osUser = user
        self.timeout = timeout
        self.ignore_detect_fail = ignore_detect_fail
        self.warnings = []

    def anti_ddos_delay(self):
        delay = random.randrange(self.timeout)
        time.sleep(delay)

    def run(self):
        self.anti_ddos_delay()
        result = {}
        for signal, func in self.funcs.iteritems():
            try:
                result[signal] = func(self)
            except:
                if self.ignore_detect_fail:
                    continue
                else:
                    raise

        return result, self.warnings

    @classmethod
    def member_count(cls):
        return 8


class IRunner(object):
    def __init__(self, options):
        pass

    def wait(self):
        raise Exception("Pure virtual function called")


class TSkynetRunner(IRunner):
    def __init__(self, options, funcs, models):
        super(TSkynetRunner, self).__init__(options)

        self.options = options

        client = api.cqueue.Client('cqudp', netlibus=True)

        if options.skynet_timeout is None:
            timeout = max(10, len(options.hosts) / 30 + 1)
        else:
            timeout = options.skynet_timeout

        if options.user is None:
            skynet_user = gaux.aux_utils.getlogin()
        else:
            skynet_user = options.user

        self.generator = client.run(options.hosts, GetStats(models, funcs, user=skynet_user,
                                                            ignore_detect_fail=options.ignore_detect_fail)).wait(
            timeout)

    def wait(self):
        for elem in self.generator:
            yield elem


class TLocalRunner(IRunner):
    AVAIL_FUNCS = ['ipv4addr', 'ipv6addr', 'queue', 'model',
                   'botmem', 'botdisk', 'botssd', 'shelves', 'hwaddr', 'l3enabled', 'unit',
                   'gpu_count', 'gpu_models']

    DISABLED_FUNCS = ['vlan', 'dc', 'switch', 'golemowners', 'invnum', 'rack']

    def __init__(self, options, funcs, models, nprocs=20):
        super(TLocalRunner, self).__init__(options)
        self.hosts = options.hosts
        self.funcs = funcs
        self.models = models
        self.nprocs = nprocs

        self.ignore_detect_fail = options.ignore_detect_fail

        unavail_funcs = set(funcs.keys()) - set(self.AVAIL_FUNCS).union(set(self.DISABLED_FUNCS))
        if len(unavail_funcs) > 0:
            raise Exception("Can not calculate funcs <%s> in local runner" % ','.join(unavail_funcs))

    def process_host(self, host):
        parent = type("DummyParent", (), {})()
        parent.params = {'custom_hostname': host}

        result = {}
        result_warnings = []
        try:
            for signal, func in self.funcs.iteritems():
                if signal not in self.AVAIL_FUNCS:
                    continue
                try:
                    result[signal] = func(parent)
                except:
                    if self.ignore_detect_fail:
                        continue
                    else:
                        raise
            return host, (result, result_warnings), None
        except Exception:
            return host, None, traceback.format_exc()

    def wait(self):
        if self.nprocs > 1:
            pool = ThreadPool(processes=self.nprocs, initializer=init_CURDB)
            for host_result in pool.imap(self.process_host, self.hosts):
                yield host_result
        else:
            for host in self.hosts:
                yield self.process_host(host)


class IPreparedDataRunner(IRunner):
    def __init__(self, options, funcs, models):
        super(IPreparedDataRunner, self).__init__(options)
        self.hosts = options.hosts
        self.funcs = funcs
        self.models = models

        self.ignore_detect_fail = options.ignore_detect_fail

        # prepare mongo stuff
        self.data = self.prepare_data()

        self.debug_output_history()

    def debug_output_history(cls):
        with open('db/history.log', 'a') as wfile:
            wfile.write('{} {} `{}`\n'.format(datetime.datetime.now(), cls, ' '.join(sys.argv)))

    def prepare_data(self):
        raise NotImplementedError("Found not implemented function <prepare_data>")

    def wait(self):
        for host in self.hosts:
            result = {}
            if host not in self.data:
                try:
                    raise Exception("Host <%s> not found in db" % host)
                except Exception:
                    yield host, None, traceback.format_exc()
            else:
                failed = False
                for signal in self.funcs:
                    try:
                        result[signal] = self.data[host][signal]
                    except KeyError:
                        if self.ignore_detect_fail:
                            continue
                        else:
                            failed = True
                            yield host, None, traceback.format_exc()
                result['change_time'] = self.data[host].get('change_time', {})

                if not failed:
                    yield host, (result, []), None


class THbRunner(IPreparedDataRunner):
    def __init__(self, options, funcs, models):
        super(THbRunner, self).__init__(options, funcs, models)

    def prepare_data(self):
        mongocoll = get_mongo_collection('hwdata')

        mongoresult = mongocoll.find({'host': {'$in': self.hosts}}, )

        now = datetime.datetime.now()

        result = filter(lambda x: x['last_update'] + datetime.timedelta(hours=30) >= now, mongoresult)
        result = dict(map(lambda x: (x['host'], x), result))

        # can not update queue from heartbeat
        for v in result.itervalues():
            v.pop('queue', None)
            v.pop('dc', None)
            v.pop('model', None)
            # these fields are not updated from heartbeat
            v.pop('invnum', None)
            v.pop('rack', None)
            v.pop('vlan', None)
            v.pop('switch', None)

        # memory set memory from botmem field if (KIWIADMIN-845)
        for host_name, host_data in result.iteritems():
            if host_data.get('memory', 0) == 248:
                host_data['memory'] = 256

            if 'memory' not in host_data:
                host = CURDB.hosts.get_host(host_name)
                host_data['memory'] = host.botmem

        return result


class TOopsRunner(IPreparedDataRunner):
    OOPS_FFACTORS = {
        '1U': '1U',
        '1x0.5U': '0.5U',
        '2U': '2U',
        '2x0.5U': '2x0.5U',
        '3U': '3U',
        '4U': '4U',
        '5U': '5U',
        '6U': '6U',
    }

    UPDATE_SECTIONS = {
        'net_info': {
            'fields': ('vlans', 'net')
        }
    }

    DEFAULT_HOST_DATA = {
        'name': "",
        'change_time': {},
    }

    DISABLED_FUNCS = ["golemowners", "vlan", "rack", "switch"]

    def __init__(self, options, funcs, models):
        funcs_copy = funcs.copy()
        for key in self.DISABLED_FUNCS:
            funcs_copy.pop(key)
        super(TOopsRunner, self).__init__(options, funcs_copy, models)

    def update_hwstat(self, result):
        data = read_hwstat()
        m = re.compile('[^a-zA-Z0-9-._]')

        for elem in data:
            elem = {k: v if v is not None else '' for k, v in elem.iteritems()}
            hostname = elem["fqdn"].lower()
            if hostname == "" or hostname not in result:
                continue

            invnum = str(elem["inventory_number"])
            rack = elem["loc_segment5"]
            if isinstance(rack, unicode):
                rack = rack.encode('utf-8')
            unit = elem["loc_segment6"]
            botprj = transliterate.translit(elem.get("Service", ""), 'ru', reversed=True).encode('utf-8')
            botprj = botprj.replace('"', '')

            # detecting dc is more difficult
            if elem["loc_segment2"] == 'MANTSALA':
                dc = 'man'
            elif elem["loc_segment2"] == 'MOW':
                dc = elem["loc_segment3"].lower()
            elif elem["loc_segment2"] == 'VLADIMIR':
                dc = 'vla'
            else:
                dc = elem["loc_segment2"].lower()

            # detecting queue is more difficult
            queue = elem["loc_segment4"].lower()
            if queue == 'man-2#b.1.07':
                queue = 'man2'
            elif queue == 'man-1#b.1.06':
                queue = 'man1'

            result[hostname]['name'] = hostname
            result[hostname]['invnum'] = invnum
            result[hostname]['dc'] = re.sub(m, '_', dc)
            result[hostname]['queue'] = re.sub(m, '_', queue)
            result[hostname]['rack'] = rack
            result[hostname]['botprj'] = botprj
            result[hostname]['unit'] = unit

            # probing different locations
            model = 'unknown'
            if 'componentOf' in elem['consist_of']:
                components = elem['consist_of']['componentOf']
                for k in components:
                    if components[k]["context"] == "SRV.CPU":
                        if "attribute12" in components[k]:
                            model = components[k]["attribute12"]
                        model = CURDB.cpumodels.get_model_by_botmodel(model)
                        break

            if model == 'unknown':
                model = elem["item_segment2"].partition('/')[0]
                model = CURDB.cpumodels.get_model_by_botmodel(model)

            if model == 'unknown':
                model = elem["attribute17"]
                model = CURDB.cpumodels.get_model_by_botmodel(model)

            result[hostname]['model'] = model

            # detecting form factor is more difficult
            ffactor = 'unknown'
            for part in elem["item_segment1"].split('/'):
                if part in TOopsRunner.OOPS_FFACTORS:
                    ffactor = TOopsRunner.OOPS_FFACTORS[part]
                    break
            result[hostname]['ffactor'] = ffactor

    def update_yp(self, result, yp_keys=None):
        yp_keys = yp_keys or self.UPDATE_SECTIONS.keys()
        yp_client = aux_yp.YP(config.get_default_oauth())
        if 'net_info' in yp_keys:
            update_section = self.UPDATE_SECTIONS['net_info']
            hostnames_by_dc = collections.defaultdict(list)
            for hostname in result:
                if 'dc' in result[hostname]:
                    dc = result[hostname]['dc']
                    info = yp_client.parse_net_info_for_host(hostname, dc)
                    if info:
                        for field in update_section['fields']:
                            if field not in info:
                                continue
                            result[hostname][field] = info[field]
                            result[hostname]['change_time'][field] = datetime.datetime.now().strftime("%Y-%m-%dT%H:%M:%S")

    def update_oops(self, result, oops_keys=None):
        oops_keys = oops_keys or self.UPDATE_SECTIONS.keys()

        oops = aux_oops.OopsApi()
        data = oops.attributes(result.keys(), keys=oops_keys)

        for api_key in oops_keys:
            self.update_oops_parse_api(result, api_key, data)

    def update_oops_parse_api(self, result, api_key, data=None):
        if data is None:
            oops = aux_oops.OopsApi()
            data = oops.attributes(result.keys(), keys=[api_key])

        update_section = self.UPDATE_SECTIONS[api_key]
        for hostname, host_data in data.items():
            if api_key not in host_data:
                continue
            info = update_section['parser'](host_data[api_key], hostname)

            for field in update_section['fields']:
                if field == 'model':
                    info[field] = CURDB.cpumodels.get_model_by_fullname(info[field])

                if info[field] is not None:
                    result[hostname][field] = info[field]
                    result[hostname]['change_time'][field] = host_data[api_key]['@changeTime']

    def update_walle(self, result):
        walle = Walle(config.get_default_oauth())
        data = walle.hosts_info()

        self.update_walle_walle_tags(result, data)

    def update_walle_walle_tags(self, result, data=None):
        if data is None:
            walle = Walle(config.get_default_oauth())
            data = walle.hosts_info()

        for hostname in result:
            result[hostname]['walle_tags'] = []

        for hostname, host_info in data.items():
            if not host_info['project'] or not host_info['tags'] or hostname not in result:
                continue

            walle_project = 'g:{}'.format(host_info['project'])
            walle_status = 'status:{}'.format(host_info['status'])
            walle_state = 'state:{}'.format(host_info['state'])
            walle_tags = host_info['tags'] + [walle_project, walle_status, walle_state]

            result[hostname]['walle_tags'] = walle_tags

        # ===================== RX-1338 START ======================================
        walle_hosts = frozenset(data.iterkeys())
        for host in CURDB.hosts.get_hosts():
            if host.name not in walle_hosts:
                if host.name not in result:
                    result[host.name] = copy.copy(self.DEFAULT_HOST_DATA)
                    result[host.name]["name"] = host.name
                result[host.name]['walle_tags'] = []
        # ===================== RX-1338 FINISH ======================================

    def update_walle_switch(self, result, data=None):
        if data is None:
            walle = Walle(config.get_default_oauth())
            data = walle.hosts_info()
        for hostname, host_info in data.items():
            if hostname in result and 'switch' in host_info['location']:
                result[hostname]['switch'] = host_info['location']['switch'].encode()

    def prepare_data(self):
        need_to_update_fields = set(self.funcs.keys())

        result = {}
        for hostname in aux_oops.OopsApi().all_fqdns():
            result[hostname] = copy.copy(self.DEFAULT_HOST_DATA)
            result[hostname]["name"] = hostname

        # ===================== RX-871 START ======================================
        if need_to_update_fields & {'name', 'invnum', 'dc', 'queue', 'rack', 'botprj', 'unit', 'model', 'ffactor'}:
            self.update_hwstat(result)

        update_sections = []
        for section, data in self.UPDATE_SECTIONS.items():
            common = set(data['fields']) & need_to_update_fields
            if common:
                update_sections.append(section)

        if update_sections:
            self.update_yp(result, update_sections)
        # ===================== RX-871 FINISH ======================================

        # ===================== RX-981 START ======================================
        if 'walle_tags' in self.funcs:
            self.update_walle(result)
        # ===================== RX-981 FINISH ======================================

        self.update_walle_switch(result)

        return result


def check_host_all_memory(db, oldhost, old_resource, updated_resource, field_name):
    required = 0
    host_groups = db.groups.get_host_groups(oldhost)
    for host_group in host_groups:
        group_reqs = host_group.card.reqs.instances
        required += len(host_group.get_host_instances(oldhost)) * getattr(group_reqs, field_name).value

    GB = 1024 * 1024 * 1024

    if not oldhost.is_vm_guest() and field_name == "memory":
        required += 3 * GB

    system_size = 5
    if (field_name == "ssd" and oldhost.disk > 5) or (field_name == "disk" and oldhost.ssd > 5):
        system_size = 0.0

    if required > (updated_resource - system_size) * GB - updated_resource * GB * 0.02:
        print('!!! Can not update {} hardware (new {} size {:.2f} -> {:.2f} not enough to contain all instances'.format(
            oldhost.name, field_name, old_resource, updated_resource
        ))
        return False
    return True


def check_new_host_hw(db, oldhost, update_fields, options):
    """
        Sometimes changes in host hardware data contradicts with our allocated instances
        (e. g., according to hb host memory is decreased). In this cases we can not update
        our base, because it becames broken.

        :param db: database to process (CURDB usually)
        :param oldhost: host, which fields are about to be updated
        :param uptate_fields: dicti with updated properties and new values, e.g. {'memory': 64}
        :param options: utility options

        :return: True if everything Ok, otherwise False
    """

    def get_changed_host_fields(host, update_fields):
        return '\n'.join(
            map(lambda x: '    %s: %s -> %s' % (x, getattr(host, x), update_fields[x]), update_fields.keys()))

    newhost = copy.deepcopy(oldhost)  # FIXME: make better
    for k in update_fields:
        setattr(newhost, k, update_fields[k])
    newhost.postinit(db.cpumodels.models)

    # should check only groups with intlookups
    groups = db.groups.get_host_groups(oldhost)
    for group in groups:
        if len(group.card.intlookups) == 0:
            continue
        ifunc = group.gen_instance_func(custom_instance_power={})
        oldinstances = map(lambda x: Instance(*x), ifunc(oldhost))
        try:
            newinstances = map(lambda x: Instance(*x), ifunc(newhost))
        except Exception:
            print "!!! Can not update %s hardware (conflict with group %s) (case 0):\n%s" % (
            oldhost.name, group.card.name, get_changed_host_fields(oldhost, update_fields))
            traceback.print_exc()
            print "----------"
            return False
        if not options.ignore_group_constraints:
            if len(oldinstances) > len(newinstances):
                print "!!! Can not update %s hardware (conflict with group %s) (case 1):\n%s" % (
                oldhost.name, group.card.name, get_changed_host_fields(oldhost, update_fields))
                return False
            if sum([x.power for x in oldinstances]) != sum([x.power for x in newinstances]):
                print "!!! Can not update %s hardware (conflict with group %s) (case 2):\n%s" % (
                oldhost.name, group.card.name, get_changed_host_fields(oldhost, update_fields))
                return False

    # check if we have enough memory for instances in case memory was reduced
    if 'memory' in update_fields and oldhost.memory > update_fields['memory']:
        if not check_host_all_memory(db, oldhost, oldhost.memory, update_fields['memory'], 'memory_guarantee'):
            return False

    if 'disk' in update_fields and oldhost.disk > update_fields['disk']:
        if not check_host_all_memory(db, oldhost, oldhost.disk, update_fields['disk'], 'disk'):
            return False

    if 'ssd' in update_fields and oldhost.disk > update_fields['ssd']:
        if not check_host_all_memory(db, oldhost, oldhost.ssd, update_fields['ssd'], 'ssd'):
            return False

    return True


def get_parser():
    import core.argparse.types as argparse_types

    parser = ArgumentParserExt(description="Updated hosts hardware info", usage="""
    %(prog)s -a add -s <fqdn1,...,fqdnN> [-r <group>] [-y]
    %(prog)s -a addcopy -s <fqdn1,...,fqdnN> -f <fqdn_from_DB> [-y]
    %(prog)s -a addfake -s <fqdn1,...fqdnN> [-y]
    %(prog)s -a addraw -s <info1,...,infoN> [-y]
    %(prog)s -a update ([-s <fqdn1,...,fqdnN>] | [-g <group1,...,groupN>) [-i <field1,...,fieldN>] [-U <user>] [-p] [--ignore-detect-fail] [-y] [--update-older-than <days since now>] [--ignore-group-constraints]
    %(prog)s -a updatelocal ([-s <fqdn1,...,fqdnN>] | [-g <group1,...,groupN>) [-i <field1,...,fieldN>] [--ignore-detect-fail] [-y]
    %(prog)s -a remove -s <fqdn1,...,fqdnN> [-y]
""")

    ACTIONS = ["add", "addcopy", "addfake", "addraw", "update", "updatelocal", "updatehb", "updateoops", "remove", "fixcard"]

    parser.add_argument("-a", "--action", type=str, dest="action", required=True,
                        choices=ACTIONS,
                        help="Obligatory. Action to execute: %s" % ", ".join(ACTIONS))
    parser.add_argument("--db", type=core.argparse.types.gencfg_db, default=CURDB,
                        help="Optional. Path to db")
    parser.add_argument("-v", "--verbose", action="store_true", dest="verbose", default=False,
                        help="show verbose output")
    parser.add_argument("-i", "--update-fields", dest="update_fields", type=str, default=None,
                        help="Optional. Update only comma-separated list of fields: %s" % ','.join(
                            map(lambda x: x.name, get_host_fields_info())))
    parser.add_argument("-U", "--user", type=str, default=None,
                        help="Optional. User for cqueue tasks (use current login by default)")
    parser.add_argument("-s", "--hosts", dest="hosts", type=argparse_types.hostnames, default=None,
                        help="Optional. Comma-separated list of hosts to performa action on")
    parser.add_argument("-g", "--groups", dest="groups", type=argparse_types.groups, default=None,
                        help="Optional. Comma-separated list of groups")
    parser.add_argument("--exclude-groups", type=argparse_types.groups, default=None,
                        help="Optional. Exclude hosts from specified groups")
    parser.add_argument("-w", "--raw-data", dest="raw_data", type=str, default=None,
                        help="Optional. Comma-separated description of hosts as stored in hardware_data")
    parser.add_argument("-f", "--fake-host-copy-of", type=core.argparse.types.host, default=None,
                        help="Optional. Add fake hosts which will be copies of a given host")
    parser.add_argument("-l", "--run-local", action="store_true", dest="run_local", default=False,
                        help="Optional. Run locally and print tuple instead of run remotely")
    parser.add_argument("-n", "--do-not-touch-existing", action="store_true", dest="do_not_touch_existing",
                        default=False,
                        help="Optional. Do not touch existing hosts")
    parser.add_argument("--ignore-unknown-lastupdate", action="store_true", dest="ignore_unkown_lastupdate",
                        default=False,
                        help="Optional. Do to update hosts which was not updated once")
    parser.add_argument("--update-older-than", dest="update_older_than", type=int, default=None,
                        help="Optional. Updated only hosts that was not updated in last N days")
    parser.add_argument("--update-to-def-value", action="store_true", default=False,
                        help="Optional. Update fields to default values if detector returns default")
    parser.add_argument("-p", "--show-progress", dest="show_progress", action="store_true", default=False,
                        help="Optional. Show progress")
    parser.add_argument("--ignore-group-constraints", dest="ignore_group_constraints", action="store_true",
                        default=False,
                        help="Optional. Ignore group constraints and changes in power/number of instances after update")
    parser.add_argument("--ignore-detect-fail", dest="ignore_detect_fail", action="store_true", default=False,
                        help="Optional. Ignore all fails (set field to default). Beware of using this option")
    parser.add_argument("--host-filter", type=argparse_types.pythonlambda, default=None,
                        help="Optional. Extra filter on hosts")
    parser.add_argument("-y", "--apply", dest="apply", action="store_true", default=False,
                        help="Optional. Apply changes (otherwise db will not be changed)")
    parser.add_argument("-r", "--reserved-group", type=argparse_types.group, default=None,
                        help="Optional. Group for new hosts (affects only 'add' action)")
    parser.add_argument("-t", "--skynet-timeout", default=None,
                        help="Optional. Skynet timeout (set custome instead of manually calculated)")
    parser.add_argument("--threads", type=int, default=20,
                        help="Optional. Number of threads for <updatelocal> mode")
    parser.add_argument("-k", "--card-field", type=str, default=None,
                        help="Optional. Host card field to change for <fixcard> mode")
    parser.add_argument("-e", "--card-value", type=str, default=None,
                        help="Optional. New card field value for <fixcard> mode")

    return parser


def normalize(options):
    if options.action == "add":
        if options.update_fields is not None:
            raise Exception("Incompatible options 'add' and 'update-members'.")
        if options.hosts is None and options.raw_data is None:
            raise Exception("Using 'add' options with empty host list")
        if options.hosts is not None and options.raw_data is not None:
            raise Exception("Both --hosts and --raw-data options specified")
    if options.action == "remove":
        if options.hosts is None:
            raise Exception("Using 'remove' options with empty host list")
    if options.action != "update" and options.ignore_unkown_lastupdate:
        raise Exception("Option --ignore-unknown-lastupdate incompatable with action <<%s>>" % options.action)
    if options.action != "update" and options.update_older_than:
        raise Exception("Option --update-older-than incompatable with action <<%s>>" % options.action)

    if options.action != "addcopy" and options.fake_host_copy_of is not None:
        raise Exception("Optoin --addcopy is incompatable with action --fake-host-copy-of")

    if options.groups is not None:
        if options.action not in ["update", "updatelocal", "updatehb", "updateoops"]:
            raise Exception("Used --group argument with <%s> action" % options.action)
        if options.hosts is None:
            options.hosts = []
        for group in options.groups:
            options.hosts.extend(map(lambda x: x.name, group.getHosts()))
    if options.exclude_groups is not None:
        exclude_hosts = sum((x.getHosts() for x in options.exclude_groups), [])
        exclude_hosts = {x.name for x in exclude_hosts}
        options.hosts = [x for x in options.hosts if x not in exclude_hosts]
    if options.ignore_unkown_lastupdate:
        options.hosts = filter(lambda x: options.db.hosts.get_host_by_name(x).lastupdate != 'unknown', options.hosts)
    if options.update_older_than:
        now = time.time()
        options.hosts = filter(lambda x: (options.db.hosts.get_host_by_name(x).lastupdate == 'unknown') or (
        now - time.mktime(time.strptime(CURDB.hosts.get_host_by_name(x).lastupdate,
                                        TIMEFMT)) > options.update_older_than * 60 * 60 * 24), options.hosts)
    if options.host_filter:
        if options.action not in ["update", "updatelocal", "updatehb", "updateoops"]:
            raise Exception("Option --host-filter compatible only with update/updatelocal options")
        options.hosts = map(lambda x: x.name,
                            filter(options.host_filter, options.db.hosts.get_hosts_by_name(options.hosts)))

    if options.update_fields is not None:
        if isinstance(options.update_fields, (str, unicode)):
            options.update_fields = options.update_fields.split(',')

        avail_fields = list(x.name for x in get_host_fields_info() if x.primary)
        unknown_fields = filter(lambda x: x not in avail_fields, options.update_fields)
        if len(unknown_fields) > 0:
            raise Exception("Unknown fields to update: <%s>" % (",".join(unknown_fields)))

    return options


def return_none(x):
    return None


def main(options, from_cmd):
    normalize(options)

    funcs = {
        "invnum": return_none,
        "rack": return_none,
        "switch": return_none,
        "dc": return_none,
        "vlan": return_none,
        "golemowners": return_none
    }

    for elem in get_host_fields_info():
        key = elem.name
        if options.update_fields is not None and key not in options.update_fields:
            continue
        if key in ['domain', 'lastupdate', 'power', 'ncpu', 'switch_type', 'location', 'ssd_models', 'ssd_size', 'ssd_count',
                    'hdd_models', 'hdd_size', 'hdd_count', 'mtn_fqdn_version', 'change_time', 'invnum', 'rack',
                    'switch', 'dc', 'vlan', 'golemowners']:
            continue

        found = False
        for name, obj in inspect.getmembers(sys.modules[__name__]):
            if name == 'detect_%s' % key:
                funcs[key] = obj
                found = True
        if not found:
            raise Exception("Can not find function for detect_%s" % key)

    warnings = set()
    retval = None

    if options.action == "remove":
        for name in options.hosts:
            host = options.db.hosts.get_host_by_name(name)
            for group in options.db.groups.get_host_groups(host):
                if host in group.get_busy_hosts():
                    raise Exception("Trying to remove busy host %s in group %s" % (host.name, group.card.name))
                if group.card.host_donor is not None:
                    continue
                group.removeHost(host)
            options.db.hosts.remove_host(host)
    elif options.action in ["add", "update", "updatelocal", "updatehb", "updateoops"]:
        if options.action == "update" and options.hosts is None:
            options.hosts = map(lambda x: x.name, options.db.hosts.get_all_hosts())

        if len(options.hosts) == 0:
            print red_text("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
            print red_text("NOTHING TO UPDATE: GOT EMPTY HOST LIST")
            print red_text("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!")
            return None

        models = dict(map(lambda x: (x.fullname, x.model), options.db.cpumodels.models.values()))

        if options.action in ["add", "update"]:
            # have to update sys.path
            if sys.path[0] == _packages_path:
                sys.path.pop(0)
                sys.path.append(_packages_path)
            runner = TSkynetRunner(options, funcs, models)
        elif options.action == "updatehb":
            runner = THbRunner(options, funcs, models)
        elif options.action == "updateoops":
            runner = TOopsRunner(options, funcs, models)
        elif options.action == "updatelocal":
            if options.update_fields is None:
                funcs = dict(filter(lambda (name, pf): name in TLocalRunner.AVAIL_FUNCS, funcs.iteritems()))
            runner = TLocalRunner(options, funcs, models, nprocs=options.threads)
        else:
            raise Exception("Unknown action <%s>" % options.action)

        good_hosts = []
        updated_hosts = set()
        processed = 0
        extended_host_status = {}

        if len(options.hosts) > 0:
            for hostname, result, failure in runner.wait():
                if '.' not in hostname:
                    print('[WARN] Found invalid hostname: {}'.format(hostname))
                    continue

                processed += 1
                if failure is not None:
                    extended_host_status[hostname] = {'status': EStatuses.SKYNET_FAILED,
                                                      'err': formatException(failure)}

                    if options.verbose:
                        print 'HOST %s' % hostname
                        print formatException(failure)
                else:
                    extended_host_status[hostname] = {'status': EStatuses.SUCCESS}

                    result, host_warnings = result

                    if 'name' in funcs:
                        if 'name' not in result:
                            print red_text("Host %s did not detect its name" % hostname)
                        elif result['name'] != hostname:
                            print red_text("Host %s returned as %s" % (hostname, result['name']))
                        result['name'] = hostname

                    warnings |= set(host_warnings)

                    good_hosts.append(hostname)
                    if not options.db.hosts.has_host(hostname):
                        newhost = Host()

                        for k in result:
                            if k == 'change_time':
                                continue

                            value = result[k]
                            if k == 'unit' or k == 'switch':
                                value = bytes(value)

                            try:
                                setattr(newhost, k, value)
                            except:
                                print("setattr(newhost, k, value) error")
                                print(newhost, k, value)
                                setattr(newhost, k.encode('utf-8'), value.encode('utf-8'))

                        if 'name' not in result:
                            setattr(newhost, 'name', hostname)

                        if newhost.memory == 0:
                            newhost.memory = 10

                        for k in result.get('change_time', []):
                            newhost.change_time[k] = result['change_time'][k]

                        options.db.hosts.add_host(newhost)
                        newhost.postinit(options.db.cpumodels.models)
                        newhost.domain = '.' + newhost.name.partition('.')[2]
                    else:
                        if options.do_not_touch_existing:
                            continue

                        host = options.db.hosts.get_host_by_name(hostname)

                        update_fields = dict()
                        update_times = {}
                        for k in get_host_fields_info():
                            if k.name not in result or k.name in ('change_time',):
                                continue
                            elif k.name not in SUPPORT_UPDATE_TO_DEF_VALUE and \
                                    result[k.name] == k.def_value and not options.update_to_def_value:
                                continue
                            elif result[k.name] is None:
                                continue
                            elif getattr(host, k.name) == locate(k.type)(result[k.name]):
                                continue

                            if k.name in result.get('change_time', []):
                                last_change = to_datetime(host.change_time.get(k.name, '1970-01-01T00:00:00'))
                                update_time = to_datetime(result['change_time'][k.name])

                                if last_change >= update_time:
                                    continue
                                update_times[k.name] = result['change_time'][k.name]

                            update_fields[k.name] = locate(k.type)(result[k.name])

                            # TODO(shotinleg): Debug output
                            if k.name in update_times:
                                logging.info('Update <{}> for <{}>: {} ({} -> {})'.format(
                                    k.name, host.name, update_fields[k.name],
                                    host.change_time.get(k.name, '1970-01-01T00:00:00'), update_times[k.name]
                                ))

                        if len(update_fields):
                            if check_new_host_hw(options.db, host, update_fields, options):
                                applyed_updates = set()
                                for k in update_fields:
                                    # ====================================== RX-318 START ========================================
                                    if k != 'vlans':
                                        setattr(host, k, update_fields[k])
                                        # ====================================== RX-336 START ====================================
                                        if k == 'ipv6addr' and options.db.version >= '2.2.45':
                                            setattr(host, 'mtn_fqdn_version', 3)
                                        # ====================================== RX-336 FINISH ===================================
                                    else:
                                        old_value = getattr(host, k)
                                        if old_value.get('vlan688', None) != update_fields[k].get('vlan688', None):
                                            setattr(host, 'mtn_fqdn_version', 3)
                                        old_value.update(update_fields[k])
                                        setattr(host, k, old_value)
                                    # ====================================== RX-318 FINISH =======================================

                                host.change_time.update(update_times)

                                host.postinit(options.db.cpumodels.models)
                                if update_fields.keys() != ['lastupdate']:
                                    updated_hosts.add(host)

                if options.show_progress:
                    gaux.aux_utils.print_progress(processed, len(options.hosts))
        if options.show_progress:
            gaux.aux_utils.print_progress(processed, len(options.hosts), stopped=True)

        for hostname in options.hosts:
            if hostname not in extended_host_status:
                extended_host_status[hostname] = {'status': EStatuses.SKYNET_TIMEDOUT}

        bad_hosts = list(set(options.hosts) - set(good_hosts))

        if from_cmd or options.verbose:
            print "Overall statistics: %d processed, %d updated, %d fail" % (
            len(good_hosts), len(updated_hosts), len(bad_hosts))
            if len(bad_hosts):
                n_bad_hosts = len(bad_hosts)
                show_bad_hosts = bad_hosts[:100]
                if n_bad_hosts > len(show_bad_hosts):
                    show_bad_hosts.append("...")
                print "Hosts with no results (%d total): %s" % (n_bad_hosts, ','.join(show_bad_hosts))

        if options.reserved_group is not None:
            for hostname in good_hosts:
                options.reserved_group.addHost(options.db.hosts.get_host_by_name(hostname))

        if not from_cmd:
            retval = extended_host_status

    elif options.action == "addraw":
        for host_data in options.raw_data.split(';'):
            # host data is comma-separated list of key=value
            host_data = {x.partition('=')[0] : x.partition('=')[2] for x in host_data.split(',')}
            newhost = Host()
            for k, v in host_data.iteritems():
                if k in ('disk', 'ssd', 'memory', 'ssd_size', 'hdd_size', 'flags'):
                    v = int(v)
                setattr(newhost, k, v)
            newhost.domain = '.{}'.format(host_data['name'].partition('.')[2])

            newhost.postinit(options.db.hosts.cpu_models)
            options.db.hosts.add_host(newhost)

    elif options.action == "addcopy":
        fakehost = options.fake_host_copy_of
        template_data = dict(map(lambda x: (x.name, getattr(fakehost, x.name, x.def_value)), get_host_fields_info()))
        for hostname in options.hosts:
            template_data['name'] = hostname.partition('.')[0]
            if '.' in hostname:
                template_data['domain'] = '.' + hostname.partition('.')[2]
            else:
                template_data['domain'] = ''
            template_data['invnum'] = 'unknown'
            template_data['ipv4addr'] = 'unknown'
            template_data['ipv6addr'] = 'unknown'

            newhost = Host()
            for k, v in template_data.iteritems():
                setattr(newhost, k, v)

            options.db.hosts.add_host(newhost)

        warnings = ["Added hosts by copying %s" % options.fake_host_copy_of.name]
    elif options.action == "addfake":
        for hostname in options.hosts:
            newhost = Host()
            newhost.name = hostname
            newhost.domain = '.{}'.format(hostname.partition('.')[2])
            options.db.hosts.add_host(newhost)

        warnings = ["Added fake hosts"]
    elif options.action == "fixcard":
        host_field_info = None
        for elem in get_host_fields_info():
            if elem.name == options.card_field:
                host_field_info = elem
                break
        if host_field_info is None:
            raise Exception("There is not host card field <%s>" % options.card_field)

        for hostname in options.hosts:
            host = options.db.hosts.get_host_by_name(hostname)
            setattr(host, options.card_field, locate(host_field_info.type)(options.card_value))

        CURDB.hosts.mark_as_modified()
    else:
        raise Exception("Unknown action %s" % options.action)

    if from_cmd or options.verbose:
        print 'Warnings:'
        print '\n'.join(sorted(list(warnings)))

    if options.apply:
        options.db.hosts.mark_as_modified()
        options.db.update(smart=1)
    else:
        print red_text("Not updated!!! Add option -y to update.")

    return retval


def jsmain(d, from_cmd=True):
    options = get_parser().parse_json(d)
    normalize(options)
    return main(options, from_cmd)


if __name__ == '__main__':
    # oops = OopsApi()
    # print(json.dumps(oops.ip_info_for_vlans((688, 788), ['sas2-7037.search.yandex.net']), indent=4))
    logging.basicConfig(level=logging.INFO, format='%(levelname)-8s %(asctime)s  %(message)s')
    options = get_parser().parse_cmd()
    main(options, True)

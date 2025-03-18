import copy
import json
import os
import re
import time
import urllib2
from collections import defaultdict

import gaux.aux_decorators
from gaux.aux_colortext import red_text
from core.card.types import ByteSize
try:
    from core.db import CURDB
except ImportError: # can not import when running cqueue.Run
    pass
from core.resources import TResources
from core.hosts import Host
from core.igroups import IGroup
from core.instances import Instance
from core.intlookups import Intlookup
from core.audit.cpu import TCpuSuggest
from core.nanny_services import NannyService
from core.hbfmacroses import HbfMacros
from core.hbfranges import HbfRange

# =============================================================================
# Helper functions
# =============================================================================
def split_by_delims(s, delims='(?:\s|,|;)+'):
    return re.split(delims, s.strip())


class HostsOrInstances(object):
    """
        Type for host or instances. Convert string, containing comma-separated hosts or instsnces to
        host and instances. Used to check if specified instance is among specified ones.
    """

    def __init__(self, data):
        self.hosts = set()
        self.instances = defaultdict(list)
        for elem in data.split(','):
            if elem.find(':') > 0:
                hostname, _, port = elem.partition(':')
                hostname = CURDB.hosts.resolve_short_name(hostname)
                self.instances[hostname].append(int(port))
            else:
                hostname = CURDB.hosts.resolve_short_name(elem)
                self.hosts.add(hostname)

    def has_instance(self, instance):
        if instance.host.name in self.hosts:
            return True
        if instance.port in self.instances[instance.host.name]:
            return True
        return False

    def get_all_instances(self, db=CURDB):
        all_hosts = db.hosts.get_hosts_by_name(list(self.hosts | set(self.instances.keys())))
        all_instances = sum(map(lambda x: db.groups.get_host_instances(x), all_hosts), [])
        return filter(lambda x: self.has_instance(x), all_instances)


@gaux.aux_decorators.hide_value_error
def argparse_hosts_or_instances(s):
    return HostsOrInstances(s)


@gaux.aux_decorators.hide_value_error
def intlookup(s):
    """
        Intlookup, loaded via api.

        :type s: str

        :param s: string with intlookup names
        :return (core.intlookups.Intlookup): intlookup object
    """
    if isinstance(s, Intlookup):
        return s
    elif CURDB.intlookups.has_intlookup(s):
        return CURDB.intlookups.get_intlookup(s)
    elif CURDB.groups.has_group(s):  # got group instead of intlookup: return its intlookup
        group = CURDB.groups.get_group(s)
        assert len(group.card.intlookups) == 1, "Group %s has %d (%s) intlookups when need exactly one" % (
            group.card.name, len(group.card.intlookups), ", ".join(group.card.intlookups))
        return CURDB.intlookups.get_intlookup(group.card.intlookups[0])
    else:
        raise Exception('Do not know how to convert <{}> (type {}) to intlookup'.format(s, type(s)))


def read_list(str_value):
    if str_value.startswith('#'):
        with open(str_value[1:]) as rfile:
            return [x.strip() for x in rfile.read().replace('\n', ',').split(',') if x.strip()]
    return filter(str.strip, str_value.split(','))


@gaux.aux_decorators.hide_value_error
def group_t(str_value):
    return CURDB.groups.get_group(str_value)


@gaux.aux_decorators.hide_value_error
def groups_t(str_value):
    if str_value == 'ALL':
        return CURDB.groups.get_groups()
    return map(CURDB.groups.get_group, read_list(str_value))


@gaux.aux_decorators.hide_value_error
def hosts_t(str_value):
    all_hosts = {x.invnum: x for x in CURDB.hosts.get_hosts()}
    if str_value == 'ALL':
        return all_hosts.values()

    hosts = []
    for host_id in read_list(str_value):
        # Host by invnum
        if host_id.isdigit():
            hosts.append(all_hosts[int(host_id)])
        # Hosts from group
        elif host_id.upper() == host_id:
            hosts += CURDB.groups.get_group(host_id).getHosts()
        # Host by fqdn
        else:
            hosts.append(CURDB.hosts.get_host_by_name(host_id))
    return hosts


@gaux.aux_decorators.hide_value_error
def intlookups(s):
    """
        List of intlookups, loaded from comma-separated intlookup names.

        :type s: str

        :param s: string with comma-separated list of intlookup names
        :return (list of core.intlookups.Intlookup): list of intlookup objects
    """
    if s == 'ALL':
        return CURDB.intlookups.get_intlookups()
    elif isinstance(s, list):
        return map(lambda x: intlookup(x), s)
    else:
        return map(lambda x: intlookup(x), s.split(','))


@gaux.aux_decorators.hide_value_error
def sasconfig(s):
    """
        Yaml-config for 'sas' optimization algorightm (examples in optimizers/sas/configs). Used by sas_optimizer
        as main config and variety of scripts to find specific project intlookups.

        :type s: str

        :param s: Yaml file with config for sas optimizer
        :return (card_node.CardNode): yaml, converted to our internal card node format
    """
    from core.card.node import load_card_node
    return load_card_node(s, os.path.join(CURDB.SCHEMES_DIR, 'sasconfig.yaml'))


@gaux.aux_decorators.hide_value_error
def sasconfigs(s):
    """
        List of sas optimization configs, loaded from comma-separated list of files.

        :type s: str

        :param s: comma-separated list of yaml files
        :return (list of card_node.CardNode): list of yaml, converted to our internal card node format
    """
    return map(lambda x: sasconfig(x), split_by_delims(s))


@gaux.aux_decorators.hide_value_error
def pythonlambda(s):
    """
        Python lambda function. We should check if this function is safe (do not cause any external
        effects like 'rm -rf /'

        :type s: str

        :param s: string with python lambda function like 'lambda x: x + 10'
        :return (python function): function loaded from input string
    """
    if isinstance(s, str):
        return eval(s)
    else:
        return s


@gaux.aux_decorators.hide_value_error
def pythonfunc(s):
    """
        Extremely unsafe python funcction, loaded from input string. Could use locals and globals
        during execution

        :type s: str

        :param s: string with python lambda function
        :return (python function): function loaded from input string
    """
    gl = {}
    lo = {}
    exec (s, gl, lo)

    if len(lo.keys()) > 1:
        raise Exception("Multiple functions defined: %s" % ",".join(lo.keys()))
    elif len(lo.keys()) == 0:
        raise Exception("No functions defined in code %s" % s)

    return lo.values()[0]


@gaux.aux_decorators.hide_value_error
def host(s):
    """
        Host, loaded from its name. Must use fqdn here

        :type s: str

        :param s: string with host names
        :return (core.hostsinfo.Host): gencfg host structure
    """
    if CURDB.hosts.has_host(s):
        return CURDB.hosts.get_host_by_name(s)
    else:
        # try to find hosts starting with s (support short names)
        candidates = filter(lambda x: x.startswith('%s.' % s) or x == s, CURDB.hosts.host_names)
        if len(candidates) == 1:
            return CURDB.hosts.get_host_by_name(candidates[0])
        elif len(candidates) == 0:
            raise Exception("Host %s not found" % s)
        else:
            raise Exception("Multiple candidates for host short names %s" % s)


def normalize_hostname(hostname, inv_to_host_map=None):
    """
        Host names can be presented in various formats:
            - by its real gencfg name: <name.search.yandex.net>
            - by short name: <name>
            - by host invnum
            - all hosts from intlookup
            - some global constants (like <ALL>) converted to list of hosts

        :param hostname: hostname
        :return: list of hostnames from gencfg db (or None if host can not be converted)
    """

    hostname = hostname.strip()

    if hostname == 'ALL': # global constant
        return map(lambda x: x.name, CURDB.hosts.get_all_hosts())
    elif hostname.find('@') >= 0: # skynet blinov format
        from library.sky.hostresolver import Resolver
        return map(lambda x: x.split('.')[0], Resolver().resolveInstances(s).keys())
    elif CURDB.hosts.has_host(hostname): # have host in db
        return [hostname]
    elif CURDB.intlookups.has_intlookup(hostname): # have intlookup with such name
        instances = CURDB.intlookups.get_intlookup(hostname).get_base_instances()
        return list(set(map(lambda x: x.host.name, instances)))
    elif CURDB.groups.has_group(hostname): # have group with such name
        return map(lambda x: x.name, CURDB.groups.get_group(hostname).getHosts())

    if inv_to_host_map:
        res = inv_to_host_map.get(hostname)
        if res is not None:
            return [res]
    else:
        res = filter(lambda x: x.invnum == hostname, CURDB.hosts.get_hosts())
        if res:
            return res

    if (hostname in CURDB.hosts.fqdn_mapping):  # find host by its short name
        return [CURDB.hosts.fqdn_mapping[hostname]]

    return None


def _load_inv_to_host_map():
    res = {}

    for host in CURDB.hosts.get_hosts():
        res[host.invnum] = host.name

    return res


@gaux.aux_decorators.hide_value_error
@gaux.aux_decorators.loadfromfile
def hostnames(s):
    """
        List of hosts, loaded from comma- or space- separated list of host names. Also can use skynet api
        to resolve bsconfig instances

        :type s: str

        :param s: string with list of host names
        :return (list of str): list of resolved host names
    """
    try:
        if isinstance(s, list):
            return copy.copy(s)
        elif isinstance(s, str):
            inv_to_host_map = _load_inv_to_host_map()

            result = []
            for candidate in filter(lambda x: x != '', split_by_delims(s)):
                converted = normalize_hostname(candidate, inv_to_host_map=inv_to_host_map)

                if converted is not None:
                    result.extend(converted)
                else: # do not have host with such name in db, just leave unchanged
                    result.append(candidate)
            return result
        else:
            raise Exception("Unknown type <%s> of input data while parsing hostnames" % (type(s)))
    except Exception, e:
        print e
        raise

@gaux.aux_decorators.hide_value_error
def hosts(s):
    """
        List of hosts, loaded from comma- or space- separated host names. Also can use inventory numbers instead
        of names, e.g. 'ws2-200.yandex.ru,sas1-0110.search.yandex.net,3234652362,man1-0500.search.yandex.net'

        :type s: str

        :param s: string is comma- or space- separated list of host names
        :return (list of core.hostsinfo.Host): list of gencfg host structures
    """

    if isinstance(s, str):
        candidates = hostnames(s)

        result = []
        notfound = []

        for candidate in candidates:
            # check hostname
            if CURDB.hosts.has_host(candidate):
                result.append(CURDB.hosts.get_host_by_name(candidate))
                continue

            # check inventory number
            filtered = filter(lambda x: x.invnum == candidate, CURDB.hosts.get_hosts())
            if len(filtered) == 1:
                result.append(filtered[0])
                continue

            # check intlookup and add all intlookup basesearchers
            # looks weird but ...
            if CURDB.intlookups.has_intlookup(candidate):
                instances = CURDB.intlookups.get_intlookup(candidate).get_base_instances()
                hosts = list(set(map(lambda x: x.host, instances)))
                result.extend(hosts)
                continue

            # check groups and add all group hosts
            if CURDB.groups.has_group(candidate):
                hosts = CURDB.groups.get_group(candidate).getHosts()
                result.extend(hosts)

            notfound.append(candidate)

        if len(notfound) > 0:
            raise Exception("Hosts <%s> not found in db" % (",".join(notfound)))

    elif isinstance(s, (list, tuple)):
        result = []
        for elem in s:
            if isinstance(elem, Host):
                result.append(elem)
            elif isinstance(elem, str):
                result.append(CURDB.hosts.get_host_by_name(elem))
            else:
                raise Exception("Do not know how to convert <%s> to host" % (type(s)))
    else:
        raise Exception("Do not know how to convert type <%s> to list of hosts" % (type(s)))

    return result


@gaux.aux_decorators.hide_value_error
def group(s):
    """
        Convert string gencfg group name to group.

        :type s: str

        :param s: group name
        :return (core.igroups.IGroup): gencfg group structure
    """
    if isinstance(s, str):
        return CURDB.groups.get_group(s)
    elif isinstance(s, IGroup):
        return s
    else:
        raise Exception("Got unknown type <%s> while converting to gencfg group" % (type(s)))


@gaux.aux_decorators.hide_value_error
def hbf_range(s):
    """
        Convert string hbf range name to hbf range object.
        :type s: str
        :param s: hbf range name
        :return (core.hbfranges.HbfRange): gencfg hbf range structure
    """
    if isinstance(s, str):
        return CURDB.hbfranges.get_range_by_name(s)
    elif isinstance(s, HbfRange):
        return s
    else:
        raise Exception("Got unknown type <%s> while converting to gencfg hbf range" % (type(s)))


@gaux.aux_decorators.hide_value_error
@gaux.aux_decorators.loadfromfile
def groups(s):
    """
        Convert comma- separated list of group names to list of gencfg groups.

        :type s: str

        :param s: comma-separated list of group names
        :return (list of core.igroups.IGroup): list of gencfg group structure
    """
    if s == 'ALL':
        return CURDB.groups.get_groups()
    if s == 'ALLM':
        return filter(lambda x: x.card.master is None, CURDB.groups.get_groups())

    if isinstance(s, str):
        return map(lambda x: CURDB.groups.get_group(x), split_by_delims(s))

    if isinstance(s, list):
        ret = s
        if len(s) > 0 and isinstance(s[0], str):
            ret = map(lambda x: CURDB.groups.get_group(x), ret)
        return ret

    raise Exception("Can not convert type <%s> to groups" % (type(s)))


@gaux.aux_decorators.hide_value_error
def nanny_service(s):
    """Convert string to nanny service"""
    if isinstance(s, str):
        return CURDB.nanny_services.get(s)
    elif isinstance(s, NannyService):
        return s
    else:
        raise Exception("Got unknown type <%s> while converting cached nanny service" % (type(s)))


@gaux.aux_decorators.hide_value_error
@gaux.aux_decorators.loadfromfile
def nanny_services(s):
    """
        Convert comma- separated list of nanny service names to list of cached nanny services.

        :type s: str

        :param s: comma-separated list of nanny services names
        :return (list of core.igroups.IGroup): list of gencfg group structure
    """
    if s == 'ALL':
        return CURDB.nanny_services.get_all()

    if isinstance(s, str):
        return map(lambda x: CURDB.nanny_services.get(x), split_by_delims(s))

    if isinstance(s, list):
        ret = s
        if len(s) > 0 and isinstance(s[0], str):
            ret = map(lambda x: CURDB.nanny_services.get(x), ret)
        return ret

    raise Exception("Can not convert type <%s> to nanny services" % (type(s)))


@gaux.aux_decorators.hide_value_error
@gaux.aux_decorators.loadfromfile
def grouphosts(s):
    """
        Comma-separated list of groups or hosts. Treated as list of hosts.
        Example: MSK_WEB_BASE,ws2-200.yandex.ru,sas1-0200.search.yandex.net

        :type s: str

        :param s: comma-separated list of group or host names
        :return (list of core.hostsinfo.Host): list of gencfg host structure
    """
    if isinstance(s, str):
        if s == 'ALL':
            return CURDB.hosts.get_all_hosts()
        else:
            result = []
            for elem in split_by_delims(s):
                if elem.isupper():
                    result.extend(CURDB.groups.get_group(elem).getHosts())
                else:
                    result.append(CURDB.hosts.get_host_by_name(elem))
            return list(set(result))
    elif isinstance(s, list):
        result = []
        for elem in s:
            if isinstance(elem, IGroup):
                result.extend(elem.getHosts())
            elif isinstance(elem, Host):
                result.append(elem)
            elif isinstance(elem, str):
                result.append(CURDB.hosts.get_host_by_name(elem))
            else:
                raise Exception("Do not know how to covert type <%s> to host" % (type(elem)))
    else:
        raise Exception("Do not know how to convert type <%s> to grouphosts" % (type(s)))


@gaux.aux_decorators.hide_value_error
@gaux.aux_decorators.loadfromfile
def xgroups(s):
    """
        Comma-separated list of igroups regexp (like SAS_.*,.*_BASE,...).

        :type s: str

        :param s: list of comma-separated regexps on group name
        :return (list of core.igroups.IGroup): list of gencfg groups
    """

    if s == 'ALL':
        return CURDB.groups.get_groups()
    if s == 'ALLM':
        return filter(lambda x: x.card.master is None, CURDB.groups.get_groups())

    allgroups = CURDB.groups.get_groups()

    result = []
    for flt in split_by_delims(s):
        matched = filter(lambda x: re.match(flt + '$', x.card.name), allgroups)
        if len(matched) == 0:
            raise Exception("Can not match anything to %s" % flt)
        result.extend(matched)
    result = list(set(result))

    return sorted(result, cmp=lambda x, y: cmp(x.card.name, y.card.name))


@gaux.aux_decorators.hide_value_error
def stripped_searcherlookup(s):
    """
        Searcherlookup, loaded from searcherlookup.conf . Used in some check script

        :type s: str

        :param s: filename with generated searcherlookup
        :return (searcherlookup_generator.StrippedSearcherlookup): searcherlookup with somewhat fast access to tag list
    """
    from core.searcherlookup import StrippedSearcherlookup
    return StrippedSearcherlookup(s)


@gaux.aux_decorators.hide_value_error
@gaux.aux_decorators.loadfromfile
def instances(s, db=CURDB, raise_failed=True, allow_fake_instances=False):
    """
        List of instances in format <host:port> or <host:port:power:type>
        List can be comma- or space-separated

        :type s: str
        :type db: core.DB
        :type raise_failed: bool
        :type allow_fake_instances: bool

        :param s: comma- or space- separated list of instances
        :param db: database to load instances from. Use db in current repository by default
        :param raise_failed: raise if some of instances not found in db
        :param allow_fake_instances: if instance not found, create FakeInstance object instead of Instance

        :return (list of (core.instances.Instance or core.instances.FakeInstance)): list of instances
    """
    if isinstance(s, str):
        s = split_by_delims(s)

    result = []
    for instance in s:
        if isinstance(instance, Instance):
            result.append(instance)
        elif isinstance(instance, str):
            host, port = db.hosts.get_host_by_name(instance.split(':')[0]), int(instance.split(':')[1])
            try:
                result.append(filter(lambda x: x.port == port, db.groups.get_host_instances(host))[0])
            except:
                if raise_failed:
                    print red_text("Instance %s:%s not found" % (host.name, port))
                    raise
                if allow_fake_instances:
                    result.append(Instance(host, 1.0, port, 'UNKNOWN', 0))
        else:
            raise Exception("Do not know how to convert type <%s> to instance" % (type(instance)))

    return result


@gaux.aux_decorators.hide_value_error
@gaux.aux_decorators.loadfromfile
def extended_instances(s, db=CURDB):
    """
        List of instances with may be fake/unexisting ones. Simply call istances func with params

        :type s: str

        :param s: comma- or space- separated list of instances
        :return (list of (core.instances.Instance or core.instances.FakeInstance)): list of instances
    """
    return instances(s, db=db, raise_failed=False, allow_fake_instances=True)


@gaux.aux_decorators.hide_value_error
def yamlconfig(s):
    """
        Yaml config, loaded from string with file name

        :type s: str

        :param s: yaml file name
        :return (yaml.Yaml): loaded yaml
    """
    from pkg_resources import require
    require('PyYAML')
    import yaml

    return yaml.load(open(s).read())


@gaux.aux_decorators.hide_value_error
def machinemodel(s):
    """
        Full cpu model description, converted from cpu name. All available models are stored in
        db/hardware_data/models.yaml and presented as card_node.CardNode list

        :type s: str

        :param s: valid model name
        :return (card_node.CardNode): card node, describing specified model
    """
    if s not in CURDB.cpumodels.models:
        raise Exception("Unknown model %s" % s)
    return CURDB.cpumodels.get_model(s)


@gaux.aux_decorators.hide_value_error
def machinemodels(s):
    """
        List of cpu models description, converted from comma- separated cpu names. All available models are stored in
        db/hardware_data/models.yaml and presented as card_node.CardNode list

        :type s: str

        :param s: valid model name
        :return (list of card_node.CardNode): list of card nodes, describing specified models
    """

    return map(lambda x: machinemodel(x), split_by_delims(s))


@gaux.aux_decorators.hide_value_error
def analyzer_database(s):
    """
        Load sqlite database, generated by analyzer/main.py to dict: (host, port) -> dict with sql field/value.

        :type s: str

        :param s: file name with sqlite database
        :return (dict): dict with all instances and theirs data
    """

    all_instances = dict(map(lambda x: (x.name(), x), CURDB.groups.get_all_instances()))

    import sqlite3
    conn = sqlite3.connect(s)
    cursor = conn.execute("SELECT * FROM data")

    fields = map(lambda x: x[0], cursor.description)

    result = {}
    for elems in cursor.fetchall():
        elems_as_dict = dict(zip(fields, elems))
        elems_as_dict['host'] = CURDB.hosts.get_host_by_name(elems_as_dict['host'])
        result[all_instances['%s:%s' % (elems_as_dict['host'].name, elems_as_dict['port'])]] = elems_as_dict
    return result


@gaux.aux_decorators.hide_value_error
def tier(s):
    """
        Gencfg internal tier structure loaded from its name

        :type s: str

        :param s: tier name
        :retrun (core.hostcfg.Tier): gencfg tier internal structure
    """
    from core.db import CURDB
    return CURDB.tiers.get_tier(s)


@gaux.aux_decorators.hide_value_error
def tiers(s):
    """
        List of gencfg tiers, loaded from comma- separated list

        :type s: str

        :param s: comma- separated list of tier names
        :retrun (list of core.hostcfg.Tier): list of gencfg tier internal structure
    """
    return map(lambda x: tier(x), split_by_delims(s))


@gaux.aux_decorators.hide_value_error
def some_content(path):
    """
        Wrapper around some source of data, which can be remote http url of file name

        :type path: str

        :param path: http url of file name of local filesystem
        :return (str): content of remote url/local file
    """
    if path.startswith("http://") or path.startswith("https://"):
        return urllib2.urlopen(path, timeout=10).read()
    else:
        path = os.path.expanduser(path)
        return open(path).read()


@gaux.aux_decorators.static_var("description",
                               """Extended timestamp. Can be specified in different formats:
<ul>
   <li><b>1428343437</b> : 1428343437 seconds since epoch</li>
   <li><b>4h</b> : 4 hours ago</li>
   <li><b>3d</b> : 3 days ago</li>
   <li><b>11w2d</b> - 11 weeks and 2 days ago</li>
   <li><b>3m4d5h</b> : 94 days 5 hours ago</li>
   <li><b>2014-10-07 22:00</b> : standard date format</li>
</ul>""")
def xtimestamp(s):
    # check if we already converted timestamp
    if isinstance(s, int):
        return s

    # match seconds since epoch
    m = re.match('^[0-9]+$', s)
    if m:
        if int(s) < 1000:
            raise Exception("Too small value <%s> in seconds since epoch" % s)

        return int(s)

    # match standard format
    try:
        t = time.mktime(time.strptime(s, "%Y-%m-%d %H:%M"))
        return int(t)
    except ValueError:
        pass

    # match now
    if s == 'now':
        return int(time.time())

    # match days before now
    result = int(time.time())
    MULTS = [
        ('m', 60 * 60 * 24 * 30),
        ('w', 60 * 60 * 24 * 7),
        ('d', 60 * 60 * 24),
        ('h', 60 * 60),
    ]
    for suffix, multiplier in MULTS:
        m = re.match('^([0-9]+)(%s).*' % suffix, s)
        if m:
            result -= int(m.group(1)) * multiplier
            s = s[len(m.group(1)) + len(m.group(2)):]
    if len(s) > 0:
        raise Exception("Can not parse <%s>" % s)
    return result


@gaux.aux_decorators.hide_value_error
def primus_int_count(s, db=CURDB):
    """
        Expression to caclulate number of shards
        Examples:
         - "123" : 123 shards
         - "RRGTier0" : number of rrg shards (1008 currently)
         - VideoQuickTier0*2+VideoUltraTier0: sum of VideoQuickTier0, VideoQuickTier0 and VideoUltraTier0

        :type s: str
        :type db: core.db.DB

        :param s: string with expression
        :param db: gencfg database (current db by default)
        :return (int): total number of shards, calculated by expression
    """
    return CURDB.tiers.primus_int_count(s)[0]


@gaux.aux_decorators.hide_value_error
@gaux.aux_decorators.loadfromfile
def floaded_str(s):
    """
        Input string. Can be replaced by file content if starts with '#'
    """
    return s


@gaux.aux_decorators.hide_value_error
@gaux.aux_decorators.loadfromfile
def comma_list(s):
    """
        Comma or other delimiter separated list of strings
    """
    if isinstance(s, (list, tuple)):
        return copy.copy(s)
    elif s == '':
        return []
    else:
        return split_by_delims(s)


@gaux.aux_decorators.hide_value_error
def comma_list_ext(s, deny_repeated=True, allow_list=None):
    """
        Comma or other delimiter separated list of strings with extra checkers
    """

    result = comma_list(s)

    if deny_repeated:
        repeated_values = set([x for x in result if result.count(x) > 1])  # FIXME: too slow
        if len(repeated_values):
            raise Exception("Repeated values <%s> are not allowed" % ",".join(repeated_values))

    if allow_list is not None:
        disallow_list = set(result) - set(allow_list)
        if len(disallow_list):
            raise Exception("Unknown values <%s> are not allowed (allowed <%s>)" % (
                ",".join(disallow_list), ",".join(allow_list)))

    return result


@gaux.aux_decorators.hide_value_error
def gencfg_db(s):
    """
        Gencfg db, created from filesystem path

        :type s: str

        :param s: path go gencfg db
    """

    if isinstance(s, str):
        if s == 'CURDB':
            from core.db import CURDB
            return CURDB
        else:
            from core.db import DB
            return DB(s)
    else:
        return s


@gaux.aux_decorators.hide_value_error
def gencfg_prev_db(s):
    """
        We need separate function for previous db, because gencfg_db used exclusively for replacement of CURDB
    """

    return gencfg_db(s)


@gaux.aux_decorators.hide_value_error
@gaux.aux_decorators.loadfromfile
@gaux.aux_decorators.static_var("description", """Simple or complex json dict, eg '{ "a" : "b" , "c" : {"d" : 1 }}'""")
def jsondict(s):
    """
        Represent input string as dict structure. Support two formats:
            - usual json as presented
            - k1=v1,k2=v2,...,kn=vn for dicts with depth equal to 1
    """
    if isinstance(s, str):
        try:
            result = json.loads(s)
        except ValueError: #
            result = s.split(',')
            result = map(lambda x: (x.partition('=')[0], x.partition('=')[2]), result)
            result = dict(result)

            # Try to guess value type by converting it to basic types
            for propname in result.keys():
                propvalue = result[propname]
                if propvalue == 'True':
                    propvalue = True
                elif propvalue == 'False':
                    propvalue = False
                else:
                    try:
                        propvalue = int(propvalue)
                    except:
                        try:
                            propvalue = float(propvalue)
                        except:
                            pass
                result[propname] = propvalue

        result = jsondict_byteify(result)
        if not isinstance(result, dict):
            raise Exception("Non-dict type in json <%s>" % s)

    elif isinstance(s, dict):
        result = s
    else:
        raise Exception("Do not know how to convert type <%s> to jsondict" % type(s))

    return result

@gaux.aux_decorators.hide_value_error
def kvlist(s):
    """
        Represents input data in format key1=value1,key2=value2,... as dict
    """
    if isinstance(s, dict):
        return s
    elif isinstance(s, str):
        result = {}

        # split by comma, do not count <\,> as regular comma, replace <\,> by ,
        slice_indices = [0] + [x.end(1) for x in re.finditer('[^\\\\](,)', s)]
        elems = [s[i:j-1] for i,j in zip(slice_indices, slice_indices[1:]+[len(s)+1])]
        elems = [x.replace('\\,', ',') for x in elems]

        for elem in elems:
            if elem.find('=') < 0:
                raise Exception("Can not convert string <%s> to kvlist" % s)

            k, _, v = elem.partition("=")
            if k in result:
                raise Exception("Key <%s> found twice in <%s>" % (k, s))

            result[k] = v

        return result
    else:
        raise Exception("Do not know how to convert type <%s> to kvlist" % (type(s)))


@gaux.aux_decorators.hide_value_error
def jsondict_byteify(d):
    """Convert all unicode strings to normal strings recursively"""
    if isinstance(d, dict):
        return {jsondict_byteify(key): jsondict_byteify(value)
                for key, value in d.iteritems()}
    elif isinstance(d, list):
        return [jsondict_byteify(element) for element in d]
    elif isinstance(d, unicode):
        return d.encode('utf-8')
    else:
        return d


def byte_size(s):
    """Convert strings like <12 Gb> , <11Mb> to core.card.types.ByteSize structure"""
    if isinstance(s, ByteSize):
        return s
    else:
        return ByteSize(s)


def cpu_suggest(s):
    """Convert data into cpu suggest info"""
    if isinstance(s, TCpuSuggest):
        return s
    elif isinstance(s, str):
        return TCpuSuggest.from_string(s)
    else:
        raise Exception('Do not know how to convert type <{}> (value <{}>) to TCpuSuggest'.format(type(s), s))


@gaux.aux_decorators.loadfromfile
def cpu_suggest_list(s):
    """Convert data into list of cpu suggest info"""
    if isinstance(s, list):
        return [cpu_suggest(x) for x in s]
    elif s == '':
        return []
    else:
        return [cpu_suggest(x) for x in split_by_delims(s)]


@gaux.aux_decorators.hide_value_error
def gencfg_hbf_macros(s):
    if isinstance(s, HbfMacros):
        return s

    return CURDB.hbfmacroses.get_hbf_macros(s)


@gaux.aux_decorators.hide_value_error
def gencfg_resources(s):
    """Create structure with resources from line"""
    return TResources.from_line(s)

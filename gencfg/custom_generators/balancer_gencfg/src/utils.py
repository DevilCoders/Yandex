#!/skynet/python/bin/python
# -*- coding: utf8 -*-

from __future__ import print_function

import os
import re
import socket
import sys
import json
import urllib2
import time
import random
import requests
import copy
import collections
import logging
import cPickle
import traceback

from collections import OrderedDict
from src.lua_globals import LuaGlobal, LuaAnonymousKey
from src.transports import InstanceDbTransportHolder
from optionschecker import Helper


def get_api_searcherlookup(gname, tag):
    tout = 0.1
    last_e = None

    url = 'http://api.gencfg.yandex-team.ru/{path}/searcherlookup/groups/{group}/instances'.format(
        path=('trunk' if tag == 'trunk' else 'tags/{}'.format(tag)),
        group=gname,
    )

    while tout <= 11:
        try:
            r = requests.get(url)
            r.raise_for_status()
            return json.loads(r.content)
        except Exception as e:
            last_e = e
            time.sleep(tout)
            tout *= 10

    raise Exception('Failed to fetch %s searcherlookup from tag %s: %s' % (gname, tag, str(last_e)))


class ErsatzHostName:
    def __init__(self, hostname):
        self.name = hostname


class ErsatzIntGroup:
    def __init__(self, fqdn, port, power):
        self.host = ErsatzHostName(fqdn)
        self.port = port
        self.power = power

    def name(self):
        return '{}:{}'.format(self.host.name, self.port)

    def get_host_name(self):
        return self.host.name


class ErsatzIntlookup:
    def __init__(self, group_name, tag):
        (meta, sorted_shard_instances) = intlookup_from_searcherlookup(get_api_searcherlookup(group_name, tag))
        self.shards_count = len(sorted_shard_instances) * meta["hosts_per_group"]
        self.group_name = meta["file_name"]
        self.shard_instances = sorted_shard_instances
        self.file_name = group_name

    def get_shards_count(self):
        return self.shards_count

    def get_base_instances_for_shard(self, shard_number):
        return self.shard_instances[shard_number]


def intlookup_from_searcherlookup(searcherlookup):
    brigades = collections.defaultdict(list)
    tiers = set([])
    # FIXME
    # switch = ""
    hosts_per_group = 1
    group_name = ""
    for instance in searcherlookup['instances']:
        tags = instance['tags']
        for tag in tags:
            if tag.startswith('a_topology_group-'):
                group_name = tag[len('a_topology_group-'):]
            elif tag.startswith('a_tier_'):
                tiers.add(tag[len('a_tier_'):])
        fqdn = instance['hostname']
        port = instance['port']
        power = instance['power']
        # dc = instance['dc']
        # queue = instance['queue']
        brigades[instance['shard_name']].append(ErsatzIntGroup(fqdn, port, power))
    meta = {
        "file_name": group_name,
        "hosts_per_group": hosts_per_group,
        "brigade_groups_count": len(brigades),
        "base_type": group_name,
        "tiers": [tier for tier in tiers],
    }
    return (meta, [v for k, v in sorted(brigades.items())])


class DummyInstance(object):
    def __init__(
        self, hostname, port, power,
        dc='unknown',
        cached_ip=None,
        ipv4addr=None,
        ipv6addr=None,
        hbf_mtn_hostname=None,
        hbf_mtn_ipv6addr=None,
        tags=None,
    ):
        self.hostname = hostname
        self.dc = dc
        self.port = port
        self.power = power
        self.cached_ip = cached_ip
        self.ipv4addr = ipv4addr
        self.ipv6addr = ipv6addr
        self.hbf_mtn_hostname = hbf_mtn_hostname
        self.hbf_mtn_ipv6addr = hbf_mtn_ipv6addr
        if tags is None:
            self.tags = set()
        else:
            self.tags = copy.copy(tags)

        if self.cached_ip is None and self.ipv6addr is not None:
            self.cached_ip = self.ipv6addr
        if self.cached_ip is None and self.ipv4addr is not None:
            self.cached_ip = self.ipv4addr


def load_params(gname):
    if gname.find('(') < 0:
        return gname, {}

    props = gname.split('(')[1].split(')')[0]
    props = map(lambda x: (x.partition('=')[0].strip(), x.partition('=')[2].strip()), props.split(','))

    gname = gname.split('(')[0]

    return gname, props


# FIXME: dublicate from aux_utils


def retry_urlopen(retries, *args, **kwargs):
    for i in range(retries):
        try:
            ret = GENCFG_CACHE.get((args, kwargs))
            if not ret:
                ret = urllib2.urlopen(*args, **kwargs)
                GENCFG_CACHE.put((args, kwargs), ret)
        except Exception:
            pass

    raise SystemExit("Failed {} attemps in while getting {}".format(retries, args[0]))


def get_instances(groups, override_domain=None):
    if not hasattr(get_instances, 'memodict'):
        get_instances.memodict = dict()

    memokey = (tuple(groups), override_domain)
    if memokey not in get_instances.memodict:
        get_instances.memodict[memokey] = get_instances_impl(groups, override_domain=override_domain)

    return get_instances.memodict[memokey]


class GenCfgCache(object):

    def __init__(self):
        self.file_name = None
        self.cache = {}
        self.gencfg_times = {}
        self.hits = 0
        self.misses = 0
        self.requests = 0
        self.enabled = True
        self.using_only_cache = False

    def disable(self):
        self.enabled = False

    def use_only_cache(self):
        self.using_only_cache = True

    def load(self, file_name=None):
        self.file_name = file_name
        if not file_name:
            logging.info("Cache file name is empty, nothing to load")
            return

        if os.path.exists(file_name):
            logging.info("Loading cache from %s", file_name)
            with open(file_name) as f:
                self.cache = cPickle.load(f)
        else:
            logging.info("Cache file %s does not exists, skipping", file_name)

    def get(self, key):
        self.requests += 1
        if self.enabled and key in self.cache:
            self.hits += 1
            return self.cache[key]
        if self.enabled and self.using_only_cache:
            raise Exception("{} is not in cache while using only cache".format(key))
        self.misses += 1
        return None

    def put(self, key, result, gencfg_time=0):
        self.cache[key] = result
        self.gencfg_times[key] = gencfg_time

    def save(self):
        if not self.enabled:
            return

        if not self.file_name:
            raise Exception("Trying to save cache to nowhere. ")

        logging.debug("Serializing cache of %s entries", len(self.cache))
        with open(self.file_name, 'wb') as f:
            # use highest pickle protocol
            cPickle.dump(self.cache, f, -1)

    def get_stats(self):
        return {
            "requests": self.requests,
            "hits": self.hits,
            "misses": self.misses,
            "entries": len(self.cache),
        }

    def print_stats(self):
        gencfg_times = sorted(
            [(self.gencfg_times[key], key) for key in self.gencfg_times],
            key=lambda x: x[0],
            reverse=True,
        )
        # print top-20 slowpokes
        for gencfg_time in gencfg_times[:20]:
            logging.debug("%10.2f %s", gencfg_time[0], gencfg_time[1])


GENCFG_CACHE = GenCfgCache()


def get_instances_impl(groups, override_domain=None):
    result = []
    for gname in groups:
        if not isinstance(gname, (str, unicode)):
            result.append(gname)
            continue

        if isinstance(gname, unicode):
            gname = gname.encode("utf8")

        gname, props = load_params(gname)
        if gname[0].isupper():  # generator group (either from trunk or tag

            instances = GENCFG_CACHE.get(gname)
            if not instances:
                start_time = time.time()
                logging.debug("Resolving group %s", gname)
                instances = InstanceDbTransportHolder.get_transport().get_group_instances(gname)
                GENCFG_CACHE.put(gname, instances, gencfg_time=time.time() - start_time)
            else:
                logging.debug("Group %s loaded from cache", gname)

        elif gname.startswith('intlookup:'):  # intlookup from gencfg
            intlookup_name, tp = gname.split(':')
            if tp not in ['base', 'int']:
                raise Exception("Unknown type for intlookup: %s" % tp)
            instances = InstanceDbTransportHolder.get_transport().get_intlookup_instances(intlookup_name, tp)

        elif gname.startswith('nannyservice:'):
            nanny_service = gname.partition(':')[2]

            requests.packages.urllib3.disable_warnings()  # black magic, omit ssl warnings
            session = None
            if not GENCFG_CACHE.using_only_cache:
                if 'NANNY_OAUTH_TOKEN' not in os.environ:
                    try:
                        import library.python.oauth as lpo
                        client_id = '59bc07e5a6744e99820b8b7ef42c8ae0'
                        client_secret = 'b1381677109343ca802fa6052498c7a8'
                        OAUTH_TOKEN = lpo.get_token(client_id, client_secret)
                    except Exception:
                        raise Exception(
                            "Variable <NANNY_OAUTH_TOKEN> with default oauth token is not found in environment "
                            "and no ssh key available"
                        )
                else:
                    OAUTH_TOKEN = os.environ['NANNY_OAUTH_TOKEN']
                session = requests.Session()
                session.headers['Authorization'] = 'OAuth {}'.format(OAUTH_TOKEN)
                session.headers['Content-Type'] = 'application/json'

            url = 'https://nanny.yandex-team.ru/v2/services/?category=/conductor/{}'.format(nanny_service)

            # make multiple retries to stop failing when conductor api flaps
            services = GENCFG_CACHE.get(url)
            if not services:
                start_time = time.time()
                for sleep_time in (1, 5, 10, 50, 250, 500, 500, 500, 500, 500):
                    try:
                        response = session.get(url, verify=False)
                        if response.status_code != 200:
                            raise Exception(
                                "Non-200 response code `{}` from Nanny on request `{}`".format(
                                    response.status_code, url
                                )
                            )
                        services = response.json()
                        if "error" in services:
                            raise Exception(
                                "Got error `{}` from Nanny on request `{}`".format(
                                    services["error"], url
                                )
                            )

                        GENCFG_CACHE.put(url, services, gencfg_time=time.time() - start_time)
                        break
                    except requests.exceptions.HTTPError:
                        time.sleep(sleep_time)
                else:
                    raise Exception('Failed to load url <{}>'.format(url))

            instances = []
            if 'result' not in services:
                raise RuntimeError('No result for service {}'.format(nanny_service))

            entries = services['result']
            if not entries:
                raise RuntimeError(
                    "No instances discovered resolving conductor service `{}`, "
                    "please check your configuration. ".format(nanny_service)
                )

            for entry in entries:
                dc = str(entry['_id'])
                for instance in entry['runtime_attrs']['content']['instances']['instance_list']:
                    instances.append(DummyInstance(
                        str(instance['host']), instance['port'], instance['weight'], dc=dc,
                        cached_ip=str(instance['ipv6_address']),
                        ipv4addr=str(instance['ipv4_address']) if 'ipv4_address' in instance else None,
                        ipv6addr=str(instance['ipv6_address']) if 'ipv6_address' in instance else None))

        elif gname.startswith('conline:'):  # GENCFG-421
            group = gname.partition(':')[2]

            # FIXME: get rid off code dublication from web
            url = "https://clusterstate.yandex-team.ru/api/v1/groups/{}/CURRENT".format(group)
            content = retry_urlopen(3, url, timeout=20).read()
            jsoned = json.loads(content)

            instances = []
            for data in jsoned["current"]["instances"]:
                host, port = data
                instances.append(DummyInstance(str(host), int(port), 1.))

        else:
            # custom delimiter for ipv6 addrs: we can not use ':' due to ambiguity
            if '@' in gname:
                parts = gname.split('@')
            else:
                parts = gname.split(':')

            if len(parts) == 2:
                host, port = parts
                instances = [DummyInstance(host, int(port), 1.)]
            else:
                host, port, weight = parts[:3]
                instances = [DummyInstance(host, int(port), float(weight))]

        for prop, prop_value in props:
            instances.sort(key=lambda x: '%s%s' % (x.hostname, x.port))
            if prop == 'location':
                instances = filter(lambda x: x.location == prop_value, instances)
            elif prop == 'percents':
                prop_value = int(prop_value)
                instances = map(lambda x: instances[x],
                                filter(lambda x: x * prop_value / 100 != (x - 1) * prop_value / 100,
                                       range(len(instances))))
            elif prop == 'luaport':
                instances = map(lambda x: DummyInstance(x.hostname, LuaGlobal(prop_value, x.port), x.power, x.dc,
                                                        cached_ip=getattr(x, 'cached_ip', None),
                                                        ipv4addr=getattr(x, 'ipv4addr', None),
                                                        ipv6addr=getattr(x, 'ipv6addr', None), tags=x.tags), instances)
            elif prop == 'shiftport':
                instances = map(lambda x: DummyInstance(x.hostname, x.port + int(prop_value), x.power, x.dc,
                                                        cached_ip=getattr(x, 'cached_ip', None),
                                                        ipv4addr=getattr(x, 'ipv4addr', None),
                                                        ipv6addr=getattr(x, 'ipv6addr', None), tags=x.tags), instances)
            elif prop == 'domain':
                instances = map(
                    lambda x: DummyInstance('%s%s' % (x.hostname.partition('.')[0], prop_value), x.port, x.power, x.dc,
                                            cached_ip=getattr(x, 'cached_ip', None),
                                            ipv4addr=getattr(x, 'ipv4addr', None),
                                            ipv6addr=getattr(x, 'ipv6addr', None), tags=x.tags), instances)
            elif prop == 'tag':
                data = retry_urlopen(
                    2,
                    "http://api.gencfg.yandex-team.ru/tags/{}/groups/{}/instances".format(
                        prop_value,
                        gname
                    ),
                    timeout=20
                ).read()
                data = json.loads(data)
                instances = map(lambda x: DummyInstance(str(x['hostname']), x['port'], x['power'], str(x['dc'])),
                                data['instances'])
            elif prop == 'resolve':
                families = map(lambda x: int(x), prop_value)
                for instance in instances:
                    instance.cached_ip = resolve_host(instance, families=families)
            # noiseless request
            elif prop == 'instancefilter':
                flt = eval(prop_value)
                instances = filter(flt, instances)
            elif prop == 'custompower':  # GENCFG-910
                for instance in instances:
                    instance.power = float(prop_value)
            elif prop == 'hbf_mtn' and (int(prop_value) == 1):  # GENCFG-974
                no_hbf_instances = [x for x in instances if x.hbf_mtn_hostname is None]
                if len(no_hbf_instances):
                    raise Exception('Can not generate hbf_mtn instances for {}'.format(
                        ' '.join(('{}:{}'.format(x.hostname, x.port) for x in no_hbf_instances)))
                    )
                instances = [
                    DummyInstance(
                        x.hbf_mtn_hostname,
                        x.port,
                        x.power,
                        dc=x.dc,
                        cached_ip=x.hbf_mtn_ipv6addr,
                        ipv4addr=None,
                        ipv6addr=x.hbf_mtn_ipv6addr,
                        tags=x.tags,
                    )
                    for x in instances
                ]
            elif prop == 'itag':  # GENCFG-961
                instances = [x for x in instances if prop_value in x.tags]
            else:
                raise Exception("Unknown property %s in group %s" % (prop, gname))

        result.extend(instances)

    if override_domain:
        for instance in result:
            instance.hostname = instance.hostname.partition('.')[0] + override_domain

    return sorted(result, key=lambda x: '%s:%s' % (x.hostname, x.port))


def get_host_weight(host, options=None):
    if options is None:
        pass
    return 1.0


def resolve_host(host, families=None):
    if not hasattr(resolve_host, 'memodict'):
        resolve_host.memodict = dict()

    memokey = (host, tuple(families))
    if memokey not in resolve_host.memodict:
        resolve_host.memodict[memokey] = resolve_host_impl(host, families=families)

    return resolve_host.memodict[memokey]


def resolve_host_impl(host, families=None):
    for attr in ['cached_ip', 'ipv6addr', 'ipv4addr']:
        cached_ip = getattr(host, attr, None)
        if cached_ip is not None:
            return cached_ip

    if families is None:
        families = [6, 4]

    key = "ip:{}*{}".format(host.hostname, families)
    result = GENCFG_CACHE.get(key)
    if not result:
        start_time = time.time()
        result = InstanceDbTransportHolder.get_transport().resolve_host(host, families, use_curdb=True)
        GENCFG_CACHE.put(key, result, gencfg_time=time.time() - start_time)
    return result


def load_listen_addrs(options):
    listen_addrs = list()
    for listen_options in options:
        all_ips = listen_options.get('ips', [])
        if 'ip' in listen_options:
            all_ips.append(listen_options['ip'])

        all_ports = listen_options.get('ports', [])
        if 'port' in listen_options:
            all_ports.append(listen_options['port'])

        for ip in all_ips:
            for port in all_ports:
                addr = (ip, port)
                if addr in listen_addrs:
                    raise Exception("Multiple listeners at %s:%s" % (addr[0], addr[1]))
                else:
                    listen_addrs.append(addr)

    return listen_addrs


def get_ip_from_rt(slbs):
    ret = []

    if type(slbs) is not list:
        slbs = [slbs]

    for slb in slbs:
        iplist = None

        url = 'https://ro.racktables.yandex.net/export/slb-api.php?vs={}&format=json&show'.format(slb)
        request = urllib2.Request(url)

        for attempt in xrange(10):
            try:
                response = GENCFG_CACHE.get(url)
                if not response:
                    response = urllib2.urlopen(request, timeout=60).read()
                    GENCFG_CACHE.put(url, response)

                response = json.loads(response)
                iplist = sorted([i.encode('ascii') for i in response[slb]['ips'].keys()])
                break
            except urllib2.HTTPError as e:
                logging.error("HTTPError: code %s, message %s", e.code, str(e))
                time.sleep(5 + random.randint(1, 20))
            except urllib2.URLError as e:
                logging.error("URLError: code %s, message %s", e.reason, str(e))
                time.sleep(5 + random.randint(1, 20))
            except Exception:
                logging.error("Generic exception:\n%s", traceback.format_exc())

        if not iplist:
            raise Exception(
                "Failed to convert slb <{}> "
                "to iplist using a racktables (url <{}>)".format(
                    slb,
                    url,
                ))

        ret += iplist

    return ret


@Helper(
    'GetIpFromRT',
    '''Макрос для получения данных из Racktables.''',
    [
        ('balancer', '', str, True, 'fqdn http балансера, который записан в RT.'),
        ('slb', '', str, True, 'Имя slb балансера в RT'),
    ]
)
def GetIpFromRT(options):
    attempts = 3
    for attempt in range(0, attempts):
        try:
            balancer_ip = GENCFG_CACHE.get("socket.getaddrinfo" + options['balancer'])
            if not balancer_ip:
                balancer_ip = socket.getaddrinfo(options['balancer'], 0, 0, socket.SOCK_STREAM)[0][4][0]
                GENCFG_CACHE.put("socket.getaddrinfo" + options['balancer'], balancer_ip)
            break
        except socket.gaierror as e:
            logging.error("getaddrinfo error %s", e)
            time.sleep(2 + attempt)
    else:
        raise Exception('Error while trying to resolve ip of {}'.format(options['balancer']))

    url = 'https://ro.racktables.yandex.net/export/slb-info.php?mode=vslist&ip={}'.format(balancer_ip)
    iplist = []

    request = urllib2.Request(url)

    for attempt in xrange(10):
        try:
            response_lines = GENCFG_CACHE.get(url)
            if not response_lines:
                response = urllib2.urlopen(request, timeout=60)
                response_lines = [r for r in response]
                GENCFG_CACHE.put(url, response_lines)
            for line in response_lines:
                if line.replace('"', '').rstrip().split('\t')[3] == options['slb']:
                    if line.split('\t')[0] not in iplist:
                        iplist.append(line.split('\t')[0])
            break
        except urllib2.HTTPError as e:
            logging.error("HTTPError: code %s, message %s", e.code, str(e))
            time.sleep(5 + random.randint(1, 20))
        except urllib2.URLError as e:
            logging.error("URLError: code %s, message %s", e.reason, str(e))
            time.sleep(5 + random.randint(1, 20))
        except Exception:
            logging.error("Generic exception:\n%s", traceback.format_exc())

    if not iplist:
        raise Exception(
            "Failed {} attempt in converting balancer ip <{}> "
            "to iplist using a racktables (url <{}>)".format(
                attempts,
                balancer_ip,
                url,
            ))

    return iplist


@Helper(
    'ResolveIps',
    '''Обертка для резолва ip адресов tld.''',
    [
        ('domain', None, str, True, 'fqdn http балансера, который записан в RT.'),
    ]
)
def ResolveIps(options):
    retries = 3
    while retries > 0:
        try:
            ips = GENCFG_CACHE.get("socket.getaddrinfo" + options['domain'])
            if not ips:
                ips = set([_item[4][0] for _item in socket.getaddrinfo(options['domain'], 0, 0, socket.SOCK_STREAM)])
                GENCFG_CACHE.put("socket.getaddrinfo" + options['domain'], ips)
            break
        except socket.gaierror:
            time.sleep(retries)
            retries -= 1
    else:
        print('Error while trying to resolve ips of slb %s' % options['domain'])
        sys.exit(1)

    return sorted(ips)


@Helper(
    'GetIpsFromRT',
    '''Обертка для получения ip адресов''',
    [
        ('domain', None, str, True, 'fqdn http балансера, который записан в RT.'),
    ]
)
def GetIpsFromRT(options):
    max_tries = 8 + 1
    found = False
    url = 'https://ro.racktables.yandex.net/export/slb-api.php?vs={}&show&format=json'.format(
        options['domain']
    )
    request = urllib2.Request(url)

    for t in range(1, max_tries):
        try:
            data = GENCFG_CACHE.get(url)
            if not data:
                data = json.load(urllib2.urlopen(request, timeout=20))
                GENCFG_CACHE.put(url, data)

        except urllib2.HTTPError as err:
            sys.stderr.write(
                '[WARNING] HTTP error due request to "ro.racktables.yandex.net" Status code: {}; Reason: {};\n'.format(
                    err.code,
                    err.reason
                )
            )
            time.sleep((2 ** t) + (random.randint(0, 1000) / 1000))

        except urllib2.URLError as e:
            sys.stderr.write('Got URLError when requesting <{}>: {}\n'.format(url, e.reason))
            time.sleep((2 ** t) + (random.randint(0, 1000) / 1000))

        else:
            try:
                ips = [str(key) for key in data[options['domain']]['ips']]
                found = True
                break
            except Exception as e:
                sys.stderr.write('Got error when postprocessing <{}>: {}\n'.format(url, str(e)))

    if not found:
        raise Exception(
            'Failed getting ips for domain {} from RT using {} tries'.format(options['domain'], max_tries)
        )

    return ips


def get_instances_len(instance_groups):
    length = 0
    for name in instance_groups:
        gname, props = load_params(name)
        if gname.isupper():
            length += len(get_instances([name]))
        elif 'conline' in gname.split(':'):
            gname_splited = gname.split(':')[1]
            length += len(get_instances([gname_splited]))
        else:
            length += 1
    return length


@Helper(
    'GetServiceInstances',
    '''Забираем инстансы сервиса из Няни''',
    [
        ('service', None, str, True, 'ID сервиса в каталоге сервисов Няни'),
        ('geo', None, str, False, 'Идентификатор геопризнака'),
        ('group_name', None, str, False, 'Идентификатор группы'),
    ]
)
def GetServiceInstances(options):
    max_tries = 8 + 1
    found = False

    for t in range(1, max_tries):
        try:
            with requests.Session() as s:
                url = 'http://nanny.yandex-team.ru/v2/services/%s/current_state/instances/'
                request_result = s.get(url % options['service'], headers={'Content-Type': 'application/json'})

        except requests.exceptions.RequestException as err:
            print('Getting instances of service %s from Nanny failed: %s' % (options['service'], err))
            time.sleep((2 ** t) + (random.randint(0, 1000) / 1000))

        else:
            if not request_result.json().get('result'):
                sys.stderr.write(
                    '[WARNING]: Recieved empty list of backends for service: {}\n'.format(options['service'])
                )
                time.sleep((2 ** t) + (random.randint(0, 1000) / 1000))
                continue

        try:
            if options.get('geo') is not None:
                instances = [
                    str('%s:%s' % (instance['hostname'], instance['port']))
                    for instance in request_result.json()['result'] if 'a_geo_%s' % options['geo'] in instance['itags']
                ]
            elif options.get('group_name') is not None:
                instances = [
                    str('%s:%s' % (instance['hostname'], instance['port']))
                    for instance in request_result.json()['result'] if options['group_name'] in instance['itags']
                ]
            else:
                instances = [
                    str('%s:%s' % (instance['hostname'], instance['port']))
                    for instance in request_result.json()['result']
                ]

        except Exception as err:
            sys.stderr.write('Invalid JSON from Nanny for {} service: {}\n'.format(options['service'], err))
            time.sleep((2 ** t) + (random.randint(0, 1000) / 1000))
            continue

        else:
            if not instances:
                sys.stderr.write(
                    'The list of backends for service: {}, with your filter is empty\n'.format(
                        options['service']
                    )
                )
                time.sleep((2 ** t) + (random.randint(0, 1000) / 1000))
                continue
            else:
                found = True
                break

    if not found:
        raise RuntimeError(
            'Failed getting instances list for service {} from Nanny using {} tries\n'.format(
                options['service'], max_tries
            )
        )

    return instances


class NannyServiceTool(object):
    """
     Wrapper for nanny service import instances
    """
    def __init__(self, service):
        self.service = service
        assert service, "Nanny service name empty"
        self.instances_by_dc = self.__create_dc_list__()
        assert self.instances_by_dc, "Conductor empty instances list for service {}".format(service)
        self.instances_as_list = self.__create_plain_list__(self.instances_by_dc)

    def __create_dc_list__(self):

        # import src.utils
        instances = get_instances(['nannyservice:%s' % self.service])
        service_by_dc = dict()

        for instance in instances:
            if instance.dc in service_by_dc:
                    service_by_dc[instance.dc].append("%s:%s" % (instance.hostname, instance.port))
            else:
                service_by_dc[instance.dc] = list()
                service_by_dc[instance.dc].append("%s:%s" % (instance.hostname, instance.port))

        return service_by_dc

    def __create_plain_list__(self, dc_list):
        instance_list = list()

        for dc in dc_list:
            instance_list.extend(dc_list[dc])

        return instance_list


@Helper(
    'GetServiceInstancesHQ',
    '''Забираем инстансы сервиса из HQ''',
    [
        ('service', None, str, True, 'Имя сервиса в каталоге сервисов Няни'),
        # ('geo', None, str, False, 'Идентификатор геопризнака'),
        # ('group_name', None, str, False, 'Идентификатор группы'),
    ]
)
def GetServiceInstancesHQ(options):
    """
    В этом примере мы:
      * используем federated сервис для получения всех кластеров HQ
        * https://wiki.yandex-team.ru/jandekspoisk/sepe/nanny/federated/api/
      * затем у каждого запросим инстансы интересующего нас сервиса
        * https://wiki.yandex-team.ru/jandekspoisk/sepe/nanny/hq/api/
    """

    from nanny_rpc_client import RequestsRpcClient
    # Определения запросов и ответов сервера
    from clusterpb import federated_stub, federated_pb2
    from clusterpb import hq_pb2, hq_stub

    client = RequestsRpcClient(
        # SWAT-3317
        # 'http://federated.yandex-team.ru/rpc/federated/',
        'http://federated.yandex-team.ru/rpc/federated/',
        request_timeout=10
    )
    # Инициализируем клиент для Federated Service'а
    f_client = federated_stub.FederatedClusterServiceStub(client)
    # Создаём объект запроса, без параметров
    find_req = federated_pb2.FindClustersRequest()

    max_tries = 3 + 1  # Три запроса
    found_clusters = False

    for t in range(1, max_tries):
        try:
            # Выполняем запрос и сохраняем список кластеров
            clusters = f_client.find_clusters(find_req).value
            found_clusters = True
            break

        except Exception as err:
            print('Failed get clusters from federated: {}'.format(err))
            time.sleep((2 ** t) + (random.randint(0, 1000) / 1000))

    if not found_clusters:
        raise Exception(
            'Failed getting clusters list from federated using {} tries'.format(max_tries)
        )

    data = []  # Здесь будем хранить список инстансов сервиса

    # Создаём объект запроса (он будет один для всех кластеров)
    find_req = hq_pb2.FindInstancesRequest()
    # Указываем интересующий нас сервис
    find_req.filter.service_id = options['service']

    # Выполняем поиск инстансов в каждом кластере
    for c in clusters:
        # Создаём клиент для конкретного кластера
        client = RequestsRpcClient(
            '{}{}'.format(c.spec.endpoint.url, 'rpc/instances/'),
            request_timeout=10
        )
        hq_client = hq_stub.InstanceServiceStub(client)
        for t in range(1, max_tries):
            try:
                resp = hq_client.find_instances(find_req)
                data.extend(resp.instance)
                break

            except Exception as err:
                print('Failed get data from hq cluster {}: {}'.format(c.spec.endpoint.url, err))
                time.sleep((2 ** t) + (random.randint(0, 1000) / 1000))

    if data:
        # Инстанс через meta - неправильный путь, потому, что не учитывает состояние
        # Инстанс через spec
        instances = [
            '{}:{}'.format(
                i.spec.node_name,
                i.spec.allocation.port[0].port
            ) for i in data if i.status.ready.status == u'True'
        ]
        '''port[0].port - потому, что может быть вот так

        {"port": [
            {
                "name": "web",
                "port": 1041,
                "protocol": "TCP"
            },
            {
                "name": "rpc",
                "port": 1045,
                "protocol": "TCP"
            },
            {
                "name": "quorum",
                "port": 1042,
                "protocol": "TCP"
            }
        ]}
        '''
        return instances

    else:
        raise Exception('Failed getting instance list from HQ')


@Helper(
    'ValidateStatusCodes',
    '''Проверям корректность статус-кодов''',
    [
        (
            'codes_list', None, list, True,
            'Список с статус кодами или их семейством в формате Yxx, где Y в [0-5]'
        ),
    ]
)
def ValidateStatusCodes(options):
    status_code_pattern = ('[1-5][0-9]{2}|[1-5]xx')
    for code in options['codes_list']:
        if re.match(status_code_pattern, code) is None:
            raise Exception('Unknow status code of family: {}'.format(code))


def ExpandSlb(host_list, ipversion=4):
    """
    :param host_list:  входящий список хостов и slb
    :ipversion: версия ipшников 4,6 , 46 разрезолвить в оба по-умолчанию 46
    :return: лист айпи адрессов с портами·
    """

    def get_ip_list(hostname, ipver):
        iplist = []
        socktype = socket.AF_INET  # ipv4 only
        if ipver == 6:
            socktype = socket.AF_INET6
        if ipver == 46:
            socktype = 0  # means both INET and INET6

        for i in range(3):
            try:
                addr = GENCFG_CACHE.get("socket.getaddrinfo" + hostname)
                if not addr:
                    addr = socket.getaddrinfo(hostname, 0, socktype, socket.SOCK_STREAM)
                    GENCFG_CACHE.put("socket.getaddrinfo" + hostname, addr)
                break
            except socket.gaierror as e:
                if i == 2:
                    raise Exception("Can not resolve host <%s> with proto <%s> (%s)" % (hostname, ipver, str(e)))
                else:
                    time.sleep(1)
                    continue

        for i, x in enumerate(addr):
            iplist.append(x[4][0])

        return iplist

    resolved_host = []
    for i, x in enumerate(host_list):
        host = x.rsplit(":")[0]
        port = x.rsplit(":")[1]

        ips = get_ip_list(host, ipversion)
        for j, y in enumerate(ips):
            resolved_host.append(y + ":" + port)

    return resolved_host


def fake_config_tag():
    return '{}'.format(int(time.time()))


@Helper(
    'GenSniContexts',
    '''Хелпер для генерации контекстов для SslSni''',
    [
        ('data', None, OrderedDict, True, 'Набор данных для генерации контекстов')
    ]
)
def GenSniContexts(options):
    sni_contexts = OrderedDict()
    priority = 1000

    for key, value in options['data'].iteritems():
        priority -= 1
        sni_contexts[key] = OrderedDict([
            ('cert', LuaGlobal(
                'public_cert{}'.format(1001 - priority),
                '/dev/shm/balancer/allCAs-{}.pem'.format(key)
            )),
            ('priv', LuaGlobal(
                'private_cert{}'.format(1001 - priority),
                '/dev/shm/balancer/priv/{}.pem'.format(key)
            )),
            ('events', OrderedDict([
                ('reload_ticket_keys', 'reload_ticket'),
            ])),
            ('priority', priority),
            ('servername', {
                'servername_regexp': value['servername_regexp'],
                'case_insensitive': True,
            }),
            ('ocsp', 'ocsp/allCAs-{}.der'.format(key)),
            ('ocsp_file_switch', './controls/disable_ocsp'),
            ('ticket_keys_list', OrderedDict([
                (LuaAnonymousKey(), OrderedDict([
                    ('keyfile', '/dev/shm/balancer/priv/%s.%s.key' % (j, key)),
                    ('priority', 1000 - i)
                ])) for i, j in enumerate(['1st', '2nd', '3rd'])
            ]))
        ])

    return sni_contexts

""" Utilities for iptables_except_my_group """
import sys
import subprocess
import logging
import logging.config
import requests
from requests.adapters import HTTPAdapter
from requests.packages.urllib3.util import Retry
import requests_cache
import socket
import re
from collections import defaultdict
from itertools import chain


PROTO = {
    socket.AF_INET: {
        'proto': '4', 'family': 'inet'
    },
    socket.AF_INET6: {
        'proto': '6', 'family': 'inet6'
    }
}

IPTABLES_BIN = {socket.AF_INET: "iptables", socket.AF_INET6: "ip6tables"}
IPSET_NAME_MAX_LEN = 31
RULE_TEMPLATE = "-p tcp --dport {} -m set ! --match-set {} src -j REJECT " \
                "--reject-with tcp-reset"
IPT_CHAIN_NAME = "INPUT"


BLACKLISTED_PORTS = [22]

M_OK = 0
M_WARN = 1
M_CRIT = 2

CONDUCTOR_CACHE = '/var/cache/iptables_except_my_group'
CONDUCTOR_RETRIES = 3
CONDUCTOR_CACHE_TTL = 300

logger = logging.getLogger(__name__)


def setup_logger(config=None, default_debug=False):
    """ Configure logger """
    if config is None:
        config = {
            'version': 1,
            'disable_existing_loggers': False,
            'formatters': {
                'default': {
                    'format': '%(asctime)s [%(levelname)s] '
                              '%(name)s: %(message)s'
                },
            },
            'handlers': {
                'default': {
                    'level': 'DEBUG' if default_debug else 'INFO',
                    'class': 'logging.StreamHandler',
                    'formatter': 'default'
                },
            },
            'loggers': {
                '': {
                    'handlers': ['default'],
                    'level': 'DEBUG' if default_debug else 'INFO',
                    'propagate': True
                }
            }
        }
    logging.config.dictConfig(config)


def call_cmd(*args):
    """ Just call command. Output goes to stdout. """
    cmd = list(chain.from_iterable(args))
    logger.debug('Cmd %s', cmd)
    return subprocess.call(cmd)


def call_shell(*args):
    """ Call command using shell """
    cmd = list(chain.from_iterable(args))
    logger.debug('Shell %s', cmd)
    return subprocess.call(" ".join(cmd), shell=True)


def run_cmd(*args, out_format=None):
    """ Run command and return output, parsed and converted if needed """
    cmd = list(chain.from_iterable(args))
    out = subprocess.check_output(cmd).decode("utf-8")
    if out_format is None:
        return out
    elif out_format == "list":
        return out.splitlines()
    else:
        raise NotImplementedError


def _ask_conductor(endpoint, timeout=5):
    """
    Get data from conductor, raise RuntimeError if error
    :param endpoint: api endpoint, string
    :param timeout: http read/connect timeout
    :return: result.text, string
    """
    _c_api = 'https://c.yandex-team.ru/api-cached'
    final_url = _c_api + endpoint

    # Adding cache and retries for requests
    requests_cache.install_cache(CONDUCTOR_CACHE,
                                 backend='sqlite',
                                 expire_after=CONDUCTOR_CACHE_TTL)
    _retries = Retry(total=CONDUCTOR_RETRIES, backoff_factor=0.1,
                     status_forcelist=[500, 502, 503, 504])
    req_with_retries = requests.Session()
    req_with_retries.mount('http://', HTTPAdapter(max_retries=_retries))
    req_with_retries.mount('https://', HTTPAdapter(max_retries=_retries))

    logger.debug('HTTP request: %s', final_url)
    try:
        req = req_with_retries.get(final_url, timeout=timeout)
        logger.debug('HTTP result: %s (%s). From cache: %s',
                     req.text, req.status_code, req.from_cache)
        req.raise_for_status()
    except Exception as error:
        raise RuntimeError('Error while communicating with '
                           'conductor api: {}'.format(error))
    return req.text


def get_my_group(remove_backup_suffix=False):
    """
    Get hosts group from conductor
    :param remove_backup_suffix: Remove -backup prefix from group name, boolean
    :return: group name, string
    """
    my_fqdn = socket.getfqdn()
    group = _ask_conductor('/generator/aggregation_group?fqdn=' + my_fqdn)
    if remove_backup_suffix and group.endswith('-backup'):
        group = group[:-len("-backup")]
    logger.debug('Resolved group: %s', group)
    return group


def get_my_neighbors(my_group):
    """ Get addresses of all my neighbors from GROUPS2HOSTS_URL """
    neighbors = defaultdict(set)
    hosts_list = _ask_conductor('/groups2hosts/' + my_group)

    for host in hosts_list.splitlines():
        records = socket.getaddrinfo(host, 0, 0, socket.SOCK_STREAM)
        # list of tuples (family, socktype, proto, canonname, sockaddr)
        #                   0        1        2        3          4
        # see https://docs.python.org/3/library/socket.html#socket.getaddrinfo
        for rec in records:
            if rec[0] in PROTO:
                neighbors[rec[0]].add(rec[4][0])
    logger.debug('Resolved neighbors: %s', neighbors)
    return neighbors


def make_ipset_name(group, s_proto_name):
    """ Truncate name when it's longer then IPSET_MAX_NAME_LEN """
    ipset_name = "{}-{}".format(group, s_proto_name)
    # 31 is max length for ipset's set name and also preserve
    # one char for tmp rule name
    logger.debug("ipset long name: %s, len: %d", ipset_name, len(ipset_name))
    if len(ipset_name) > IPSET_NAME_MAX_LEN-1:
        ipset_name = "s_" + ipset_name[-(IPSET_NAME_MAX_LEN-len("s_")-1):]
    safe_ipset_name = re.sub(r"[^a-zA-Z0-9_\-]", "_", ipset_name)
    logger.debug('Result ipset name: %s', safe_ipset_name)
    return safe_ipset_name


def make_iptables_rules(group, neighbors, port=1334, force=False):
    """ Create proper iptables rules """
    if port in BLACKLISTED_PORTS and not force:
        raise RuntimeError('Port is in blacklist. Use force to skip check')

    for family in neighbors:
        ipset_name = make_ipset_name(group, PROTO[family]['proto'])

        check_rule = " ".join((
            IPTABLES_BIN[family], "-C", IPT_CHAIN_NAME,
            RULE_TEMPLATE.format(port, ipset_name), "2>/dev/null"))

        add_rule = " ".join((
            IPTABLES_BIN[family], "-A", IPT_CHAIN_NAME,
            RULE_TEMPLATE.format(port, ipset_name)))

        tmp_name = "_" + ipset_name
        call_cmd(["ipset", "create", tmp_name, "hash:ip", "family",
                  PROTO[family]['family']])

        for host in neighbors[family]:
            call_cmd(["ipset", "add", tmp_name, host])

        # Create main table to be sure it exists
        call_cmd(["ipset", "-!", "create", ipset_name, "hash:ip", "family",
                  PROTO[family]['family']])

        call_cmd(["ipset", "swap", tmp_name, ipset_name])
        call_cmd(["ipset", "destroy", tmp_name])

        if call_shell([check_rule]) != 0:
            logger.debug('Rule (%s) not found, adding it (%s)',
                         check_rule, add_rule)
            call_shell([add_rule])
            logger.info('Added rule')
        else:
            logger.info('Rule already exists')


def remove_iptables_rules(group, neighbors, port=1334, force=False):
    """ Delete iptables rules """
    for family in neighbors:
        ipset_name = make_ipset_name(group, PROTO[family]['proto'])
        if call_shell(["ipset", "list", ipset_name, ">/dev/null", "2>&1"]) > 0:
            continue
        delete_rule = " ".join(
            (IPTABLES_BIN[family], "-D", IPT_CHAIN_NAME,
             RULE_TEMPLATE.format(port, ipset_name)))
        if call_shell([IPTABLES_BIN[family], "-C", IPT_CHAIN_NAME,
                       RULE_TEMPLATE.format(port, ipset_name),
                       "2>/dev/null"]) == 0:
            logger.debug('Found rule, deleting it (%s)', delete_rule)
            call_shell([delete_rule])
            logger.info('Removed rules')
        else:
            logger.info('No rules found')


def check_iptables_rules(group, neighbors, port=1334, force=False):
    """ Check if rule is in iptables """
    for family in neighbors:
        ipset_name = make_ipset_name(group, PROTO[family]['proto'])
        if call_shell(["ipset", "list", ipset_name, ">/dev/null", "2>&1"]) > 0:
            raise RuntimeError('Ipset {} not found'.format(ipset_name))
        if call_shell(
                [IPTABLES_BIN[family], "-C", IPT_CHAIN_NAME,
                 RULE_TEMPLATE.format(port, ipset_name), "2>/dev/null"]) == 0:
            logger.info('Closed')
            return True
    logger.info('Opened')
    return False


def operate_iptables(action, port, remove_backup_suffix=False, force=False):
    """ Work with iptables """
    _actions = {
        'open': remove_iptables_rules,
        'close': make_iptables_rules,
        'check': check_iptables_rules,
        'status': check_iptables_rules
    }
    if action not in _actions.keys():
        raise RuntimeError('Unknown action: {}. Possible: {}'.format(
            action, _actions.keys()))

    group = get_my_group(remove_backup_suffix=remove_backup_suffix)
    neighbors = get_my_neighbors(group)

    logger.debug('Running %s action with params: %s %s %s %s',
                 action, group, neighbors, port, force)

    return _actions[action](group, neighbors, port, force)


def monrun(level, data):
    """
    Produce monrun formatted output and exit
    :param data: monrun message, string
    :param level: monrun level, string or int
    :return:
    """
    print('{};{}'.format(level, data))
    sys.exit(0)

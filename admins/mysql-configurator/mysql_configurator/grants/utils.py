"""
common functions
"""
import socket
import os
import re
import logging
import subprocess
from hashlib import sha1
from StringIO import StringIO
import netaddr
import requests
import git
from vault_client.instances import Production as VaultClient


LOG = logging.getLogger('Utils')


def net_to_mhost(net):
    """
    converts network to MySQL ip notation
    :param net: network or host IP or IPv6
    :return: MySQL host like 2a02:%
    """
    maddr = None
    # we should know what we get: ipv6 or ipv4
    if re.match(r"([\d]+\.[\d]+\.[\d]+\.[\d]+/[\d]+)", net):
        maddr = netaddr.IPNetwork(net).network.__str__() + "/" + netaddr.IPNetwork(
            net).netmask.__str__()
    # if we here than net is ipv6 addr
    elif is_valid_cidr(net):
        if not re.match(r"([\d]+\.[\d]+\.[\d]+\.[\d]+)", net):
            tmp = net.split(":")
            if re.match(r"/[0-9]+", tmp[-1]):
                # print tmp
                tmp[-2] = "%"
                tmp.pop(-1)
            else:
                # print tmp
                tmp[-1] = "%"

            maddr = ":".join(tmp)
    # find one ipv6 addr
    elif re.match(r"((?:[0-9a-fA-F]*:)+)", net):
        maddr = net
    # find one ipv4 addr
    elif re.match(r"([\d]+\.[\d]+\.[\d]+\.[\d]+)", net):
        maddr = net
    elif re.match(r"(a-f0-9:\.)?%", net):
        maddr = net
    # localhost and similar definitions
    elif re.match(r"[a-zA-Z0-9]+", net):
        maddr = net
    else:
        return None

    return maddr


def resolve_host(host):
    """
    resolves host to ip
    :param host: hostname
    :return: ip addrs list
    """
    try:
        info = socket.getaddrinfo(host, None, 0, 0, socket.AI_ADDRCONFIG)
    except socket.gaierror as fail:
        LOG.debug("Failed to get addrinfo for %s: %s", host, fail.__str__())
        return None
    addr = []
    for i in info:
        addr.append(i[4][0])
    return addr


def group2hosts(group):
    """
    resolves conductor group into the list of hosts
    :param group: c. group
    :return: list of hosts
    """
    group = group.replace('%', '')
    url = "https://c.yandex-team.ru/api/groups2hosts/" + group
    try:
        data = requests.get(url, verify='/etc/ssl/certs/ca-certificates.crt')
        if data.status_code != 200:
            LOG.debug("Filed resolve group %s: conductor returned %s", group, data.status_code)
            return None
    except (requests.ConnectionError, requests.HTTPError):
        LOG.debug("Can't get data for group %s", group)
        return None

    return data.content.split()


def group2ip(group):
    """
    resolve conductor group into IP addresses
    :param group:
    :return: IP addresses
    """
    addrs = None
    hosts = group2hosts(group)
    if hosts:
        addrs = list(map(resolve_host, hosts))
    return addrs


def formatuser(user):
    """
    create mysql username like 'user'@'host'
    :param user: username like user@host
    :return:
    """
    user, host = user.split('@')
    return "'{}'@'{}'".format(user, host)


def secget(filename):
    """
    get file from secdist
    :param filename: secget path to config (without prefix /repo/project/)
    :return: text stream
    """
    cmd = '/usr/bin/secget ' + filename
    data = subprocess.check_output(cmd, shell=True)
    return data


def git_clone(repository, destination):
    """
    Clone git repository into local destination
    :param repository: git repository url
    :param destination: local path where to clone
    :return: None
    """
    if os.path.exists(destination):
        # clean local copy before actions
        clean(destination)
    try:
        git.Repo.clone_from(repository, destination)
    except git.GitCommandError as err:
        raise Exception(err)

def yav_get(secret_uuid):
    """
    Get secret from Yandex Vault
    :param secret_uuid: secret uuid
    :return: dict with keys and values
    """
    client = VaultClient()
    try:
        return client.get_version(secret_uuid)["value"]
    except Exception as err:
        raise Exception(err)

def merge(one, two):
    """
    merge two dicts
    :param one: dict one
    :param two: dict two
    :return: merged dict one
    """
    if isinstance(one, dict) and isinstance(two, dict):
        for key, val in two.iteritems():
            if key not in one:
                one[key] = val
            else:
                one[key] = merge(one[key], val)
    return one


def listreduce(data, target):
    """
    converts list of lists data into one simple list target
    :param data: input composite list
    :param target: out simple list
    """
    if isinstance(data, list):
        for item in data:
            listreduce(item, target)
    else:
        if data is not None:
            target.append(data)


def mypassword(password_string):
    """
    generates mysql password hash
    :param password_string: password
    :return: password hash
    """
    password_hash = ''
    if password_string is not None:
        if password_string != "":
            password_hash = '*' + sha1(sha1(password_string).digest()).hexdigest().upper()
    return password_hash


def convert(config):
    """
    Converts legacy mysql grants config into new
    :param config: legacy config yaml dump
    :return: grants config in new format
    """
    grants = config['grants']
    users = config['users']
    hosts = config['hosts']

    newconfig = {}

    newconfig.update({'hosts': hosts})
    newconfig.update({'users': users})

    newgrants = []

    for user in grants:
        for host in grants[user]:
            for target in grants[user][host]['grants']:
                privs = grants[user][host]['grants'][target]
                newgrants.append({'host': host, 'user': user, 'db': target, 'privs': privs})

    newconfig.update({'grants': newgrants})
    return newconfig


def clean(path):
    """
    clean all data in path
    :param path: path on FS
    """
    if path is not None:
        if os.path.exists(path):
            protect = ['/', '/usr', '/var', '/lib']
            if path in protect:
                return None
            for root, dirs, files in os.walk(path, topdown=False):
                for name in files:
                    os.remove(os.path.join(root, name))
                for name in dirs:
                    os.rmdir(os.path.join(root, name))
            LOG.debug("Clean complete for path %s", path)
            return None
        else:
            LOG.debug("%s does not exists, clean is not needed.", path)
            return None
    else:
        LOG.debug("Got empty path for clean.")
        return None


def savedata(fname, data):
    """
    saves data to file
    :param fname: filename
    :param data: data StringIO
    :return:
    """
    with open(fname, 'w') as conf:
        conf.write(data.getvalue())


def is_valid_cidr(address):
    """Check if address is valid

            The provided address can be a IPv6 or a IPv4
            CIDR address.
            """
    try:
        # Validate the correct CIDR Address
        netaddr.IPNetwork(address)
    except netaddr.core.AddrFormatError:
        return False
    except UnboundLocalError:
        # NOTE(MotoKen): work around bug in netaddr 0.7.5 (see detail in
        # https://github.com/drkjam/netaddr/issues/2)
        return False

    # Prior validation partially verify /xx part
    # Verify it here
    ip_segment = address.split('/')

    if len(ip_segment) <= 1 or ip_segment[1] == '':
        return False

    return True


def get_project(host):
    """
    get conducor project
    :param host: hostname
    :return: project name
    """
    project = None
    url = "https://c.yandex-team.ru/api/hosts2projects/" + host
    try:
        data = requests.get(url, verify='/etc/ssl/certs/ca-certificates.crt')
        if data.status_code == 200:
            project = data.content.strip()
    except requests.RequestException as err:
        LOG.debug("Failed to get project name %s", err.message)
    return project


def define_my_version():
    """
    determine the MySQL server version
    :return: version like 5.5 5.6 or 5.7, etc.
    """
    version = None
    try:
        out = subprocess.check_output(['/usr/sbin/mysqld', '-V'])
    except (OSError, subprocess.CalledProcessError) as err:
        LOG.debug(err)
        return version

    data = re.findall(r'Ver\s[0-9\.\-]+', out)
    if data:
        try:
            version = data[0].split()[1]
        except KeyError:
            pass

    return version

def cnf2dict(conf):
    """
    converts raw config to dict
    :param conf: raw config
    :return: config as dict
    """
    dictcnf = {}
    # there was try-except to prevent re module error on parsing pattern(fails on python 2.7.3)
    re.compile(r'(^\w+)\s*=?\s*(\w*)?')
    section_name = 'mysqld'
    for line in conf:
        if re.match(r'^#\s?.*', line):
            continue
        elif re.match(r'^\[[\w\-]+\]$', line):
            section_name = line.replace('[', '').replace(']', '').strip()
            dictcnf.update({section_name: {}})
        elif re.match(r'^\w+\s*=?\s*(\w*)?', line):
            opt = line.split('=', 1)
            if len(opt) < 2:
                dictcnf[section_name].update({opt[0].strip(): 1})
            else:
                dictcnf[section_name].update({opt[0].strip(): opt[1].strip()})

    if dictcnf.keys():
        return dictcnf

    return None


def dict2cnf(conf):
    """
    converts config from dictionary to StringIO object
    :param conf: dict config
    :return: StringIO config
    """
    rawconf = StringIO()
    for section in conf.keys():
        rawconf.write(u"\n[{}]\n".format(section))
        for key in conf[section].keys():
            rawconf.write(u"{} = {}\n".format(key, conf[section][key]))

    return rawconf


def transform(val):
    """transform values like 1G to bytes"""
    if isinstance(val, str):
        if re.match('^[0-9]+(K|M|G)+', val):
            if 'K' in val:
                val = int(val[:-1]) * 1024
            elif 'M' in val:
                val = int(val[:-1]) * 1024 * 1024
            elif 'G' in val:
                val = int(val[:-1]) * 1024 * 1024 * 1024
    return val

import csv
import configparser
import fcntl
import io
import json
import logging
import os
import os.path
import pathlib
import shutil
import shutil
import ssl
import subprocess
import tempfile
import time
import urllib
import urllib.request
from functools import wraps
import sys

INFINIBAND_ROOT_CLUSTER_TAG_NAME = "infiniband cluster"

MAX_U_INT8 = 255
MAX_U_INT16 = 65535
MAX_U_INT64 = 18446744073709551615

# => (type (FIXME: just recode from HEX to DEC), rename)
PM_INFO_TYPES = {"ExcessiveBufferOverrunErrors": None,  # "u_int8_t", disable in favor ExcessiveBufferOverrunErrorsExt
                 "ExcessiveBufferOverrunErrorsExt": "ExcessiveBufferOverrunErrors",
                 "LinkDownedCounter": None,  # "u_int8_t", disable in favor LinkDownedCounter
                 "LinkDownedCounterExt": "LinkDownedCounter",
                 "LinkErrorRecoveryCounter": None,  # "u_int8_t", disable in favor LinkErrorRecoveryCounterExt
                 "LinkErrorRecoveryCounterExt": "LinkErrorRecoveryCounter",
                 "LocalLinkIntegrityErrors": None,  # "u_int8_t", disable in favor LocalLinkIntegrityErrorsExt
                 "LocalLinkIntegrityErrorsExt": "LocalLinkIntegrityErrors",
                 "PortBufferOverrunErrors": "",  # 16.1.4.2 PORTRCVERRORDETAILS When reading (Get), this is ignored. Total packets received on the port discarded due to buffer overrun
                 "PortDLIDMappingErrors": "",  # 16.1.4.2 PORTRCVERRORDETAILS When reading (Get), this is ignored.
                 "PortInactiveDiscards": "",  # 16.1.4.3 PORTXMITDISCARDDETAILS When reading (Get), this is ignored.
                 "PortLocalPhysicalErrors": "",  # 16.1.4.2 PORTRCVERRORDETAILS When reading (Get), this is ignored.
                 "PortLoopingErrors": "",  # 16.1.4.2 PORTRCVERRORDETAILS When reading (Get), this is ignored.
                 "PortMalformedPacketErrors": "",  # 16.1.4.2 PORTRCVERRORDETAILS When reading (Get), this is ignored.
                 "PortMultiCastRcvPkts": "",
                 "PortMultiCastXmitPkts": "",
                 "PortNeighborMTUDiscards": "",  # 16.1.4.3 PORTXMITDISCARDDETAILS When reading (Get), this is ignored.
                 "PortRcvConstraintErrors": None,  # "u_int8_t", disable in favor PortRcvConstraintErrorsExt
                 "PortRcvConstraintErrorsExt": "PortRcvConstraintErrors",
                 "PortRcvData": None,  # "u_int32_t", disable in favor PortRcvDataExtended
                 "PortRcvDataExtended": "PortRcvData",
                 "PortRcvErrors": None,  # "u_int16_t", disable in favor PortRcvErrorsExt
                 "PortRcvErrorsExt": "PortRcvErrors",
                 "PortRcvPkts": None,  # "u_int32_t",  disable in favor PortRcvPktsExtended
                 "PortRcvPktsExtended": "PortRcvPkts",  # Ext ?
                 "PortRcvRemotePhysicalErrors": None,  # "u_int16_t", disable in favor PortRcvRemotePhysicalErrorsExt
                 "PortRcvRemotePhysicalErrorsExt": "PortRcvRemotePhysicalErrors",
                 "PortRcvSwitchRelayErrors": None,  # "u_int16_t", disable in favor PortRcvSwitchRelayErrorsExt
                 "PortRcvSwitchRelayErrorsExt": "PortRcvSwitchRelayErrors",
                 "PortSwHOQLifetimeLimitDiscards": "",  # 16.1.4.3 PORTXMITDISCARDDETAILS When reading (Get), this is ignored.
                 "PortSwLifetimeLimitDiscards": "",  # 16.1.4.3 PORTXMITDISCARDDETAILS When reading (Get), this is ignored.
                 "PortUniCastRcvPkts": "",
                 "PortUniCastXmitPkts": "",
                 "PortVLMappingErrors": "",  # 16.1.4.2 PORTRCVERRORDETAILS When reading (Get), this is ignored.
                 "PortXmitConstraintErrors": None,  # "u_int8_t",  disable in favor PortXmitConstraintErrorsExt
                 "PortXmitConstraintErrorsExt": "PortXmitConstraintErrors",
                 "PortXmitData": None,  # "u_int32_t", disable in favor PortXmitDataExtended
                 "PortXmitDataExtended": "PortXmitData",
                 "PortXmitDiscards": None,  # "u_int16_t", disable in favor PortXmitDiscardsExt
                 "PortXmitDiscardsExt": "PortXmitDiscards",
                 "PortXmitPkts": None,  # "u_int32_t", disable in favor PortXmitPktsExtended
                 "PortXmitPktsExtended": "PortXmitPkts",  # Ext?
                 "PortXmitWait": None,  # "u_int32_t", disable in favor PortXmitPktsExtended
                 "PortXmitWaitExt": "PortXmitWait",
                 "QP1DroppedExt": "QP1Dropped",
                 "SymbolErrorCounter": None,  # "u_int16_t", disable in favor SymbolErrorCounterExt
                 "SymbolErrorCounterExt": "SymbolErrorCounter",
                 "VL15Dropped": None,  # "u_int16_t", disable in favor VL15DroppedExt
                 "VL15DroppedExt": "VL15Dropped",
                 "max_retransmission_rate": None,
                 "retransmission_per_sec": "",
                 }

PORTS_TYPES = {
    "CapMsk": None,
    "CapMsk2": None,
    "ClientReregister": "",
    "DiagCode": "",
    "FECActv": "FECModeActive",
    "FilterRawInb": "FilterRawInbound",
    "FilterRawOutb": "FilterRawOutbound",
    "GIDPrfx": None,
    "GUIDCap": None,
    "HoQLife": "",
    "InitType": "",
    "InitTypeReply": "",
    "LID": None,
    "LinkDownDefState": "LinkDownDefaultState",
    "LinkRoundTripLatency": "",
    "LinkSpeedEn": "LinkSpeedEnabled",
    "LinkSpeedActv": "LinkSpeedActive",
    "LinkSpeedSup": "",
    "LinkWidthActv": "LinkWidthActive",
    "LinkWidthEn": "LinkWidthEnabled",
    "LinkWidthSup": "LinkWidthSupported",
    "LMC": None,
    "LocalPhyError": "",  # Threshold value. When the count of marginal link errors exceeds
    # this threshold, the local link integrity error shall be detected as described in 7.12.2
    # Error Recovery Procedures on page 244.
    "LocalPortNum": "",
    "MaxCreditHint": "",
    "MKey": "M_Key",
    "M_KeyLeasePeriod": "",
    "MKeyProtBits": "M_KeyProtectBits",
    "MKeyViolations": "",
    "MSMLID": "",
    "MSMSL": "",
    "MTUCap": "",
    "NMTU": "NeighborMTU",
    "OOOSLMask": "",
    "OpVLs": "",
    "OverrunErrors": "",
    "PartEnfInb": "",
    "PartEnfOutb": "",
    "PKeyViolations": "P_KeyViolations",
    "PortPhyState": "",
        # 1: Sleep
        # 2: Polling
        # 3: Disabled
        # 4: PortConfigurationTraining
        # 5: LinkUp
        # 6: LinkErrorRecovery
        # 7: Phy Test
        # 8 - 15: Reserved

    "PortState": "",
        # 1: Down (includes failed links)
        # 2: Initialize
        # 3: Armed
        # 4: Active
        # 5 - 15: Reserved
    "QKeyViolations": "Q_KeyViolations",
    "RespTimeValue": "",
    "RetransActv": "",
    "SubnTmo": "",
    "VLArbHighCap": "VLArbitrationHighCap",
    "VLArbLowCap": "VLArbitrationLowCap",
    "VLCap": "",
    "VLHighLimit": "",
    "VLStallCnt": "VLStallCount",
}


def retry(ExceptionToCheck, tries=4, delay=3, backoff=2):
    """Retry calling the decorated function using an exponential backoff.

    http://www.saltycrane.com/blog/2009/11/trying-out-retry-decorator-python/
    original from: http://wiki.python.org/moin/PythonDecoratorLibrary#Retry

    :param ExceptionToCheck: the exception to check. may be a tuple of
        exceptions to check
    :type ExceptionToCheck: Exception or tuple
    :param tries: number of times to try (not retry) before giving up
    :type tries: int
    :param delay: initial delay between retries in seconds
    :type delay: int
    :param backoff: backoff multiplier e.g. value of 2 will double the delay
        each retry
    :type backoff: int
    """

    def deco_retry(f):
        @wraps(f)
        def f_retry(*args, **kwargs):
            mtries, mdelay = tries, delay
            while mtries > 1:
                try:
                    return f(*args, **kwargs)
                except ExceptionToCheck as e:

                    logging.warning("%r, Retrying in %d seconds...", e, mdelay)
                    time.sleep(mdelay)
                    mtries -= 1
                    mdelay *= backoff
            return f(*args, **kwargs)

        return f_retry  # true decorator

    return deco_retry


def export_ib_diag(directory):
    os.makedirs(directory, exist_ok=True)
    stages = ["dup_guids", "dup_node_desc", "lids", "sm", "nodes_info", "pkey",
              "vs_cap_gmp", "links", "speed_width_check", "temp_sensing", "virt"]
    # "pm" excluded
    skip = ["--skip=%s" % s for s in stages]
    cmd_args = ["ibdiagnet", "-o", directory] + skip
    logging.info("exec %r", " ".join(cmd_args))

    subprocess.run(["sudo", "ibdiagnet", "-o", directory] + skip,
                   stdout=subprocess.PIPE,
                   stderr=subprocess.PIPE,
                   )

def read_ibdiagnet_db(directory="/var/cache/ib-mon", cachettl=15):
    marker = pathlib.Path(os.path.join(directory, "ibdiagnet2.db_csv"))
    lockfile = pathlib.Path("/var/lock/ib-mon-ibdiagnet-export.lock")
    fdlock = flock(lockfile)

    use_cache = False
    if cachettl and marker.exists():
       stat = marker.stat()
       if stat.st_mtime + cachettl >= time.time():
           logging.info("cache hit")
           use_cache = True

    if not use_cache:
        export_ib_diag(directory)

    file=os.path.join(directory, "ibdiagnet2.db_csv")
    with open(file) as f:
        content = f.read()
    return content

def flock(lockfile, blocking=True):
    try:
        fd = open(lockfile, "r")
    except FileNotFoundError:
        fd = open(lockfile, "w")

    try:
        flags = fcntl.LOCK_EX if blocking else fcntl.LOCK_EX | fcntl.LOCK_NB
        fcntl.flock(fd, flags)
    except BlockingIOError:
        return None

    return fd


def get_url_with_auth_and_seflsigined_cert(url, user, password):
    password_mgr = urllib.request.HTTPPasswordMgrWithPriorAuth()
    password_mgr.add_password(None, url, user, password)
    handler = urllib.request.HTTPBasicAuthHandler(password_mgr)

    myssl = ssl.create_default_context();
    myssl.check_hostname=False
    myssl.verify_mode=ssl.CERT_NONE
    s_handler = urllib.request.HTTPSHandler(context=myssl)

    opener = urllib.request.build_opener(handler, s_handler)
    urllib.request.install_opener(opener)

    with urllib.request.urlopen(url, timeout=10) as f:
        return f.read().decode()


def to_influx_fmt(items, measurement_name, timestamp):
    res = []
    for item in items:
        tags_s = ",".join(["%s=%s" % (k, v) for (k, v) in item["tags"].items()])
        values_s = ",".join(["%s=%s" % (k, v) for (k, v) in item["fields"].items()])
        res.append(measurement_name + ",%s %s %s000000000" % (tags_s, values_s, timestamp))
    return res


def get_ca_params():
    ssl_ca_file_centos = "/etc/pki/tls/certs/ca-bundle.crt"
    if pathlib.Path(ssl_ca_file_centos).exists():
        # centos 8
        return { 'cafile': ssl_ca_file_centos }
    else:
        # ubuntu
        return { 'capath': "/etc/ssl/certs/" }


def get_rt_host(fqdn, token):
    query = """
    { query: object(search: "%s" ) { 
        name
        fqdn
        etags {
            tag
            id
            parent_id
        }
        itags {
            tag
            id
            parent_id
        }
    }}
    """
    data = query % fqdn
    ret = invapi_query(data, token)
    logging.debug("get_rt_host: invapi reply: '%s'", ret)
    host=ret['query'][0]
    return host


def get_ufm_hosts_for_cluster(cluster_tag, token):
    query = """
    { query: object(code: "{%s} and {сервер АСУ} and {Linux}" ) {
        name
        fqdn
    }}
    """
    data = query % cluster_tag
    ret = invapi_query(data, token)
    logging.debug("get_ufm_hosts_for_cluster: invapi reply: '%s'", ret)
    return ret['query']

def my_fqdn():
    result = subprocess.run(["hostname", "-f"], stdout=subprocess.PIPE)
    return result.stdout.decode().strip()


def find_tag_in_host(host, tag):
    for taginfo in host["itags"]:
        if taginfo["tag"] == tag:
            return taginfo
    return None


def get_cluster_tag_from_host(host):
    root_cluster_tag = find_tag_in_host(host, INFINIBAND_ROOT_CLUSTER_TAG_NAME)
    if root_cluster_tag:
        for taginfo in host["etags"]:
            if taginfo["parent_id"] == root_cluster_tag["id"]:
                return taginfo
    return None


def get_ufm_hosts_for_cluster_with_host_cached(fqdn, token, cachedir=False, cachettl=300):
    cache = pathlib.Path("/var/cache/ib-mon/ufm_hosts.cache")
    if cachettl and cache.exists():
        stat = cache.stat()
        if stat.st_mtime + cachettl >= time.time():
            logging.info("cache hit")

            with open(cache) as f:
                try:
                    return json.load(f)
                except:
                    pass

    ufm_hosts_names = get_ufm_hosts_for_cluster_with_host(fqdn, token)

    newcache = pathlib.Path("/var/cache/ib-mon/ufm_hosts.cache.new")
    with open(newcache, "w") as f:
        f.write(json.dumps(ufm_hosts_names))
    os.rename(newcache, cache)

    return ufm_hosts_names


def get_ufm_hosts_for_cluster_with_host(fqdn, token):
    ufm_hosts_names = {}
    host = get_rt_host(fqdn, token)
    if host:
        cluster_tag = get_cluster_tag_from_host(host)
        if cluster_tag:
            ufm_hosts = get_ufm_hosts_for_cluster(cluster_tag["tag"], token)
            if ufm_hosts:
                for host in ufm_hosts:
                    ufm_hosts_names[host['name']] = {
                            "fqdn": host['fqdn'],
                            "cluster": cluster_tag["tag"],
                        }
                    ufm_hosts_names[host['fqdn']] = {
                            "fqdn": host['name'],
                            "cluster": cluster_tag["tag"],
                        }
    return ufm_hosts_names


def split_to_sections(content):
    inside = False
    data = []
    sections = {}
    for line in content.splitlines():
        if line.startswith("START_"):
            secname = line[6:]
            if inside:
                raise Exception()
            inside = True
            continue
        if line.startswith("END_"):
            if not inside:
                raise Exception()
            inside = False
            sections[secname] = data

            data = []
            secname = ""
        elif inside:
            data.append(line)
    return sections


def parse_section(data):
    reader = csv.DictReader(data, delimiter=',', skipinitialspace=True)
    res = []
    # next(reader, None)  # header
    for row in reader:
        prow = {}
        for k, v in row.items():
            prow[k] = v
        res.append(prow)
    return res


def parse_values(data, data_type):
    res = {}
    for k, v in data.items():
        cfg = data_type.get(k)
        if cfg is None:
            continue
        if len(cfg) != 0:
            k = cfg

        if v.startswith("0x"):
            v = int(v, 16)
            if v > 0x7FFFFFFFFFFFFFFF:  # two's complement
                v -= 0x10000000000000000
        elif v == "N/A":
            v = -2
        else:
            v = int(v)
        res[k] = v
    return res


def collect_links_and_nodes(data):
    links = {}
    port_properties = {}
    for port in parse_section(data["PORTS"]):
        link_from = (port["NodeGuid"], port["PortNum"])
        links[link_from] = (None, None)
        values = parse_values(port.copy(), PORTS_TYPES)
        port_properties[link_from] = {
                "speed": get_speed(values['LinkSpeedActive'], values['LinkWidthActive']),
                "base_speed_alias": get_link_base_speed_alias(values['LinkSpeedActive']),
                "max_base_speed_alias": get_link_max_base_speed_alias(values['LinkSpeedSup']),
            }

    for link in parse_section(data["LINKS"]):
        link_from = (link["NodeGuid1"], link["PortNum1"])
        link_to = (link["NodeGuid2"], link["PortNum2"])
        links[link_from] = link_to
        links[link_to] = link_from

    (switch_to_descr, hca_to_descr) = collect_nodes(data)
    return (links, switch_to_descr, hca_to_descr, port_properties)


def collect_nodes(data):
    switches_guid = []
    for switch in parse_section(data["SWITCHES"]):
        switches_guid.append(switch["NodeGUID"])

    switch_to_descr = {}
    hca_to_descr = {}
    for node in parse_section(data["NODES"]):
        if node["SystemImageGUID"] == node["NodeGUID"] and node["NodeGUID"] == node["PortGUID"]:
            if node["NodeGUID"] in switches_guid:
                if node["NodeDesc"].find(" ") >= 0:
                    continue
                switch_to_descr[node["SystemImageGUID"]] = node["NodeDesc"].lower()
            else:
                hca_to_descr[node["SystemImageGUID"]] = node["NodeDesc"].lower()

    return (switch_to_descr, hca_to_descr)


def get_port_state(state):
    map_state = {
        1: 2, # "Down",
        2: 2, # "Initialize",
        3: 2, # "Armed",
        4: 1, # "Active"
    }
    state = int(state)
    if state in map_state:
        return map_state[state]
    return "N/A"


def get_port_phy_state(state):
    map_state = {
        1: 2, # "Sleep",
        2: 2, # "Polling",
        3: 2, # "Disabled",
        4: 2, # "PortConfigurationTraining",
        5: 1, # "LinkUp",
        6: 2, # "LinkErrorRecovery",
        7: 3, # "PhyTest",
    }
    state = int(state)
    if state in map_state:
        return map_state[state]
    return "N/A"

# for LinkSpeedActv
def get_link_base_speed_mbps(link_speed_actv):
    return get_link_base_speed_by_alias(get_link_base_speed_alias(link_speed_actv)) * 1_000


# for LinkSpeedActv
def get_link_base_speed_alias(link_speed_actv):
    map_speed = {
        1: "SDR",
        2: "DDR",
        4: "QDR",
    }
    map_speed_ext = {
        1: "FDR",
        2: "EDR",
        4: "HDR",
    }

    extended_speed = (int(link_speed_actv)>>8) & 0xFF
    if extended_speed in map_speed_ext:
        speed = map_speed_ext[extended_speed]
    else:
        regular_speed = int(link_speed_actv) & 0xFF
        if regular_speed in map_speed:
            speed = map_speed[regular_speed]
        else:
            speed = "unknown-speed"
    return speed


# for LinkSpeedActv
def get_link_base_speed_by_alias(alias):
    map = {
        "SDR": 2.5,
        "DDR": 5.0,
        "QDR": 10.0,
        "FDR": 14.0625,
        "EDR": 25.78125,
        "HDR": 53.125,
    }

    if alias in map:
        return map[alias]
    else:
        return 0


# for LinkWidthActv
def get_link_mult(width):
    map_link_width_active_mult = {
        1: 1,
        2: 4,
        4: 8,
        8: 12,
    }
    width = int(width) & 0xFF
    if width in map_link_width_active_mult:
        mult = map_link_width_active_mult[width]
    else:
        mult = 0

    return mult


def get_speed(speed, width):
    return int(get_link_base_speed_mbps(speed) * get_link_mult(width))


# for LinkSpeedSupported
def get_link_max_base_speed_alias(link_speed_sup):
    """
LinkSpeedExtSupported (part of LinkSpeedSup)
        0: No extended speed supported
    """
    map_speed = {
        1: "SDR", # 2.5 Gbps
        3: "DDR", # 2.5 or 5.0 Gbps
        5: "QDR", # 2.5 or 10.0 Gbps
        7: "QDR", # 2.5 or 5.0 or 10.0 Gbps
    }
    map_speed_ext = {
        1: "FDR", # 14.0625 Gbps (FDR)
        2: "EDR", # 25.78125 Gbps (EDR)
        3: "EDR", # 14.0625 Gbps (FDR) or 25.78125 Gbps (EDR)
        4: "HDR", # 53.125 Gbps (HDR)
        5: "HDR", # 14.0625 Gbps (FDR) or 53.125 Gbps (HDR)
        6: "HDR", # 25.78125 Gbps (EDR) or 53.125 Gbps (HDR)
        7: "HDR", # 14.0625 Gbps (FDR), 25.78125 Gbps (EDR) or 53.125 Gbps (HDR)
    }

    extended_speed = (int(link_speed_sup)>>8) & 0xFF
    if extended_speed in map_speed_ext:
        speed = map_speed_ext[extended_speed]
    else:
        regular_speed = int(link_speed_sup) & 0xFF
        if regular_speed in map_speed:
            speed = map_speed[regular_speed]
        else:
            speed = "unknown-speed"
    return speed


@retry(urllib.error.URLError)
def invapi_query(data, token=None):
    req = urllib.request.Request("https://ro.racktables.yandex-team.ru/api",
        data=json.dumps({"query": data}).encode('ascii'))

    #logging.debug("invapi_query: data: '%s'", data)

    if token:
        req.add_header("Authorization", "OAuth " + token)
    req.add_header('Content-Type', 'application/json')
    req.add_header('User-Agent', 'ib-mon')

    ca_params = get_ca_params()
    try:
        with urllib.request.urlopen(req, timeout=30, **ca_params) as f:
            ret = json.load(f)["data"]
    except urllib.error.URLError as e:
        ret = None
        logging.error("invapi request error: %r", e)
        raise e

    return ret

def basedir():
    return os.path.dirname(os.path.realpath(__file__))

def read_config(file=None):
    config = configparser.ConfigParser()
    config.read(file if file else "/etc/ib-mon.conf")
    return config


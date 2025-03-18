import json
import socket
import urllib2
import re

from gaux.aux_utils import run_command

ISS_ILIST_URL = "http://127.0.0.1:25536/instances/active"


class EInstanceTypes(object):
    BSCONFIG = "bsconfig"
    ISS = "iss"
    HOSTPORT = "hostport"
    FAKE = "fake"
    ALL = [BSCONFIG, ISS, FAKE]


def _update_bsconfig_instance_env(env):
    # fill some log paths
    IKEY = "a_itype_"
    itypes = filter(lambda x: x.startswith(IKEY), env["tags"])
    if len(itypes) == 1:  # Skip old-style instances without autotags
        itype = itypes[0][len(IKEY):]
        if itype == "base":
            env["loadlog"] = "/ssd/www/logs/current-loadlog-base-%s" % env["port"]
        elif itype == "int":
            env["loadlog"] = "/ssd/www/logs/current-loadlog-w-int-%s" % env["port"]
        elif itype == "mmeta":
            env["loadlog"] = "/ssd/www/logs/current-loadlog-base-%s" % env["port"]
        elif itype == "fusion":
            env["loadlog"] = "/ssd/www/logs/current-loadlog-fusion-%s" % env["port"]

    # fill generaor
    TKEY = "a_topology_version-"
    tags = filter(lambda x: x.startswith(TKEY), env["tags"])
    if len(tags) == 1:  # skip old-style instacnes without autotags
        tag = tags[0][len(TKEY):]
        stable_m = re.match('stable-(\d+)-r(\d+)', tag)
        if stable_m:
            major_tag = int(stable_m.group(1))
            minor_tag = int(stable_m.group(2))
            env["major_tag"] = major_tag
            env["minor_tag"] = minor_tag
            env["version"] = 1000000 * major_tag + minor_tag
        else:
            env["major_tag"] = 0
            env["minor_tag"] = 0
            trunk_m = re.match('trunk-(\d+)', tag)
            if trunk_m:
                rev = int(trunk_m.group(1))
                env['version'] = 1000000000 + rev

    GKEY = "a_topology_group-"
    group = filter(lambda x: x.startswith(GKEY), env['tags'])
    if len(group) == 1:
        group_name = group[0][len(GKEY):]
        env['group'] = group_name


def get_bsconfig_instances_env():
    retcode, out, err = run_command(["/db/bin/bsconfig", "list", "--json"])
    all_instances = json.loads(out)

    result = {}
    for k in all_instances.keys():
        env = all_instances[k]
        _update_bsconfig_instance_env(env)

        host = k.split(':')[0]
        port = int(k.split('@')[0].split(':')[1])
        result[(host, port)] = {
            "type": EInstanceTypes.BSCONFIG,
            "env": env,
        }

    return result


def _update_iss_instance_env(env):
    TKEY = "a_topology_version-"

    tags = filter(lambda x: x.startswith(TKEY), env["properties"]["tags"].split(' '))
    if len(tags) != 1:
        return

    IKEY = "a_itype_"
    itypes = filter(lambda x: x.startswith(IKEY), env["properties"]["tags"].split(' '))
    if len(itypes) == 1:  # Skip old-style instances without autotags
        itype = itypes[0][len(IKEY):]
        if itype == "base":
            env["loadlog"] = "/ssd/www/logs/current-loadlog-base-%s" % env["properties"]["port"]
        elif itype == "int":
            env["loadlog"] = "/ssd/www/logs/current-loadlog-w-int-%s" % env["properties"]["port"]
        elif itype == "mmeta":
            env["loadlog"] = "/ssd/www/logs/current-loadlog-base-%s" % env["properties"]["port"]
        elif itype == "fusion":
            env["loadlog"] = "/ssd/www/logs/current-loadlog-fusion-%s" % env["properties"]["port"]

    tag = tags[0][len(TKEY):]

    stable_m = re.match('stable-(\d+)-r(\d+)', tag)
    if stable_m:
        major_tag = int(stable_m.group(1))
        minor_tag = int(stable_m.group(2))
        env["major_tag"] = major_tag
        env["minor_tag"] = minor_tag
        env["version"] = 1000000 * major_tag + minor_tag
    else:
        env["major_tag"] = 0
        env["minor_tag"] = 0
        trunk_m = re.match('trunk-(\d+)', tag)
        if trunk_m:
            rev = int(trunk_m.group(1))
            env['version'] = 1000000000 + rev

    GKEY = "a_topology_group-"
    group = filter(lambda x: x.startswith(GKEY), env["properties"]["tags"].split(' '))
    if len(group) == 1:
        group_name = group[0][len(GKEY):]
        env['group'] = group_name

    env["tags"] = env["properties"]["tags"].split(' ')

    return env


def get_iss_instances_env():
    out = urllib2.urlopen(ISS_ILIST_URL, timeout=10).read()
    all_instances = json.loads(out)

    result = {}
    for k in all_instances.keys():
        env = all_instances[k]

        _update_iss_instance_env(env)

        host, _, port = k.partition(':')
        port = int(port)
        result[(host.partition('.')[0], port)] = {
            "type": EInstanceTypes.ISS,
            "env": env,
        }

    return result


def get_fake_instances_env():
    return {
        (socket.gethostname().split('.')[0], 65535): {
            "type": EInstanceTypes.FAKE,
            "env": {"major_tag": 0, "minor_tag": 0},
        },
    }


def get_all_instances_env():
    try:
        bsconfig_data = get_bsconfig_instances_env()
    except:
        bsconfig_data = {}

    try:
        iss_data = get_iss_instances_env()
    except:
        iss_data = {}

    try:
        fake_data = get_fake_instances_env()
    except:
        fake_data = {}

    return dict(bsconfig_data.items() + iss_data.items() + fake_data.items())

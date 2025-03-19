# coding: utf8
import os
import os.path
import random
import time
import logging

import requests

log = logging.getLogger(__name__)


def _cauth_get(url):
    """
    Gets an url from cauth and caches it in /tmp for an hour ±10 mins.
    The non-fixed cache time is so that they do not get invalidated
    simultaneously on the whole cluster.
    """
    cache_name = f"/var/tmp/salt-cauth-cache-{url.replace('/', '-')}"

    try:
        cache_time = os.path.getmtime(cache_name)
    except OSError:
        cache_time = 0.0

    if time.time() - cache_time < random.randint(3000, 4200):  # 3600 ± 600 secs
        info = open(cache_name).read()
    else:
        resp = requests.get(
            f"https://cauth.yandex.net:4443/{url}/",
            verify="/etc/ldap/certs/cafile.pem",
        )
        resp.raise_for_status()
        info = resp.text
        old_mask = os.umask(0)
        open(cache_name, "w").write(info)
        os.umask(old_mask)

    return [
        x
        for x in [s.strip() for s in info.strip().split("\n")]
        if x and not x.startswith("#")
    ]


def cauth_info():
    result = {}

    # Users who can login to the system
    info = _cauth_get("passwd/serverusers")
    users = [x.split(":")[0] for x in info]

    result["users"] = sorted(users)

    # admins
    info = _cauth_get("passwd/serveradmins")
    admins = [x.split(":")[0] for x in info]

    result["admins"] = sorted(admins)

    # sudoers
    info = _cauth_get("sudoers")
    data = [x.split(" ")[0] for x in info]

    groups = {}
    ginfo = _cauth_get("group")
    for line in ginfo:
        fields = line.split(":")
        groups[fields[0]] = fields[-1].split(",")

    # expand the groups
    sudoers = []
    for line in data:
        if line.startswith("%"):
            gname = line[1:]
            try:
                sudoers.extend(groups[gname])
            except KeyError:
                pass
        else:
            sudoers.append(line)

    result["sudoers"] = sorted(set(sudoers))

    return {"cauth": result}

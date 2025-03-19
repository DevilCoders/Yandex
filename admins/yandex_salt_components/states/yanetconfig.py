# encoding: utf-8
"""
Generate file /etc/network/interfaces
===============================================
.. code-block:: yaml
    /etc/network/interfaces:
      yanetconfig.managed
    or
    /etc/network/interfaces:
      yanetconfig.managed:
        - mtu: 1450
        - iface: eth0
        - nojumbo: False
        - do10: True
        - bridged: False
        - virtual: False
        - fastbone: vlan777
        - noantimartians: False
        - nolo: False
        - bonding: bond0
mtu is defailt is 8950 (or 1450 if nojumbo is set True)
fastbone is interface name.
bonding is bond interfaces.
More detail option description options see in help yanetconfig.
"""

__author__ = "Andrey Trubachev <d3rp@yandex-team.ru>"

import sys
import tempfile
import logging
from types import SimpleNamespace
from typing import Any

log = logging.getLogger(__name__)

__grains__: dict[str, Any] = {}
__states__: dict[str, Any] = {}


def managed(
    name,
    noantimartians=None,
    nolo=False,
    iface="eth0",
    nojumbo=False,
    virtual=None,
    mtu=0,
    fastbone=None,
    bonding=None,
    bridged=None,
    narodmiscdom0=None,
    do10=None,
):
    # runtime imports
    sys.path.append("/usr/lib/yandex/python-netconfig/")

    import interfaces
    from interfaces import GenerationParameters
    from interfaces.network_info import NetworkInfo
    from interfaces.generators.interfaces_generator import InterfacesGenerator
    from interfaces.conductor import Conductor

    sys.path.remove("/usr/lib/yandex/python-netconfig/")
    # end runtime imports

    output_format = None
    if __grains__["os"] == "Ubuntu":  # pylint disable:
        output_format = "ubuntu"
    if __grains__["os"] == "CentOS" or __grains__["os"] == "RedHat":
        output_format = "redhat"
    fqdn = __grains__["fqdn"]

    ret = {"name": name, "changes": {}, "result": None, "comment": ""}

    if output_format is None:
        ret["result"] = False
        ret["comment"] = f"Cannot define OS. {__grains__['os']=}"
        return ret

    options = {
        "noantimartians": noantimartians,
        "nolo": nolo,
        "iface": iface,
        "nojumbo": nojumbo,
        "output_format": output_format,
        "fqdn": fqdn,
        "virtual": virtual,
        "mtu": mtu,
        "fastbone": fastbone,
        "bonding": bonding,
        "bridged": bridged,
        "debug": None,
        "output": None,
        "narodmiscdom0": narodmiscdom0,
        "do10": do10,
        "multiqueue": None,
        "use_tables": False,
    }

    opt = SimpleNamespace(**options)
    tmp_file = tempfile.NamedTemporaryFile()

    try:
        conductor = Conductor("production", None)
        host_info = conductor.host_info(fqdn)
        params = GenerationParameters(opt, host_info)
        generator = InterfacesGenerator(params, "/etc/netconfig.d")
        network_info = NetworkInfo(conductor, None)
        data = generator.generate(network_info)
        interfaces.output.write_all(tmp_file.name, data)
        with open(tmp_file.name, "r") as config_file:
            ret = __states__["file.managed"](name=name, contents=config_file.read())
        return ret
    except Exception as exc:
        ret["result"] = False
        ret["comment"] = "Failed generate network config. {0}".format(exc)
        return ret

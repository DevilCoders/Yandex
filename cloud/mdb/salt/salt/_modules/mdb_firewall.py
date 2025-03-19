# -*- coding: utf-8 -*-
"""
MDB module with some useful network related functions.
"""

import jinja2
import os

__salt__ = {}

external_access_config_template = """domain ip6 {
    table filter {
        chain INPUT {
{%   for project_id, action in project_ids_with_actions %}
            proto tcp
            mod u32 u32 "0x10&0xffffffff={{ project_id }}"
            interface {{ management_iface }} mod tcp
            saddr 2a02:6b8:c00::/40
            dport ( {{ ports | join(" ") }} ) {{ action }};
{%   endfor %}
        }
    }
}
"""


def __virtual__():
    return True


def render_external_access_config():
    ports = _get_external_ports()
    if ports:
        management_iface = management_interface()
        project_ids_with_actions = sorted(__salt__['mdb_network.get_external_project_ids_with_action']().items())

        t = jinja2.Template(external_access_config_template)

        r = t.render(
            project_ids_with_actions=project_ids_with_actions,
            management_iface=management_iface,
            ports=ports,
        )

        return r
    else:
        return None


def management_interface():
    if is_porto():
        return "eth0"
    elif is_compute():
        return "eth1"
    else:
        return next(iter(filter(lambda iface: iface != 'lo', os.listdir('/sys/class/net'))))


def is_porto():
    return __salt__['dbaas.is_porto']()


def is_compute():
    return __salt__['dbaas.is_compute']()


def _pillar(key, default=None):
    return __salt__['pillar.get'](key, default=default)


def _get_external_ports():
    cluster_type = _pillar('data:dbaas:cluster_type')
    subcluster_name = _pillar('data:dbaas:subcluster_name')
    if cluster_type == 'greenplum_cluster' and subcluster_name == 'segment_subcluster':
        ports = []
        for item in __salt__['mdb_greenplum.get_segment_info_for_config_deploy_v2']():
            if item['port'] == 0:
                return None
            else:
                ports.append(item['port'])
    else:
        ports = _pillar('firewall:ports:external')
    return ports

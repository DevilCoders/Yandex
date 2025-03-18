# -*- coding: utf-8 -*-
"""
CURDB-based transport for balancer config generator.
Generator uses it for retrieving instances data straight from CURDB

To use this file, you need to checkout balancer-gencfg repository.
To do it, run at gencfg repository's root:

./checkout.sh

balancer-gencfg repo: https://git.qe-infra.yandex-team.ru/projects/NANNY/repos/balancer-gencfg/
"""

from collections import defaultdict
import sys

from core.db import CURDB
from gaux.aux_resolver import resolve_host
import gaux.aux_hbf


from balancer_config import GENCFG_API_TIMEOUT
from src.transports.base_db_transport import InstanceDbTransport, Instance
from src.transports.gencfg_api_transport import GencfgApiTransport
from src.groupmap import TGroupToTagMapping


class CurdbTransport(InstanceDbTransport):
    """
    Transport for getting instances straight from CURDB
    """

    def __init__(self):
        super(CurdbTransport, self).__init__()

        self.db = CURDB
        self.api_transport = GencfgApiTransport(req_timeout=GENCFG_API_TIMEOUT, attempts=3)

        # GENCFG-961: create mapping <instance.name> -> <list of tags>
        self.instance_tags = defaultdict(set)

    def _instances_from_gencfg_instances(self, instances):
        result = []
        for instance in instances:
            group = CURDB.groups.get_group(instance.type)
            instance_hbf_info = gaux.aux_hbf.generate_hbf_info(group, instance)

            # GENCFG-961: generate instance tags from searcherlookup
            if instance not in self.instance_tags:
                group_searcherlookup = group.generate_searcherlookup()
                for tag in group_searcherlookup.itags_auto:
                    for slookup_instance in group_searcherlookup.itags_auto[tag]:
                        self.instance_tags[slookup_instance].add(tag)

            hbf_mtn_hostname = None
            hbf_mtn_ipv6addr = None
            if ('interfaces' in instance_hbf_info) and ('backbone' in instance_hbf_info['interfaces']):
                hbf_mtn_hostname = instance_hbf_info['interfaces']['backbone']['hostname']
                hbf_mtn_ipv6addr = instance_hbf_info['interfaces']['backbone']['ipv6addr']

            result.append(Instance(
                instance.host.name,
                instance.port,
                power=instance.power,
                dc=instance.host.dc,
                location=instance.host.location,
                domain=instance.host.domain,
                ipv4addr=None if instance.host.ipv4addr == "unknown" else instance.host.ipv4addr,
                ipv6addr=None if instance.host.ipv6addr == "unknown" else instance.host.ipv6addr,
                hbf_mtn_hostname=hbf_mtn_hostname,
                hbf_mtn_ipv6addr=hbf_mtn_ipv6addr,
                tags=self.instance_tags[instance],
            ))
        return result

    def get_group_instances(self, group_name, gencfg_version=None):
        """
        :param str group_name:
        :rtype: list of balancer_gencfg.transports.base_db_transport.Instance
        """
        del gencfg_version

        tag = self.mapping.gettag(TGroupToTagMapping.ETypes.GROUP, group_name)

        if tag == "trunk":  # get from local db
            instances = CURDB.groups.get_group(group_name).get_instances()

            return self._instances_from_gencfg_instances(instances)
        else:
            return self.api_transport.get_group_instances(group_name, tag)

    def get_intlookup_instances(self, intlookup_name, instance_type=None, gencfg_version=None):
        """
        :param intlookup_name: existing intlookup name, e.g. intlookup-msk-imgs.py
        :param instance_type: base or int for specific type, None for all instances
        :rtype: list of balancer_gencfg.transports.base_db_transport.Instance
        """
        print "Not implemented or already broken"
        sys.exit(1)
        """
        del gencfg_version

        if instance_type and instance_type not in ['base', 'int']:
            raise Exception("Unknown type for intlookup: {}".format(instance_type))

        tag = self.mapping.gettag(TGroupToTagMapping.ETypes.INTLOOKUP, group_name)

        if tag == "trunk":
            intlookup = CURDB.intlookups.get_intlookup(intlookup_name)

            if instance_type == 'int':
                instances = intlookup.get_int_instances()
            elif instance_type == 'base':
                instances = intlookup.get_base_instances()
            else:
                instances = intlookup.get_instances()
            return self._instances_from_gencfg_instances(instances)
        else:
            return self.api_transport.get_group_instances(group_name, tag)
        """

    def describe_gencfg_version(self, gencfg_version=None):
        """
        Returns a string that uniquely identifies commit of given gencfg version
        and which as human friendly as it's possible (tries to make it based on nearest tag name).

        :param gencfg_version:
        :rtype: str
        """
        del gencfg_version

        return CURDB.get_repo().describe_commit()

    def resolve_host(self, host, families, use_curdb=True, gencfg_version=None):
        del gencfg_version
        return resolve_host(host.hostname, families, use_curdb)

    def close_session(self):
        """
        Close connection to gencfg api to avoid race in reading from socket
        """
        self.api_transport.close_session()

    def name(self):
        return "curdb"

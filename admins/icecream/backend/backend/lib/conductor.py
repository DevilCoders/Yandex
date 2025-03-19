#!/usr/bin/env python3
'''Get data from c.yandex-team.ru'''

import logging
import conductor_client as c
import lib

LOG = logging.getLogger()

class Conductor():
    """Conductor client wrapper"""

    def __init__(self):
        self.conf = lib.utils.load_config()
        c.set_path(self.conf.conductor.path)
        c.set_token(self.conf.conductor.token)

    @staticmethod
    def host_info(fqdn):
        """
        Query conductor about fqdn
        if host not exists return empty response {}
        """
        host = c.Host(fqdn=fqdn)
        host_info = {}
        try:
            if host.exists():
                host_info = host.as_json()
                host_info["project"] = host.group.project.name
                if host.as_json().get("datacenter"):
                    host_info["datacenter"] = host.datacenter.name
                    try:
                        host_info["root_datacenter"] = host.datacenter.parent.name
                    except c.models.ApiResource.NotFound:
                        # if root_datacenter not set, guess if from name like 'sas1.1.1'
                        # hardcode first 3 letter here,
                        # i don't know exception from this rule
                        host_info["root_datacenter"] = host.datacenter.name[:3]
        except Exception as exc: # pylint: disable=broad-except
            LOG.exception("Failed to query conductor info for %s: %s", fqdn, exc)
        return host_info

    @staticmethod
    def host_create(fqdn, group=None, project="media", datacenter=None):
        """Create conductor host"""

        short_name = lib.utils.name_from_fqdn(fqdn)
        if not group:
            group = "{}-lost".format(project)

        host = c.Host(
            fqdn=fqdn,
            group=c.Group(name=group),
            short_name=short_name,
        )

        if datacenter:
            host.datacenter = c.Datacenter(name=datacenter)

        if not host.save():
            return host.errors()
        return None

    @staticmethod
    def host_delete(fqdn):
        """Delete conductor host"""
        host = c.Host(fqdn=fqdn)
        if not host.delete():
            return host.errors()
        return None

def _test():
    # before run check config https://github.yandex-team.ru/devtools/python-conductor-client
    c_cli = Conductor()
    c.set_path(c.client.config.get('conductor_client', 'path'))
    c.set_token(c.client.config.get('conductor_client', 'token'))

    fqdn = 'test0034.mdt.yandex.net'
    print("host_create errors: ", c_cli.host_create(fqdn=fqdn))
    print("Host info: ", c_cli.host_info(fqdn))
    print("Delete: ", c_cli.host_delete(fqdn))
    print("host_create errors: ", c_cli.host_create(fqdn=fqdn, datacenter="sas"))
    print("Host info: ", c_cli.host_info(fqdn))
    print("Delete errors: ", c_cli.host_delete(fqdn))

if __name__ == '__main__':
    _test()

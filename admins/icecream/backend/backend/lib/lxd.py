#!/usr/bin/env python3
'''lxd'''

import sys
import logging
from types import SimpleNamespace as ns
import dotmap
import pylxd
from lib import utils, exceptions as e, Mongo
# noqa pylint:disable=no-member


LOG = logging.getLogger()
MONGOCLI = Mongo()


class Dom0():
    """Dom0 client class"""

    def __init__(self, hostname):
        '''initialize variable for all methods'''

        self.hostname = hostname.rstrip()
        endpoint = "https://{0!s}:8443".format(self.hostname)
        retry = 3
        client = None
        while not client or retry > 0:
            retry -= 1
            try:
                client = pylxd.Client(
                    endpoint=endpoint,
                    cert=('cert/lxd.crt', 'cert/lxd.key'),
                    verify=False
                )
                client.authenticate("0123456789")  # WTF: XXX
            except:  # pylint: disable=bare-except
                exc = sys.exc_info()
                LOG.debug(exc)
                if retry <= 0:
                    raise e.IceLxdClientError(
                        detail="Failed to create lxd client: {}".format(exc),
                        ext={"log_exception": True}
                    )
        self.client = client


    def get_hostname(self):
        '''return hostname'''
        return self.hostname


    def get_container(self, name):
        '''get container info'''
        container = self.client.api.containers[name].get().json()["metadata"]
        LOG.debug("Container metadata %s: %s", name, container)
        meta = ns(**container)
        cpu_list = list(set(
            #    limits.cpu is string, "" or cpu id range like "0,0" or "0,1,3"
            #    limits.cpu treat as floats as valid cpu numbers, handle this case
            int(float(x)) for x in \
                meta.config["limits.cpu"] and meta.config["limits.cpu"].split(",")
        ))
        vhost = {
            "name": meta.name,
            "cpu": len(cpu_list),
            "cpuList": cpu_list,
            "ram": utils.convert_mem(meta.config.get("limits.memory", "0")),
            "project": meta.config.get("user.project", "media").strip(),
            "image": meta.config["image.release"],
            "profiles": meta.profiles,
            "network": meta.expanded_devices.get("eth0", {}).get("parent", "unknown"),
            "fqdn": meta.name.replace("--", '.') + '.yandex.net',
            "status": meta.status,
            "diskSize": int(meta.config.get("user.disk_size", 0)),
            "filesystem": meta.config.get("user.filesystem", "ext4"),
        }
        return vhost


    def get_containers(self):
        '''get all containers on host'''
        meta = self.client.api.containers.get().json()["metadata"]
        LOG.debug("Containers metadata from %s: %s", self.hostname, meta)
        vhosts = []
        for host in meta:
            host = host.split("/")[3]
            try:
                vhost = self.get_container(host)
                vhosts.append(vhost)
            except pylxd.exceptions.NotFound as exc:
                LOG.debug("Skip %s: %s", host, exc)
        return vhosts


    def container_get_status(self, name):
        '''return lxd container current status'''
        container = self.client.api.containers[name].get().json()
        return container["metadata"]["status"]

    def container_set_status(self, name, status):
        '''change current container status'''
        container = self.client.containers.get(name)
        try:
            LOG.info("Set container %s status: %s", name, status)
            status = getattr(container, status)(wait=True)
        except Exception as exc:  # pylint: disable=broad-except
            LOG.error("Failed to set container %s status %s: %s", name, status, exc)

        return status

    def container_config(self, name):
        '''return container config'''

        container = self.client.containers.get(name)
        config = container.config
        LOG.debug("Dom0 %s return container %s config: %s", self.hostname, name, config)

        return config

    def container_update(self, name, config):
        """Update container stats"""

        container = self.client.containers.get(name)
        config_diff = dict(set(config.items()) - set(container.config.items()))
        LOG.debug("Update container %s, config diff: %s", name, config_diff)
        container.config = config
        container.save(wait=True)
        result = {"status": 200, "msg": "{}: config updated".format(name)}

        # recheck config
        container = self.client.containers.get(name)
        not_updated = []
        for k in config_diff:
            v_want = config[k]
            v_have = container.config.get(k, None)
            if str(v_have) != str(v_want):  # ugly check :-(
                not_updated.append("{}={} expect {}".format(k, v_have, v_want))
        if not_updated:
            result = {
                "status": 500,
                "msg": "failed to updated {}: {}".format(name, ",".join(not_updated))
            }
        return result

    def get_storage_pools(self):
        config = self.client.api.storage_pools["lxd"].get().json()
        return config

    def set_storage_pool_fs(self, fs):
        import json
        config = self.client.api.storage_pools["lxd"].get().json()["metadata"]["config"]
        config["volume.block.filesystem"] = fs
        config = {"config": config}
        LOG.debug("Set storage-pool fs config %s", config)
        ret = self.client.api.storage_pools["lxd"].put(json.dumps(config))
        LOG.debug("Set storage-pool return %s", ret)


    def container_create(self, name, args):
        """Create container"""

        fqdn = name
        name = name.replace('.yandex.net', '').replace(".", "--")
        LOG.debug("Create container: %s (fqdn %s)", name, fqdn)

        config = {
            "name": name,
            "architecture": "x86_64",
            "profiles": ["bootstrap"] + args.profiles,
            "config": {
                "limits.cpu": args.containerCpuList,
                "limits.memory": str(args.ram),
                "user.project": args.project,
                "user.disk_size": str(args.diskSize),
                "user.filesystem": str(args.filesystem),
                "security.privileged": "true",
                "security.nesting": "true"
            },
            "source": {
                "type": "image",
                "alias": "yandex-ubuntu-" + args.image
            },
            "devices": {
                "root": {
                    "path": "/",
                    "pool": "lxd",
                    "type": "disk",
                    "size": str(args.diskSize)
                }
            }
        }

        try:
            container = self.client.containers.create(config, wait=True)
        except pylxd.exceptions.LXDAPIException as exc:
            msg = "Failed to create container {}: {}".format(fqdn, exc)
            LOG.exception(msg)
            result = {"status": 409, "msg": exc}
        except Exception as exc:  # pylint: disable=broad-except
            msg = "Unexpected error while create {}: {}".format(fqdn, exc)
            LOG.exception(msg)
            result = {"status": 500, "msg": exc}
        else:
            LOG.debug("Create container %s with config: %s", fqdn, config)
            container.start(wait=True)
            LOG.debug("Run container %s, container status: %s", fqdn, container.status)
            result = {"status": 200, "msg": "container {} create success".format(fqdn)}

        return result


    def container_delete(self, fqdn):
        '''delete container from host'''

        name = utils.name_from_fqdn(fqdn, kind="lxd")
        try:
            container = self.client.containers.get(name)
            try:
                container.stop(wait=True)
            except:  # pylint: disable=bare-except
                pass
            try:
                container.delete(wait=True)
            except Exception as exc:  # pylint: disable=broad-except
                LOG.debug("Failed to delete container %s: %s", name, exc)
        except pylxd.exceptions.NotFound as err:
            LOG.debug("Container %s not found %s", name, err)


    def container_migrate(self, container_info, new_host, new_fs=None, mode="move"):
        '''
        Migrate container_migrate
        mode allow copy container or move it
        '''

        if new_fs is "":
            new_fs = "ext4"

        if mode not in ['move', 'copy']:
            raise e.IceMigrateModeUnsupportedError(
                detail="Mode {} not supported".format(mode)
            )

        # try get new_dom0 client before stopping source container
        new_dom0 = Dom0(new_host)
        new_dom0_info = dotmap.DotMap(MONGOCLI.get_dom0(new_host)).result
        utils.check_resource(
            len(container_info.cpuList),
            len(new_dom0_info.cpuAvailList),
            'Insufficient CPUs',
        )
        mem_used = sum([x.get('ram', 0) for x in new_dom0_info.containers])
        utils.check_resource(
            container_info.ram,
            new_dom0_info.ram - mem_used,
            'Insufficient memory',
        )
        disk_used = sum([x.get('diskSize', 0) for x in new_dom0_info.containers])
        utils.check_resource(
            container_info.diskSize,
            new_dom0_info.diskSize - disk_used,
            'Insufficient storage',
        )

        container = self.client.containers.get(container_info.name)
        if container.status != 'Stopped':
            container.stop(wait=True)

        new_dom0.set_storage_pool_fs(new_fs)

        # fix source container disk size info
        # srt.migrate method use this info for create
        # target disk appropriate for migrate source
        container.devices['root']['size'] = str(container_info.diskSize)
        LOG.debug("Update %s root size: %s", container_info.fqdn, container.devices)
        container.save(wait=True)
        LOG.debug("Set %s status to 'Migrating'", container_info.fqdn)
        MONGOCLI.container_set_status(container_info.fqdn, "Migrating")

        cpu_set = utils.compute_cpuset(len(container_info.cpuList),
                                       set(),
                                       new_dom0_info.cpuAvailList)

        new_container = container.migrate(new_dom0.client, wait=True)
        new_container.profiles = container.profiles
        new_container.expanded_devices = container.expanded_devices
        new_container.config['volatile.eth0.hwaddr'] = container.config['volatile.eth0.hwaddr']
        new_container.config['limits.cpu'] = cpu_set.config_entry
        new_container.config['user.filesystem'] = new_fs
        new_container.save(wait=True)
        new_container.start(wait=True)

        if mode == "move":
            LOG.debug("Delete old container on %s", container_info.dom0)
            container.delete(wait=True)
        else:
            LOG.debug("Start old container on %s", container_info.dom0)
            if container.status != 'Running':
                container.start(wait=True)

        resp = MONGOCLI.dom0_update(self)
        LOG.debug(
            "Update source dom0 %s after migrate for %s response: %s",
            self.hostname, container_info.fqdn, resp
        )
        resp = MONGOCLI.dom0_update(new_dom0)
        LOG.debug(
            "Update target dom0 %s after migrate for %s response: %s",
            new_host, container_info.fqdn, resp
        )
        return {
            'status': 200,
            'msg': 'Success migrate({}) container {} to {}. New container status {}'
                   .format(mode, container_info.fqdn, new_host, new_container.status)
        }

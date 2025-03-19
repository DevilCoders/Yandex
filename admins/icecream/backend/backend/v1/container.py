"""Container handlers"""
import logging
from types import SimpleNamespace as ns
import dotmap
from lib import lxd, utils, exceptions as e, Remote, Conductor, Mongo, Golem

LOG = logging.getLogger()
MONGOCLI = Mongo()


def post(payload):
    """Create container"""

    LOG.debug("Create container, pyaload: %s", payload)
    args = ns(**payload)

    args.dc = getattr(args, 'dc', 'any')
    args.diskType = getattr(args, 'diskType', 'hdd')
    args.physicalFqdn = getattr(args, 'physicalFqdn', None)
    args.project = getattr(args, 'project', 'media')
    args.filesystem = getattr(args, 'filesystem', 'ext4')
    args.pool = MONGOCLI.project_to_pool(args.project)
    LOG.debug("These are args: %s", args)

    if MONGOCLI.container_info(args.fqdn):
        raise e.IceCreateContainerException(
            status=409,
            detail="container {} exist".format(args.fqdn)
        )

    dom0_hostname = MONGOCLI.dom0_pick(args.physicalFqdn, args)
    dom0_info = MONGOCLI.get_dom0(dom0_hostname)["result"]
    LOG.debug("Dom0 info for %s: %s", args.fqdn, dom0_info)
    dom0_info = ns(**dict(dom0_info))

    cpu_set = utils.compute_cpuset(int(args.cpu), set(), dom0_info.cpuAvailList)
    LOG.debug("Computed cpu_set for container %s: %s", args.fqdn, cpu_set)
    args.containerCpuList = cpu_set.config_entry

    dom0 = lxd.Dom0(dom0_hostname)
    resp = MONGOCLI.dom0_add_container(dom0, args)
    LOG.debug("mongocli.dom0_add_container %s for %s: %s", args.fqdn, dom0_hostname, resp)

    # LOG.debug("Storage pools %s", dom0.get_storage_pools())
    dom0.set_storage_pool_fs(args.filesystem)
    result = dom0.container_create(args.fqdn, args)
    LOG.debug("dom0.container_create %s response: %s", args.fqdn, result)
    if result['status'] == 200:
        resp = MONGOCLI.dom0_update(dom0)
        LOG.debug("Update dom0 for %s response: %s", args.fqdn, resp)
        Golem().create_host(args.fqdn)

    return result, result["status"]  # last value is http status for flask response


def get(fqdn):
    """Get containers info"""
    info = MONGOCLI.container_info(fqdn)
    LOG.debug("container info %s: %s", fqdn, info)
    return info, 200 if info else 404


def delete(fqdn):
    """Delete container"""
    conductor = Conductor()

    LOG.debug("Delete container %s", fqdn)
    info = MONGOCLI.container_info(fqdn)
    result = {"status": 200, "msg": "container {} not exists".format(fqdn)}
    if not info:
        LOG.warning("Failed to delete container %s: not found", fqdn)
        return result, result["status"]

    dom0 = lxd.Dom0(info["dom0"])
    dom0.container_delete(fqdn)
    errors = conductor.host_delete(fqdn)
    if errors:
        LOG.error("Failed to delete conductor host %s: %s", fqdn, errors)
    else:
        LOG.debug("Successfully delete conductor host %s", fqdn)

    resp = MONGOCLI.dom0_update(dom0)
    LOG.debug("Container %s deleted, update dom0 response: %s", fqdn, resp)
    result["msg"] = "Successfully delete container: {}".format(info)
    return result, result["status"]


def put(fqdn, payload):
    """Update container"""

    LOG.debug("Update container %s, payload: %s", fqdn, payload)
    payload = ns(**payload)
    payload.resize = hasattr(payload, "resize") and ns(**payload.resize)
    ct_info = ns(**MONGOCLI.container_info(fqdn))
    LOG.debug("Container info %s", ct_info)
    dom0 = lxd.Dom0(ct_info.dom0)

    # migrate container
    if getattr(payload, "migrate", None):
        if payload.target_fqdn:
            if ct_info.dom0 == payload.target_fqdn:
                raise e.IceMigrateError(
                    status=400,
                    detail='Source and destination dom0 should be different',
                )

            resp = dom0.container_migrate(ct_info,
                                          payload.target_fqdn,
                                          payload.target_fs,
                                          payload.migrate)
        else:
            dom0_info = MONGOCLI.get_dom0(ct_info.dom0)["result"]
            setattr(ct_info, 'dc', dom0_info["datacenter"])
            setattr(ct_info, 'pool', MONGOCLI.project_to_pool(ct_info.project))
            setattr(ct_info, 'diskType', dom0_info["diskType"])
            dom0_hostname = MONGOCLI.dom0_pick(None, ct_info)
            LOG.debug("Migrate to host %s", dom0_hostname)

            resp = dom0.container_migrate(ct_info,
                                          dom0_hostname,
                                          payload.target_fs,
                                          payload.migrate)

        return resp, resp["status"]

    # set container status
    if hasattr(payload, "status"):
        LOG.info("Set container status %s = %s", fqdn, payload.status)
        dom0.container_set_status(ct_info.name, payload.status)
        payload.status = dom0.container_get_status(ct_info.name)
        LOG.debug("Actual container status %s = %s", fqdn, payload.status)
        MONGOCLI.container_set_status(fqdn, payload.status)
        if not payload.resize:
            return {
                "status": 200,
                "msg": "Success set status {} for {}".format(payload.status, fqdn)
            }, 200

    # resize container
    try:
        return _resize_container(dom0, ct_info, payload.resize)
    except e.IceResizeError:
        raise
    except Exception as exc:
        status = getattr(getattr(exc, "response", None), "status_code", 500)
        raise e.IceResizeError(
            status=status,
            detail="Failed to resize container {}: {}".format(fqdn, exc)
        )


def _resize_container(dom0, ct_info, resize):
    fqdn = ct_info.fqdn
    LOG.debug("Try resize container %s", fqdn)

    # get config from dom0 and later save it back
    config = dom0.container_config(ct_info.name)
    dom0_info = dotmap.DotMap(MONGOCLI.get_dom0(ct_info.dom0)).result
    if not dom0_info:
        msg = "Failed to get dom0 info for {}: {}".format(fqdn, ct_info.dom0)
        LOG.debug(msg)
        return {"status": 500, "msg": msg}, 500
    LOG.debug("Update container %s, on host %s", fqdn, dom0_info.fqdn)

    current_cpu_set = set(map(
        #    limits.cpu is string, "" or cpu id range like "0,0" or "0,1,3"
        int, config.get("limits.cpu", "") and config["limits.cpu"].split(',')
    ))
    mongo_update_config = {}
    diff = resize.cpu - len(current_cpu_set)
    if diff != 0:
        LOG.debug("Update 'limits.cpu' for %s, diff %s cpus", fqdn, diff)
        utils.check_resource(diff, len(dom0_info.cpuAvailList), 'Insufficient CPUs')

        cpu_set = utils.compute_cpuset(
            diff, current_cpu_set, dom0_info.cpuAvailList
        )
        LOG.debug("Computed cpu_set for container %s: %s", fqdn, cpu_set)
        config["limits.cpu"] = cpu_set.config_entry
        mongo_update_config.update({
            "cpu": resize.cpu, "cpuList": cpu_set.new_list,
        })

    diff = resize.ram - int(config.get("limits.memory", 0))
    if diff != 0:
        mem_used = sum([x.get('ram', 0) for x in dom0_info.containers])
        LOG.debug("Update 'limits.memory' for %s, diff %s bytes memory", fqdn, diff)
        utils.check_resource(diff, dom0_info.ram - mem_used, 'Insufficient memory')
        config["limits.memory"] = str(resize.ram)
        mongo_update_config.update({"ram": resize.ram})

    diff = resize.diskSize - int(config.get("user.disk_size", 0))
    if diff != 0:
        disk_used = sum([x.get('diskSize', 0) for x in dom0_info.containers])
        LOG.debug("Update 'user.disk_size' for %s, diff %s bytes", fqdn, diff)
        utils.check_resource(diff, dom0_info.diskSize - disk_used, 'Insufficient storage')
        Remote(dom0_info.fqdn).change_lv_size(ct_info.name, resize.diskSize)
        LOG.debug("Success update disk size for %s", fqdn)
        config["user.disk_size"] = str(resize.diskSize)
        mongo_update_config.update({"diskSize": resize.diskSize})

    if mongo_update_config:
        resp = dom0.container_update(ct_info.name, config)
        LOG.debug("Dom0 update container %s response: %s", fqdn, resp)
        if resp["status"] != 200:
            return resp, resp["status"]
        resp = MONGOCLI.dom0_update(dom0)
        LOG.debug("Mongo update dom0 response for %s: %s", fqdn, resp)
    else:
        LOG.info("Container %s, config not changed", fqdn)

    return  {"status": 200, "msg": "update {} success".format(fqdn)}, 200

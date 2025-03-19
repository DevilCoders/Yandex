"""physical create handlers"""
import logging
from lib import Mongo, lxd, exceptions as e


LOG = logging.getLogger()
MONGOCLI = Mongo()


def post(payload):
    """Create physical_host in mongodb"""
    LOG.debug("payload %s", payload)

    resp = {}
    physical_fqdn = payload["hostname"]
    if not physical_fqdn:
        resp = {"status": 400, "msg": "physical_fqdn is empty"}
        LOG.error("Bad Request %s with payload %s", resp, payload)
        return resp, resp["status"]

    method = "query"
    try:
        MONGOCLI.get_dom0(physical_fqdn)
    except e.IceDom0Exception as exc:
        if getattr(exc, "status", 0) == 404:
            method = "create"
            dom0 = lxd.Dom0(physical_fqdn)
            resp = MONGOCLI.create_dom0(dom0)
        else:
            raise exc
    LOG.debug("MongoClient %s response %s", method, resp)
    return resp


def put(payload):
    """Update physical_host in mongodb"""
    LOG.debug("payload %s", payload)

    physical_fqdn = payload["hostname"]

    dom0 = lxd.Dom0(physical_fqdn)
    ret = MONGOCLI.dom0_update(dom0)
    LOG.debug("Update dom0 for %s response: %s", physical_fqdn, ret)

    resp = {"status": 200, "msg": "updated {}".format(physical_fqdn)}
    return resp


"""List physical hosts"""
import logging
from lib import Mongo

MONGOCLI = Mongo()


def search(fqdn='all'):
    """Query info about specified dom0 host"""
    log = logging.getLogger()
    log.debug("Get info about dom0 host %s", fqdn)

    if fqdn == 'all':
        return MONGOCLI.get_dom0(None, list_all=True)["result"]

    resp = MONGOCLI.get_dom0(fqdn)
    if resp["status"] != 200:
        return [], resp["status"]
    return [resp["result"]]

import logging
import socket

from admins.yandex_salt_components.modules.invapi import \
    raw_request as _raw_request  # hide function from salt, not to call when loading grains

log = logging.getLogger(__name__)


def get_host_info():
    """
    Returns RackTables  tags for the host's FQDN
    """
    fqdn = socket.getfqdn()
    fields = [
        "name",
        "fqdn",
        "comment",
        "etags{tag}",
        "itags{tag}",
        "atags{tag}",
        "asset_no",
        "breed",
        "primary_iface",
        "allocs {type osif ip}",
    ]
    hosts_info = _raw_request("search", fqdn, fields)
    rt = {
        "etags": [],
        "itags": [],
        "atags": [],
        "tags": [],
        "comment": "",
        "asset_no": "",
        "breed": "",
        "primary_iface": "",
        "allocs": []
    }
    for object in hosts_info:
        if object["fqdn"] == fqdn:
            rt = object
            rt["etags"] = [etag["tag"] for etag in object.get("etags", [])]
            rt["itags"] = [itag["tag"] for itag in object.get("itags", [])]
            rt["atags"] = [atag["tag"] for atag in object.get("atags", [])]
            rt["tags"] = rt["etags"] + rt["itags"] + rt["atags"]
            break
    return {"RT": rt}

import logging

import yt.wrapper as yt

from mstand_structs.squeeze_versions import SqueezeVersions

MSTAND_SQUEEZE_VERSIONS_ATTRIBUTE = "_mstand_squeeze_versions"
MSTAND_SQUEEZE_FROM_CACHE_ATTRIBUTE = "_mstand_squeeze_from_cache"


def get_versions(table, client):
    """
    :type table: str
    :type client: yt.YtClient
    :rtype: SqueezeVersions
    """
    serialized = yt.get_attribute(table, MSTAND_SQUEEZE_VERSIONS_ATTRIBUTE, {}, client=client)
    return SqueezeVersions.deserialize(serialized)


def get_all_versions(tables, client):
    """
    :type tables: list[str]
    :type client: yt.YtClient
    :rtype: dict[str, SqueezeVersions]
    """
    return {table: get_versions(table, client) for table in tables}


def set_versions(table, versions, client, from_cache=False):
    """
    :type table: str
    :type versions: SqueezeVersions
    :type client: yt.YtClient
    :type from_cache: bool
    """
    serialized = versions.serialize()
    logging.info("Will set versions %s for %s (from_cache: %r)", serialized, table, from_cache)
    yt.set_attribute(table, MSTAND_SQUEEZE_VERSIONS_ATTRIBUTE, serialized, client=client)
    yt.set_attribute(table, MSTAND_SQUEEZE_FROM_CACHE_ATTRIBUTE, from_cache, client=client)

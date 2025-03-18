import session_local.mr_attributes_local as attributes_local
from mstand_structs.squeeze_versions import SqueezeVersions
from session_yt.versions_yt import MSTAND_SQUEEZE_VERSIONS_ATTRIBUTE


def get_versions(path):
    """
    :type path: str
    :rtype: SqueezeVersions
    """
    serialized = attributes_local.get_attribute(path, MSTAND_SQUEEZE_VERSIONS_ATTRIBUTE, {})
    return SqueezeVersions.deserialize(serialized)


def get_all_versions(paths):
    """
    :type paths: list[str]
    :rtype: dict[str, SqueezeVersions]
    """
    return {path: get_versions(path) for path in paths}


def set_versions(path, versions):
    """
    :type path: str
    :type versions: SqueezeVersions
    """
    serialized = versions.serialize()
    attributes_local.set_attribute(path, MSTAND_SQUEEZE_VERSIONS_ATTRIBUTE, serialized)

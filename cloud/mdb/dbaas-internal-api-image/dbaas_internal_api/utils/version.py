"""
Module that provides interfaces for dealing with versions.
"""
import gzip
import io
import re
from abc import ABC, abstractmethod
from collections import defaultdict
from functools import total_ordering
from typing import Any, Dict, Iterator, List, Optional, Sequence, Tuple

import requests
from dbaas_common import tracing
from flask import current_app

from ..core.exceptions import DbaasHttpError, PreconditionFailedError
from . import config
from .constants import EDITION_DEFAULT
from .feature_flags import has_feature_flag
from .register import get_cluster_traits


@total_ordering
class Version:
    """
    Class representing DBMS version.
    """

    def __init__(self, major: int, minor: int = None, edition: str = EDITION_DEFAULT, string: str = None) -> None:
        self.major = major
        self.minor = minor
        self.edition = edition
        self._string = string

    @property
    def string(self):
        return self._string or self.to_string()

    def to_string(self, sep: str = '.', suffix_sep: str = '-', with_suffix: bool = True) -> str:
        """
        String representation of Version
        """
        elems = [str(self.major)]
        if self.minor is not None:
            elems.append(str(self.minor))
        version = sep.join(elems)
        if self.is_default_edition() or not with_suffix:
            return version
        return suffix_sep.join((version, str(self.edition)))

    def to_num(self) -> str:
        """
        Converts version from '9.6' to '906', '10' to '1000'
        """
        minor = self.minor or 0
        return str(100 * self.major + minor)

    def _is_valid_version(self, instance: Any) -> None:
        if not isinstance(instance, Version):
            raise TypeError('Unable to compare {0!r} with {1!r}'.format(self, instance))

    def __str__(self):
        return self.to_string(sep='.')

    def __repr__(self):
        return '<Version {0}>'.format(self.to_string(sep='.'))

    def __hash__(self):
        return hash((self.major, self.minor, self.edition))

    def __eq__(self, other) -> bool:
        self._is_valid_version(other)
        return bool(self.major == other.major and self.minor == other.minor and self.edition == other.edition)

    def __lt__(self, other) -> bool:
        self._is_valid_version(other)
        # Migration between the versions with different suffixes is not supported,
        # so we can't compare them correctly.
        if self.edition != other.edition:
            raise ValueError('Unable to compare {0!r} with {1!r}'.format(self, other))
        minor = self.minor or 0
        other_minor = other.minor or 0
        if self.major == other.major:
            return minor < other_minor
        return self.major < other.major

    def __le__(self, other) -> bool:
        # Actually Python doesn't require it,
        # but MYPY complains
        #   Unsupported left operand type for >= ("Version")
        return self.__lt__(other) or self.__eq__(other)

    @staticmethod
    def load(string: str, sep: str = '.', suffix_sep: str = '-', strict=True) -> "Version":
        """
        Deserialize Version object from string representation (e.g. '2.6' or '2_0').
        Version format '{major}.{minor}-{edition}', minor and edition are optional
        Examples: 9.6, 10, 10-1c
        """
        items = string.split(suffix_sep, maxsplit=1)
        string = items.pop(0)
        edition = str(items.pop(0)) if items else EDITION_DEFAULT
        items = string.split(sep, 1 if strict else -1)
        major = int(items.pop(0))
        minor = int(items.pop(0)) if items else None
        return Version(major, minor, edition=edition, string=string)

    def is_default_edition(self):
        return self.edition == EDITION_DEFAULT


def get_valid_versions(cluster_type: str) -> Sequence[Version]:
    """
    Return valid versions for the specified cluster type.
    """
    return [version for version, _ in iter_valid_versions(cluster_type)]


def get_available_versions(cluster_type: str) -> Sequence[dict]:
    """
    Return valid versions with name for the specified cluster type.
    """
    return [
        {
            'id': version,
            'name': obj['name'] if 'name' in obj else str(version),
            'deprecated': obj.get('deprecated') is True,
            'versioned_config_key': obj.get('versioned_config_key'),
        }
        if 'versioned_config_key' in obj
        else {
            'id': version,
            'name': obj['name'] if 'name' in obj else str(version),
            'deprecated': obj.get('deprecated') is True,
        }
        for version, obj in iter_valid_versions(cluster_type)
    ]


def fill_updatable_to(available_versions: List[dict], upgrade_paths: Dict[str, dict]):
    """
    Fill updatable_to field in available_versions based on upgrade_paths.
    """
    valid_versions = {ver['id'] for ver in available_versions}
    updatable_to: Dict[Version, list] = defaultdict(list)
    for to_version_str, upgrade_info in upgrade_paths.items():
        to_version = Version.load(to_version_str)
        if to_version not in valid_versions:
            continue

        feature_flag = upgrade_info.get('feature_flag')
        if feature_flag and not has_feature_flag(feature_flag):
            continue

        for from_version_str in upgrade_info['from']:
            updatable_to[Version.load(from_version_str)].append(to_version)

    for version in available_versions:
        version['updatable_to'] = updatable_to[version['id']]


def fill_updatable_to_metadb(
    available_versions: List[dict], upgrade_paths: Dict[str, dict], metadb_default_versions: dict
):
    """
    Fill updatable_to field in available_versions based on upgrade_paths.
    """
    valid_versions = {ver['id'] for ver in available_versions}
    updatable_to: Dict[Version, list] = defaultdict(list)
    for to_version_str, upgrade_info in upgrade_paths.items():
        to_version = Version.load(to_version_str)
        if to_version not in valid_versions:
            continue

        feature_flag = upgrade_info.get('feature_flag')
        if feature_flag and not has_feature_flag(feature_flag):
            continue

        for from_version_str in upgrade_info['from']:
            updatable_to[Version.load(from_version_str)].append(to_version)

    for version in available_versions:
        version['updatable_to'] = updatable_to[version['id']]
        version['deprecated'] = metadb_default_versions[version['id']].get('is_deprecated')


def get_full_version(cluster_type: str, version: Version) -> str:
    """
    Return full version string.
    """
    version_str = version.string
    for obj in _get_versions(cluster_type):
        if obj['version'].startswith(version_str):
            return obj['version']

    raise RuntimeError(f'Version "{version_str}" not found')


def _get_versions(cluster_type: str) -> Sequence[dict]:
    traits_versions = getattr(get_cluster_traits(cluster_type), 'versions', [])
    config_versions = current_app.config['VERSIONS'].get(cluster_type, [])

    return traits_versions or config_versions


def get_default_version(cluster_type: str) -> Version:
    """
    Return default version or maximal for given cluster type.
    """
    for version, obj in iter_valid_versions(cluster_type):
        if obj.get('default') is True:
            return version

    return max(filter(lambda v: v.is_default_edition(), get_valid_versions(cluster_type)))


def is_version_deprecated(cluster_type: str, target_version: Version) -> Optional[bool]:
    """
    Check if version is deprecated.
    """
    for version, obj in iter_valid_versions(cluster_type):
        if version == target_version:
            return obj.get('deprecated', False)
    return None


def ensure_version_allowed(cluster_type: str, target_version: Version) -> None:
    """
    Ensure that version is not deprecated.
    """
    for version, obj in iter_valid_versions(cluster_type):
        if version == target_version:
            if obj.get('deprecated') is True:
                if feature_flag := obj.get('allow_deprecated_feature_flag'):
                    if has_feature_flag(feature_flag):
                        # That 'user' has allow_deprecated_feature_flag
                        return
                raise PreconditionFailedError(f"Can't create cluster, version '{target_version}' is deprecated")
            return


def ensure_version_allowed_metadb(cluster_type: str, target_version: Version, metadb_default_versions: dict) -> None:
    """
    Ensure that version is not deprecated using metadb default_versions
    """
    for version, obj in iter_valid_versions(cluster_type):
        if version == target_version:
            if metadb_default_versions[target_version].get('is_deprecated') is True:
                if feature_flag := obj.get('allow_deprecated_feature_flag'):
                    if has_feature_flag(feature_flag):
                        # That 'user' has allow_deprecated_feature_flag
                        return
                raise PreconditionFailedError(f"Can't create cluster, version '{target_version}' is deprecated")
            return


def ensure_downgrade_allowed(cluster_type: str, src_version: Version, dst_version: Version) -> None:
    """
    Ensure that source version can be downgraded to destination version, or raise otherwise.
    """
    last_downgradable_version = None
    for version, meta in sorted(iter_valid_versions(cluster_type), key=lambda x: x[0], reverse=True):
        if version > src_version:
            continue

        if last_downgradable_version:
            raise PreconditionFailedError(
                f'Downgrade from version \'{src_version}\' to \'{dst_version}\' is not allowed'
            )

        if version == dst_version:
            return

        if meta.get('downgradable') is False:
            last_downgradable_version = version


def iter_valid_versions(cluster_type) -> Iterator[Tuple[Version, dict]]:
    """
    Return iterator to versions with metadata.
    """
    for obj in _get_versions(cluster_type):
        feature_flag = obj.get('feature_flag')
        if feature_flag and not has_feature_flag(feature_flag):
            continue
        yield Version.load(obj['version'], strict=False), obj


def format_versioned_key(prefix: str, version: Version) -> str:
    """
    Format versioned key.
    """
    return '{0}_{1}'.format(prefix, version.to_string('_'))


class AbstractVersionValidator(ABC):
    """
    Abstract Version validator.
    """

    def __init__(self, configuration: dict):
        self.configuration = configuration

    @abstractmethod
    def ensure_version_exists(self, package_name: str, version: Version) -> None:
        """
        Ensures specified version exists in repo.
        """


class VersionValidator(AbstractVersionValidator):
    """
    Version validator.
    """

    def __init__(self, configuration: dict):
        super().__init__(configuration)
        self._packages_uri = configuration['packages_uri']

    @tracing.trace('VersionValidator')
    def ensure_version_exists(self, package_name: str, version: Version) -> None:
        """
        Ensures specified version exists in mdb-bionic-secure repo.
        """
        try:
            resp = requests.get(self._packages_uri)
            resp.raise_for_status()
            with gzip.GzipFile(fileobj=io.BytesIO(resp.content)) as f:
                while True:
                    item = ''
                    while True:
                        line = f.readline().decode()
                        if line in ['', '\n']:
                            break
                        item = "{}{}".format(item, line)
                    if item == '':
                        raise PreconditionFailedError(
                            f'Can\'t create cluster, version \'{version.string}\' is not found'
                        )

                    if not re.search(f'Package:\\s+{package_name}$', item, re.M):
                        continue

                    match = re.search(r'Version:\s+(?P<version>\S+)$', item, re.M)
                    if match and match.group('version') == version.string:
                        return

        except requests.RequestException:
            raise DbaasHttpError("Failed to check version existence")


def version_validator() -> AbstractVersionValidator:
    """
    Return configured version checker.
    """
    version_validator_config = config.version_validator()
    return version_validator_config.get('validator', VersionValidator)(
        version_validator_config.get('validator_config', {})
    )

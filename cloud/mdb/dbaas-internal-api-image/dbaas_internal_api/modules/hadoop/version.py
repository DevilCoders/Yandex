import collections
import functools
from typing import Dict, List, Optional, Tuple, Iterable

from flask import current_app
import semver

from cloud.mdb.internal.python.compute import images
from ...core.exceptions import DbaasClientError
from ...utils.compute import get_provider


def get_public_images() -> Iterable[images.ImageModel]:
    compute_service = get_provider()
    return compute_service.list_images(folder_id=current_app.config['PUBLIC_IMAGES_FOLDER'])  # type: ignore


def is_image_dataproc(image: images.ImageModel):
    is_correct_family = image.family and (
        image.family.startswith('yandex-dataproc') or image.family.startswith('dataproc-image')
    )
    # temporary logic for old 1.0 versions without family
    is_dataproc_name = image.name and image.name.startswith('dataproc-image')
    return is_correct_family or is_dataproc_name


def get_dataproc_default_version_prefix() -> str:
    return str(current_app.config['HADOOP_DEFAULT_VERSION_PREFIX'])


def get_dataproc_images_config(with_deprecated=False, images_config: dict = None) -> Dict[semver.VersionInfo, dict]:
    """Get dataproc images"""
    result = {}
    images_config = images_config or current_app.config['HADOOP_IMAGES']
    for version_string, image_config in images_config.items():
        version = semver.VersionInfo.parse(version_string)
        if image_config.get('deprecated') and not with_deprecated:
            continue
        result[version] = image_config
    return result


def compare_version_strings(first: str, second: str) -> int:
    first_parts = tuple(map(int, first.split('.')))
    second_parts = tuple(map(int, second.split('.')))
    if len(first_parts) > len(second_parts):
        for index, part in enumerate(second_parts):
            if part < first_parts[index]:
                return 1
            elif part > first_parts[index]:
                return -1
        return -1
    elif len(first_parts) < len(second_parts):
        for index, part in enumerate(first_parts):
            if part < second_parts[index]:
                return -1
            elif part > second_parts[index]:
                return 1
        return 1
    else:
        for index, part in enumerate(second_parts):
            if part < first_parts[index]:
                return 1
            elif part > first_parts[index]:
                return -1
        return 0


def get_dataproc_config_with_minor_versions(
    dataproc_images_config: Dict[semver.VersionInfo, dict],
    old_compatibility_versions: List[str],
    compute_images: Iterable[images.ImageModel] = None,
) -> Dict[str, dict]:
    """Get dataproc image configs with minor versions from labels"""

    if not compute_images:
        compute_images = get_public_images()

    dataproc_config_with_minor_versions: Dict[str, dict] = {}
    for image in compute_images:  # type: ignore
        if 'version' in image.labels and is_image_dataproc(image):
            try:
                version = semver.VersionInfo.parse(image.labels['version'])
                image_config = get_dataproc_image_config_by_version(version)
                if image_config:
                    dataproc_config_with_minor_versions[str(version)] = image_config
            except ValueError:
                pass  # skip non semver-valid versions for images

    if old_compatibility_versions:
        # use versions.hadoop_cluster from config to ensure compatibility
        for old_compatibility_version in old_compatibility_versions:
            parsed_version = semver.VersionInfo.parse(f'{old_compatibility_version}.0')
            dataproc_config_with_minor_versions[str(old_compatibility_version)] = dataproc_images_config[parsed_version]

    #  ["1.3.1", "1.4.1", "2.0.1", "2.0.11", "99.0.0", "2.0", "1.4", "1.3"]
    #             -> ['1.3.1', '1.3', '1.4.1', '1.4', '2.0.1', '2.0.11', '2.0', '99.0.0']
    sorted_dict = collections.OrderedDict()  # versions order for ui form
    for key in sorted(dataproc_config_with_minor_versions, key=functools.cmp_to_key(compare_version_strings)):
        sorted_dict[key] = dataproc_config_with_minor_versions[key]
    return sorted_dict


def get_dataproc_image_config_by_version(
    version: semver.VersionInfo, dataproc_images_config: dict = None
) -> Optional[Dict]:
    """Get dataproc image from version id"""

    # possible to work with cached image list on seuential calls
    if not dataproc_images_config:
        dataproc_images_config = get_dataproc_images_config(with_deprecated=True)

    if version in dataproc_images_config:
        return dataproc_images_config[version]

    # if there is no config for specified version, take config from highest version bellow specified
    # example:
    # return config for 2.0.0 when 2.0.5 is specified and there is no separate config for version higher than 2.0.0
    max_possible_version = None
    for config_version in dataproc_images_config:
        if config_version <= version and (not max_possible_version or max_possible_version < config_version):
            max_possible_version = config_version

    return dataproc_images_config.get(max_possible_version)  # type: ignore


def get_version_parts(version_prefix: str = '') -> Tuple[Optional[int], Optional[int], Optional[int]]:
    if not version_prefix:
        return None, None, None
    parts = version_prefix.split('.')
    if len(parts) == 1:
        return int(parts[0]), None, None
    if len(parts) == 2:
        return int(parts[0]), int(parts[1]), None
    return int(parts[0]), int(parts[1]), int(parts[2])


def get_latest_dataproc_version_by_prefix(
    version_prefix: str = '',
    compute_images: Iterable[images.ImageModel] = None,
    dataproc_images_config: Dict[semver.VersionInfo, dict] = None,
) -> Tuple[semver.VersionInfo, str, dict]:
    """Get latest dataproc image from version prefix"""

    major, minor, patch = get_version_parts(version_prefix)
    image_version = None
    image_id = None
    image_config = None
    if not dataproc_images_config:
        dataproc_images_config = get_dataproc_images_config(with_deprecated=True)
    if not compute_images:
        compute_images = get_public_images()

    for image in compute_images:  # type: ignore
        if 'version' in image.labels and is_image_dataproc(image):
            try:
                version = semver.VersionInfo.parse(image.labels['version'])
            except ValueError:
                continue
            if major is not None and major != version.major:
                continue
            if minor is not None and minor != version.minor:
                continue
            if patch is not None and patch != version.patch:
                continue
            if not image_version or version > image_version:
                image_config = get_dataproc_image_config_by_version(version, dataproc_images_config)
                if image_config:  # ignore image versions in public folder without config
                    image_version = version
                    image_id = image.id
    if not image_version:
        raise DbaasClientError(f'Can not find DataProc image with version prefix {version_prefix}')
    return image_version, image_id, image_config  # type: ignore

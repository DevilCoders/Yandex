import json
import os

from tempfile import NamedTemporaryFile

from yc_common.clients.models.networks import NetworkAttachmentInfo
from yc_common.exceptions import Error
from yc_common.models import ModelValidationError


REMOVED_SUFFIX = ".removed"
TMP_SUFFIX = ".tmp"


class NetworkAttachmentInfoDirNotExists(Error):
    def __init__(self, dir_name):
        super().__init__(
            "Network Attachments Info dir {!r} does not exist", dir_name
        )


def get_network_attachment_info_path(network_attachment_info_dir, port_id: str, removed=False):
    path = os.path.join(network_attachment_info_dir, port_id)
    if removed:
        path += REMOVED_SUFFIX

    return path


def save_network_attachment_info(
    network_attachment_info: NetworkAttachmentInfo,
    dir_name: str, port_id: str,
    removed=False
):
    final_path = get_network_attachment_info_path(dir_name, port_id, removed=removed)

    try:
        with NamedTemporaryFile(
            mode="w", dir=dir_name,
            suffix=TMP_SUFFIX, delete=False
        ) as state_file:
            json.dump(network_attachment_info.to_api(public=False), state_file)
            state_file.flush()
            os.fsync(state_file.fileno())
        os.chmod(state_file.name, 0o644)
        os.rename(state_file.name, final_path)
    except Exception as e:
        raise Error("Failed to save network attachment info to {!r}: {}.", final_path, e)

    if removed:
        os.unlink(get_network_attachment_info_path(dir_name, port_id))


def load_network_attachment_info(dir_name: str, port_id: str, removed=False) -> NetworkAttachmentInfo:
    if not os.path.exists(dir_name):
        raise NetworkAttachmentInfoDirNotExists(dir_name)

    network_attachment_info_path = get_network_attachment_info_path(dir_name, port_id, removed=removed)

    try:
        with open(network_attachment_info_path) as state_file:
            network_attachment_info = json.load(state_file)
    except ValueError as e:
        raise Error(
            "Unable to load {!r} network attachment info state ({}): {}.",
            port_id,
            network_attachment_info_path, e
        )
    except OSError as e:
        raise e

    try:
        network_attachment_info = NetworkAttachmentInfo.from_api(network_attachment_info, ignore_unknown=True)
    except ModelValidationError as e:
        raise Error(
            "Unable to load {!r} network attachment info state ({}): {}",
            port_id,
            network_attachment_info_path, e
        )

    return network_attachment_info


def iter_network_attachments(dir_name, removed=False):
    if not os.path.exists(dir_name):
        raise NetworkAttachmentInfoDirNotExists(dir_name)

    for file_name in os.listdir(dir_name):
        if removed and not file_name.endswith(REMOVED_SUFFIX):
            continue

        if not removed and file_name.endswith(REMOVED_SUFFIX):
            continue

        file_path = os.path.join(dir_name, file_name)
        if os.path.isfile(file_path):
            yield file_path

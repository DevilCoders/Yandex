from cloud.blockstore.pylibs.clusters import get_cluster_test_config as get_cluster
from cloud.blockstore.pylibs.ycp import make_ycp_engine as make_ycp
from cloud.blockstore.pylibs.ycp import YcpWrapper as ycp_wrapper
from cloud.blockstore.pylibs.ycp import Ycp as ycp_base

import datetime
import logging
import re


def _get_ycp_wrapper(
    cluster_name: str,
    zone_id: str,
    ipc_type: str,
    logger: logging.Logger
) -> ycp_wrapper:
    cluster = get_cluster(cluster_name, zone_id)
    return ycp_wrapper(
        cluster.name,
        cluster.ipc_type_to_folder_desc[ipc_type],
        logger,
        make_ycp(False),
    )


def _log_entity_info(
    logger: logging.Logger,
    entity_type: str,
    entity_id: str,
    entity_name: str,
    entity_created_at: datetime.datetime,
) -> None:
    logger.info(f'===Found {entity_type}===')
    logger.info(f'Id: {entity_id}')
    logger.info(f'Name: {entity_name}')
    logger.info(f'Created at: {entity_created_at}')


def _log_removal_not_performed(
    logger: logging.Logger,
    entity_type: str,
) -> None:
    logger.info(f'{entity_type} is still fresh: removal is not performed')


def _log_removal_performed(
    logger: logging.Logger,
    entity_type: str,
) -> None:
    logger.info(f'{entity_type} is stale: removal was performed')


def _error_can_be_skipped(
    error: str,
) -> bool:
    if 'not found' in error:
        return True

    return False


def delete_stale_instances(
    profile: str,
    zone: str,
    ipc_type: str,
    regex: str,
    ttl: int,
    logger: logging.Logger,
) -> None:
    ycp = _get_ycp_wrapper(profile, zone, ipc_type, logger)
    for instance in ycp.list_instances():
        if not re.match(regex, instance.name):
            continue
        _log_entity_info(
            logger,
            'instance',
            instance.id,
            instance.name,
            instance.created_at,
        )

        if datetime.datetime.now() - instance.created_at.replace(tzinfo=None) < datetime.timedelta(
            seconds=ttl
        ):
            _log_removal_not_performed(logger, 'Instance')
        else:
            try:
                ycp.delete_instance(instance)
            except ycp_base.Error as e:
                if not _error_can_be_skipped(str(e)):
                    raise e
            _log_removal_performed(logger, 'Instance')


def delete_stale_disks(
    profile: str,
    zone: str,
    ipc_type: str,
    regex: str,
    ttl: int,
    logger: logging.Logger,
) -> None:
    ycp = _get_ycp_wrapper(profile, zone, ipc_type, logger)
    for disk in ycp.list_disks():
        if not re.match(regex, disk.name):
            continue
        _log_entity_info(
            logger,
            'disk',
            disk.id,
            disk.name,
            disk.created_at,
        )

        if len(disk.instance_ids) > 0:
            logger.info(f'Disk currently mounted to instances {disk.instance_ids}: removal is not performed')
            continue

        if datetime.datetime.now() - disk.created_at.replace(tzinfo=None) < datetime.timedelta(
            seconds=ttl
        ):
            _log_removal_not_performed(logger, 'Disk')
        else:
            try:
                ycp.delete_disk(disk)
            except ycp_base.Error as e:
                if not _error_can_be_skipped(str(e)):
                    raise e
            _log_removal_performed(logger, 'Disk')


def delete_stale_images(
    profile: str,
    zone: str,
    ipc_type: str,
    regex: str,
    ttl: int,
    logger: logging.Logger,
) -> None:
    ycp = _get_ycp_wrapper(profile, zone, ipc_type, logger)
    for image in ycp.list_images():
        if not re.match(regex, image.name):
            continue
        _log_entity_info(
            logger,
            'image',
            image.id,
            image.name,
            image.created_at,
        )

        if datetime.datetime.now() - image.created_at.replace(tzinfo=None) < datetime.timedelta(
            seconds=ttl
        ):
            _log_removal_not_performed(logger, 'Image')
        else:
            try:
                ycp.delete_image(image)
            except ycp_base.Error as e:
                if not _error_can_be_skipped(str(e)):
                    raise e
            _log_removal_performed(logger, 'Image')


def delete_stale_snapshots(
    profile: str,
    zone: str,
    ipc_type: str,
    regex: str,
    ttl: int,
    logger: logging.Logger,
) -> None:
    ycp = _get_ycp_wrapper(profile, zone, ipc_type, logger)
    for snapshot in ycp.list_snapshots():
        if not re.match(regex, snapshot.name):
            continue
        _log_entity_info(
            logger,
            'snapshot',
            snapshot.id,
            snapshot.name,
            snapshot.created_at,
        )

        if datetime.datetime.now() - snapshot.created_at.replace(tzinfo=None) < datetime.timedelta(
            seconds=ttl
        ):
            _log_removal_not_performed(logger, 'Snapshot')
        else:
            try:
                ycp.delete_snapshot(snapshot)
            except ycp_base.Error as e:
                if not _error_can_be_skipped(str(e)):
                    raise e
            _log_removal_performed(logger, 'Snapshot')


def delete_stale_filesystems(
    profile: str,
    zone: str,
    ipc_type: str,
    regex: str,
    ttl: int,
    logger: logging.Logger,
) -> None:
    ycp = _get_ycp_wrapper(profile, zone, ipc_type, logger)
    for filesystem in ycp.list_filesystems():
        if not re.match(regex, filesystem.name):
            continue

        _log_entity_info(
            logger,
            'filesystem',
            filesystem.id,
            filesystem.name,
            filesystem.created_at,
        )

        if len(filesystem.instance_ids) > 0:
            logger.info(f'Filesystem currently mounted to instances {filesystem.instance_ids}: removal is not performed')
            continue

        if datetime.datetime.now() - filesystem.created_at.replace(tzinfo=None) < datetime.timedelta(
            seconds=ttl
        ):
            _log_removal_not_performed(logger, 'Filesystem')
        else:
            try:
                ycp.delete_fs(filesystem)
            except ycp_base.Error as e:
                if not _error_can_be_skipped(str(e)):
                    raise e
            _log_removal_performed(logger, 'Filesystem')

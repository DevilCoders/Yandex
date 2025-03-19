"""Instance groups logic (CLOUD-33565)"""

import sys
from typing import List, Optional

from bootstrap.common.rdbms.db import Db
from bootstrap.common.rdbms.exceptions import RecordNotFoundError, RecordAlreadyInDbError

from .types import InstanceGroup, InstanceGroupRelease
from .utils import decorate_exported


__all__ = [
    "get_stand_instance_groups",

    "get_instance_group",
    "add_instance_group",
    "delete_instance_group",

    "get_instance_group_release",
    "set_instance_group_release",
]


def _get_verified_instance_group(stand_name: str, instance_group_name: str, *, db: Db, check_exists: bool) -> \
        Optional[InstanceGroup]:
    """Verify that instance group exists in stand (or not exists)"""
    # check if stand exists
    if not db.record_exists("stands", stand_name, column_name="name"):
        raise RecordNotFoundError("Stand <{}> is not found in database".format(stand_name))

    # check if instance group in stand exists
    query = """
        SELECT instance_groups.id, instance_groups.name, stands.name FROM instance_groups
        INNER JOIN stands ON
            instance_groups.stand_id = stands.id
        WHERE
            instance_groups.name = %s AND
            stands.name = %s;
    """
    instance_groups = db.select_by_condition(
        query, (instance_group_name, stand_name),
    )
    if check_exists:
        if not len(instance_groups):
            raise RecordNotFoundError("Instance Group <{}> is not registered in stand <{}>".format(
                instance_group_name, stand_name
            ))
        else:
            return InstanceGroup(*instance_groups[0])
    else:  # check if instance group does NOT exists
        if len(instance_groups):
            raise RecordAlreadyInDbError("Instance Group <{}> already registered in stand <{}>".format(
                instance_group_name, stand_name
            ))

    return None


def get_instance_group(stand_name: str, instance_group_name: str, *, db: Db) -> InstanceGroup:
    instance_group = _get_verified_instance_group(stand_name, instance_group_name, db=db, check_exists=True)

    return instance_group


def get_stand_instance_groups(stand_name: str, *, db: Db) -> List[InstanceGroup]:
    if not db.record_exists("stands", stand_name, column_name="name"):
        raise RecordNotFoundError("Stand <{}> is not found in database".format(stand_name))

    query = """
        SELECT instance_groups.id, instance_groups.name, stands.name FROM stands
        INNER JOIN instance_groups ON
            stands.id = instance_groups.stand_id
        WHERE
            stands.name = %s
        ORDER BY
            instance_groups.id;
    """

    instance_groups = [InstanceGroup(*record) for record in db.select_by_condition(query, (stand_name,))]

    return instance_groups


def add_instance_group(stand_name: str, instance_group_name: str, *, db: Db) -> InstanceGroup:
    _get_verified_instance_group(stand_name, instance_group_name, db=db, check_exists=False)

    # add instance group to db
    instance_group = InstanceGroup(None, instance_group_name, stand_name)
    db.insert_one("instance_groups", instance_group)

    # add empty release to db
    instance_group_release = InstanceGroupRelease(None, None, None, stand_name, instance_group_name, instance_group.id)
    db.insert_one("instance_group_releases", instance_group_release)

    db.conn.commit()

    return instance_group


def delete_instance_group(stand_name: str, instance_group_name: str, *, db: Db) -> None:
    instance_group = get_instance_group(stand_name, instance_group_name, db=db)

    db.delete_one("instance_groups", instance_group)

    db.conn.commit()


def get_instance_group_release(stand_name: str, instance_group_name: str, *, db: Db) -> InstanceGroupRelease:
    _get_verified_instance_group(stand_name, instance_group_name, db=db, check_exists=True)

    query = """
        SELECT instance_group_releases.id, instance_group_releases.url, instance_group_releases.image_id,
               stands.name, instance_groups.name, instance_groups.id FROM instance_group_releases
        INNER JOIN instance_groups ON
            instance_group_releases.instance_group_id = instance_groups.id
        INNER JOIN stands ON
            instance_groups.stand_id = stands.id
        WHERE
            instance_groups.name = %s AND
            stands.name = %s;
    """

    return InstanceGroupRelease(
        *db.select_by_condition(query, (instance_group_name, stand_name), ensure_one_result=True)
    )


def set_instance_group_release(stand_name: str, instance_group_name: str, url: str, image_id: Optional[str], *,
                               db: Db) -> InstanceGroupRelease:
    instance_group_release = get_instance_group_release(stand_name, instance_group_name)

    instance_group_release.url = url
    instance_group_release.image_id = image_id

    db.update_one("instance_group_releases", ["url", "image_id"], instance_group_release)

    db.conn.commit()

    return instance_group_release


decorate_exported(sys.modules[__name__])

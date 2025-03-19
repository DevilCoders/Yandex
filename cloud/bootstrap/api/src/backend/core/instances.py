"""Hosts logic (almost similar to svms logic)"""

import sys
from typing import Dict, List, Optional, Tuple, Union, Iterable

from bootstrap.common.rdbms.db import Db
from bootstrap.common.rdbms.exceptions import RecordNotFoundError, RecordAlreadyInDbError
from bootstrap.common.exceptions import BootstrapError

from .salt_roles import _get_salt_role
from .types import InstanceType, EInstanceTypes, BaseInstance, Host, ServiceVm, SaltRole, InstanceSaltRolePackage, T
from .utils import decorate_exported, ensure_record_exists

__all__ = [
    # CLOUD-35467: get host configs version
    "get_host_configs_version",

    "get_all_instances",

    # CLOUD-35467: get all hosts and svms with version
    "get_stand_hosts_and_svms_with_version",

    # get filtered
    "get_stand_instances",

    # single instance manipulation
    "get_instance",
    "add_instance",
    "update_instance",
    "delete_instance",

    # helper functions for batch manipulation
    "upsert_instances",
    "delete_instances",

    # manipulations with salt roles
    "get_instance_salt_roles",
    "get_instance_salt_role",
    "add_instance_salt_role",
    "delete_instance_salt_role",
    "upsert_instance_salt_roles",

    # manipulation with salt role packages
    "get_instance_salt_role_packages",
    "upsert_instance_salt_role_packages",
]


def _bump_host_configs_version(*, db: Db) -> None:
    """CLOUD-35467: bump version of host configs after some configs being changed"""
    cursor = db.conn.cursor()
    cursor.execute("UPDATE  host_configs_info SET version = version + 1;")


def get_host_configs_version(*, db: Db, for_update: bool = False) -> int:
    for_update_postfix = ""
    if for_update:
        for_update_postfix = "FOR UPDATE"

    query = "SELECT version FROM host_configs_info {for_update_postfix};".format(for_update_postfix=for_update_postfix)

    return db.select_by_condition(query, (), one_column=True, ensure_one_result=True)


def _get_existing_unexisting(fqdns: List[str], *, instance_type: InstanceType, db: Db) -> Tuple[List[str], List[str]]:
    query = """
        SELECT instances.fqdn FROM instances
        INNER JOIN {props_table} ON
            instances.id = {props_table}.id
        WHERE
            instances.fqdn IN %s;
    """.format(props_table=instance_type.props_table)

    existing = db.select_by_condition(query, (tuple(fqdns),), one_column=True)

    unexisting = [fqdn for fqdn in fqdns if fqdn not in existing]

    return existing, unexisting


def get_all_instances(*, instance_type: InstanceType, db: Db) -> List[Union[Host, ServiceVm]]:
    query = """
        SELECT instances.id, instances.fqdn, instances.type, stands.name, {props_table}.dynamic_config FROM instances
        LEFT JOIN stands ON
            instances.stand_id = stands.id
        INNER JOIN {props_table} ON
            instances.id = {props_table}.id
        ORDER BY
            instances.id;
    """.format(props_table=instance_type.props_table)

    instances = [instance_type.type(*record) for record in db.select_by_condition(query, ())]

    return instances


def get_stand_instances(stand_name: str, *, instance_type: InstanceType, db: Db, for_update: bool = False) \
        -> List[Union[Host, ServiceVm]]:
    # check if stand exists
    ensure_record_exists(db, "stands", stand_name, column_name="name")

    for_update_postfix = ""
    if for_update:
        for_update_postfix = "FOR UPDATE"

    query = """
        SELECT instances.id, instances.fqdn, instances.type, stands.name, {props_table}.dynamic_config FROM instances
        LEFT JOIN stands ON
            instances.stand_id = stands.id
        INNER JOIN {props_table} ON
            instances.id = {props_table}.id
        WHERE
            stands.name = %s
        ORDER BY
            instances.id
        {for_update_postfix};
    """.format(for_update_postfix=for_update_postfix, props_table=instance_type.props_table)

    instances = [instance_type.type(*record) for record in db.select_by_condition(query, (stand_name,))]

    return instances


def get_stand_hosts_and_svms_with_version(stand_name: str, *, db: Db) -> Tuple[List[Host], List[ServiceVm], int]:
    # check if stand exists
    ensure_record_exists(db, "stands", stand_name, column_name="name")

    # FIXME: do not use interface functions, which automaticall commit/rollback on finish
    hosts = get_stand_instances(stand_name, instance_type=EInstanceTypes.HOST.value, db=db, for_update=True)
    svms = get_stand_instances(stand_name, instance_type=EInstanceTypes.SVM.value, db=db, for_update=True)
    host_configs_version = get_host_configs_version(db=db, for_update=True)

    return hosts, svms, host_configs_version


def _get_base_instance(fqdn: str, *, db: Db) -> BaseInstance:
    """Get common part of <Host> and <ServiceVm> entity"""
    ensure_record_exists(db, "instances", fqdn, column_name="fqdn")

    return BaseInstance(*db.select_one("instances", fqdn, column_name="fqdn"))


def _get_instance(fqdn: str, *, instance_type: InstanceType, db: Db) -> Union[Host, ServiceVm]:
    return get_instances_as_type([fqdn], klass=instance_type.type, db=db, props_table=instance_type.props_table)[0]


def get_instance(fqdn: str, *, instance_type: InstanceType, db: Db):
    return _get_instance(fqdn, instance_type=instance_type, db=db)


def _add_instance(fqdn: str, stand: Optional[str], dynamic_config: Optional[Dict], *, instance_type: InstanceType,
                  db: Db, check_exists: bool = True) -> Union[Host, ServiceVm]:
    if check_exists and db.record_exists("instances", fqdn, column_name="fqdn"):
        raise RecordAlreadyInDbError("{} <{}> is already in database".format(instance_type.ui_name, fqdn))

    instance = instance_type.type(None, fqdn, instance_type.name, stand, dynamic_config)

    db.insert_one("instances", instance)
    db.insert_one(instance_type.props_table, instance)

    return instance


def add_instance(fqdn: str, stand: Optional[str], dynamic_config: Optional[Dict], *, instance_type: InstanceType,
                 db: Db) -> Union[Host, ServiceVm]:
    # insert to instances
    instance = _add_instance(fqdn, stand, dynamic_config, instance_type=instance_type, db=db)

    _bump_host_configs_version(db=db)

    db.conn.commit()

    return instance


def _update_instances(data_by_fqdn: Dict[str, Dict], instance_type: InstanceType, db: Db) -> List[Union[Host, ServiceVm]]:
    updates = {
        "instances": [],
        instance_type.props_table: [],
    }

    instances = get_instances_as_type(data_by_fqdn.keys(), klass=instance_type.type, db=db,
                                      props_table=instance_type.props_table)
    for instance in instances:
        data = data_by_fqdn[instance.fqdn]
        if "stand" in data:
            instance.stand = data["stand"]
            updates["instances"].append(instance)
        if "dynamic_config" in data:
            instance.dynamic_config = data["dynamic_config"]
            updates[instance_type.props_table].append(instance)

    db.update_many("instances", ["stand_id"], updates["instances"])
    db.update_many(instance_type.props_table, ["dynamic_config"], updates[instance_type.props_table])

    return instances


def _update_instance(fqdn: str, data: Dict, *, instance_type: InstanceType, db: Db) -> Union[Host, ServiceVm]:
    instance = _update_instances({fqdn: data}, instance_type, db)[0]

    return instance


def update_instance(fqdn: str, data: Dict, *, instance_type: InstanceType, db: Db) -> Union[Host, ServiceVm]:
    """Update all fields from data"""
    instance = _update_instance(fqdn, data, instance_type=instance_type, db=db)

    _bump_host_configs_version(db=db)

    db.conn.commit()

    return instance


def delete_instance(fqdn: str, *, instance_type: InstanceType, db: Db) -> None:
    instance = _get_instance(fqdn, instance_type=instance_type, db=db)

    db.delete_one("instances", instance)

    _bump_host_configs_version(db=db)

    db.conn.commit()


def upsert_instances(instances_data: List[Dict], *, instance_type: InstanceType, db: Db, ensure_all_new: bool = False,
                     ensure_all_existing: bool = False) -> List[Host]:
    """Batch manipulation function: insert missing instances, update existing instances"""
    existing_instances, unexisting_instances = _get_existing_unexisting(
        [instance_data["fqdn"] for instance_data in instances_data], instance_type=instance_type, db=db
    )

    # some checks
    if ensure_all_new and existing_instances:
        instances_str = ",".join(sorted(list(existing_instances)))
        raise RecordAlreadyInDbError("{} <{}> are already in database".format(instance_type.ui_name_plural, instances_str))
    if ensure_all_existing and unexisting_instances:
        instances_str = ",".join(sorted(unexisting_instances))
        raise RecordNotFoundError("{} <{}> are not found in database".format(instance_type.ui_name_plural, instances_str))

    # insert
    data_by_fqdn = {}
    for instance_data in instances_data:
        # insert unexisting
        fqdn = instance_data["fqdn"]
        data_by_fqdn[fqdn] = instance_data
        if fqdn not in existing_instances:
            _add_instance(fqdn, instance_data["stand"], instance_data["dynamic_config"], instance_type=instance_type,
                          db=db, check_exists=False)

    # update data
    result = _update_instances(data_by_fqdn, instance_type, db)

    _bump_host_configs_version(db=db)

    db.conn.commit()

    return result


def delete_instances(instance_fqdns: List[str], *, instance_type: InstanceType, db: Db) -> None:
    """Delete instances"""
    # check for unexisting instances
    existing_instances, unexisting_instances = _get_existing_unexisting(instance_fqdns, instance_type=instance_type, db=db)
    if unexisting_instances:
        raise RecordNotFoundError(
            "{} <{}> are not found in database".format(instance_type.ui_name_plural, ",".join(unexisting_instances))
        )

    # delete all
    query = """
        DELETE FROM instances USING {props_table}
        WHERE
            instances.fqdn IN %s AND
            instances.id = {props_table}.id;
    """.format(props_table=instance_type.props_table)

    cursor = db.conn.cursor()
    cursor.execute(query, (tuple(instance_fqdns),))

    _bump_host_configs_version(db=db)

    db.conn.commit()


def _add_instance_salt_role(fqdn: str, salt_role: str, *, db: Db) -> SaltRole:
    instance = _get_base_instance(fqdn, db=db)
    salt_role = _get_salt_role(salt_role, db=db)

    if db.record_exists_ext("instance_salt_roles", ("instance_id", "salt_role_id"), (instance.id, salt_role.id)):
        raise RecordAlreadyInDbError("Instance <{}> already has role <{}>".format(instance.fqdn, salt_role.name))

    query = "INSERT INTO instance_salt_roles (instance_id, salt_role_id) VALUES (%s, %s);"

    cursor = db.conn.cursor()
    cursor.execute(query, (instance.id, salt_role.id))

    return salt_role


def add_instance_salt_role(fqdn: str, salt_role: str, *, db: Db) -> SaltRole:
    result = _add_instance_salt_role(fqdn, salt_role, db=db)

    db.conn.commit()

    return result


def _get_instance_salt_roles(fqdn: str, *, db: Db) -> List[SaltRole]:
    ensure_record_exists(db, "instances", fqdn, column_name="fqdn")

    query = """
        SELECT salt_roles.id, salt_roles.name FROM salt_roles
        LEFT JOIN instance_salt_roles ON
            salt_roles.id = instance_salt_roles.salt_role_id
        LEFT JOIN instances ON
            instance_salt_roles.instance_id = instances.id
        WHERE
            instances.fqdn = %s
        ORDER BY
            salt_roles.id;
    """

    return [SaltRole(*args) for args in db.select_by_condition(query, (fqdn,))]


def get_instance_salt_roles(fqdn: str, *, db: Db) -> List[SaltRole]:
    return _get_instance_salt_roles(fqdn, db=db)


def _get_instance_salt_role(fqdn: str, salt_role_name: str, *, db: Db) -> SaltRole:
    for salt_role in _get_instance_salt_roles(fqdn, db=db):
        if salt_role.name == salt_role_name:
            return salt_role
    else:
        raise RecordNotFoundError("Instance <{}> does not have role <{}>".format(fqdn, salt_role_name))


def get_instance_salt_role(fqdn: str, salt_role_name: str, *, db: Db) -> SaltRole:
    return _get_instance_salt_role(fqdn, salt_role_name, db=db)


def _delete_instance_salt_role(fqdn: str, salt_role_name: str, *, db: Db) -> None:
    _get_instance_salt_role(fqdn, salt_role_name, db=db)  # check existance of all entities

    query = """
        DELETE FROM instance_salt_roles USING salt_roles, instances
        WHERE
            instance_salt_roles.instance_id = instances.id AND
            instances.fqdn = %s AND
            instance_salt_roles.salt_role_id = salt_roles.id AND
            salt_roles.name = %s;
    """

    cursor = db.conn.cursor()
    cursor.execute(query, (fqdn, salt_role_name))

    return None


def delete_instance_salt_role(fqdn: str, salt_role_name: str, *, db: Db) -> None:
    _delete_instance_salt_role(fqdn, salt_role_name, db=db)

    db.conn.commit()

    return None


def upsert_instance_salt_roles(fqdn: str, salt_roles_data: List[Dict], *, db: Db) -> List[SaltRole]:
    _get_base_instance(fqdn, db=db)  # just to check if instance exists

    new_salt_roles = {salt_role_data["name"] for salt_role_data in salt_roles_data}
    current_salt_roles = {salt_role.name for salt_role in _get_instance_salt_roles(fqdn, db=db)}

    # delete
    to_delete_salt_roles = sorted(set(current_salt_roles) - set(new_salt_roles))
    for salt_role_name in to_delete_salt_roles:
        _delete_instance_salt_role(fqdn, salt_role_name, db=db)

    # add
    to_add_roles = sorted(set(new_salt_roles) - set(current_salt_roles))
    for salt_role_name in to_add_roles:
        _add_instance_salt_role(fqdn, salt_role_name, db=db)

    # get result after all modifications
    result = _get_instance_salt_roles(fqdn, db=db)

    db.conn.commit()

    return result


def _get_instance_salt_role_packages(fqdn: str, *, db: Db) -> List[InstanceSaltRolePackage]:
    ensure_record_exists(db, "instances", fqdn, column_name="fqdn")

    query = """
        SELECT instance_salt_role_packages.id, salt_roles.name, instance_salt_role_packages.package_name,
               instance_salt_role_packages.target_version FROM instance_salt_role_packages
        LEFT JOIN instance_salt_roles ON
            instance_salt_role_packages.instance_salt_role_id = instance_salt_roles.id
        LEFT JOIN salt_roles ON
            instance_salt_roles.salt_role_id = salt_roles.id
        LEFT JOIN instances ON
            instance_salt_roles.instance_id = instances.id
        WHERE
            instances.fqdn = %s
        ORDER BY
            instance_salt_role_packages.id;
    """

    return [InstanceSaltRolePackage(*args) for args in db.select_by_condition(query, (fqdn,))]


def get_instance_salt_role_packages(fqdn: str, *, db: Db) -> List[InstanceSaltRolePackage]:
    return _get_instance_salt_role_packages(fqdn=fqdn, db=db)


def _get_instance_salt_role_id(fqdn: str, salt_role: str, db: Db) -> int:
    query = """
        SELECT instance_salt_roles.id FROM instance_salt_roles
        LEFT JOIN instances ON
            instance_salt_roles.instance_id = instances.id
        LEFT JOIN salt_roles ON
            instance_salt_roles.salt_role_id = salt_roles.id
        WHERE
            instances.fqdn = %s AND
            salt_roles.name = %s;
    """
    try:
        return db.select_by_condition(query, (fqdn, salt_role), one_column=True, ensure_one_result=True)
    except RecordNotFoundError:
        raise RecordNotFoundError("Instance <{}> does not have role <{}>".format(fqdn, salt_role))


def upsert_instance_salt_role_packages(fqdn: str, packages_info: List, *, db: Db) -> List[InstanceSaltRolePackage]:
    # some validation
    ensure_record_exists(db, "instances", fqdn, column_name="fqdn")
    for salt_role in {package_info["salt_role"] for package_info in packages_info}:
        ensure_record_exists(db, "salt_roles", salt_role, column_name="name")

    cursor = db.conn.cursor()

    # delete old data
    delete_query = """
        DELETE FROM instance_salt_role_packages USING instance_salt_roles, instances
        WHERE
            instance_salt_role_packages.instance_salt_role_id = instance_salt_roles.id AND
            instance_salt_roles.instance_id = instances.id AND
            instances.fqdn = %s;
    """
    cursor.execute(delete_query, (fqdn, ))

    # add new data
    insert_query = """
        INSERT INTO instance_salt_role_packages
            (instance_salt_role_id, package_name, target_version) VALUES (%s, %s, %s);
    """
    for package_info in packages_info:
        instance_salt_role_id = _get_instance_salt_role_id(fqdn, package_info["salt_role"], db)
        cursor.execute(
            insert_query, (instance_salt_role_id, package_info["package_name"], package_info["target_version"])
        )

    result = _get_instance_salt_role_packages(fqdn=fqdn, db=db)

    db.conn.commit()

    return result


def get_instances_as_type(host_fqdns: Iterable[str], klass: T, db: Db, props_table: str = None) \
        -> List[T]:
    supported_classes_with_relations = (Host, ServiceVm)
    if klass not in supported_classes_with_relations and props_table:
        raise BootstrapError("Can only get relations for classes {}".format(supported_classes_with_relations))

    fieldset = "*"
    if props_table:
        fieldset = "instances.id, instances.fqdn, instances.type, stands.name, {props_table}.dynamic_config"\
            .format(props_table=props_table)
    query = """
        SELECT {} FROM instances
    """.format(fieldset)
    if props_table:
        query += """
            LEFT JOIN stands ON
                instances.stand_id = stands.id
            INNER JOIN {props_table} ON
                instances.id = {props_table}.id
        """.format(props_table=props_table)
    query += """
        WHERE
            instances.fqdn IN %s;
    """
    existing_hosts = [
        klass(*record) for record in db.select_by_condition(query, (tuple(host_fqdns), ))
    ]

    missing_hosts = set(host_fqdns) - {host.fqdn for host in existing_hosts}
    if missing_hosts:
        raise RecordNotFoundError("Hosts <{}> do not exist in database".format(",".join(sorted(missing_hosts))))

    return existing_hosts


decorate_exported(sys.modules[__name__])

# -*- coding: utf-8 -*-
"""
DBaaS Internal API PostgreSQL database methods
"""
from . import utils
from ...utils import metadb
from ...utils.metadata import CreateDatabaseMetadata, DeleteDatabaseMetadata, ModifyDatabaseMetadata
from ...utils.operation_creator import create_operation
from ...utils.register import DbaasOperation, Resource, register_request_handler
from .api import convertors
from .constants import DATABASE_OBJ, MY_CLUSTER_TYPE
from .pillar import get_cluster_pillar
from .traits import PostgresqlOperations, PostgresqlTasks
from .types import Database
from .validation import validate_extensions, validate_databases, EXTENSION_DEPENDENCIES

MY_OBJ = DATABASE_OBJ


@register_request_handler(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.CREATE)
def add_postgresql_database(cluster, database_spec, **_):
    """
    Creates postgresql database. Returns task for worker
    """
    cluster_pillar = get_cluster_pillar(cluster)
    cid = cluster['cid']

    db = convertors.database_from_spec(database_spec)
    utils.validate_database_name(db.name)
    utils.validate_user_name(db.owner)
    validate_extensions(
        db.extensions,
        utils.get_cluster_major_version(cid),
        cluster_pillar.config.get_config(),
    )

    cluster_pillar.databases.add_database(db)
    validate_databases(cluster_pillar.databases.get_databases())
    metadb.update_cluster_pillar(cid, cluster_pillar)

    return create_operation(
        task_type=PostgresqlTasks.database_create,
        operation_type=PostgresqlOperations.database_add,
        metadata=CreateDatabaseMetadata(database_name=db.name),
        cid=cid,
        task_args={
            'target-database': db.name,
            'zk_hosts': cluster_pillar.pgsync.get_zk_hosts_as_str(),
        },
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.MODIFY)
def modify_postgresql_database(cluster, database_name, extensions=None, new_database_name=None, **kwargs):
    """
    Modifies postgresql database. Returns task for worker
    """
    cluster_pillar = get_cluster_pillar(cluster)
    cid = cluster['cid']

    db = cluster_pillar.databases.database(database_name)

    new_extensions = {}
    deleted_extensions = {}
    if extensions is not None:
        extensions = utils.parse_extensions(extensions)
        validate_extensions(
            extensions,
            utils.get_cluster_major_version(cid),
            cluster_pillar.config.get_config(),
        )

        new_extensions = set(extensions) - set(db.extensions)
        deleted_extensions = set(db.extensions) - set(extensions)
        db = Database(
            name=db.name,
            owner=db.owner,
            lc_ctype=db.lc_ctype,
            lc_collate=db.lc_collate,
            extensions=extensions,
            template=db.template,
        )
        cluster_pillar.databases.update_database(db)
        validate_databases(cluster_pillar.databases.get_databases())

    if new_database_name:
        cluster_pillar.databases.update_database_name(db, new_database_name)

    metadb.update_cluster_pillar(cid, cluster_pillar)

    return create_operation(
        task_type=PostgresqlTasks.database_modify,
        operation_type=PostgresqlOperations.database_modify,
        metadata=ModifyDatabaseMetadata(database_name=database_name),
        cid=cid,
        task_args={
            'target-database': database_name,
            'new-database-name': new_database_name,
            'zk_hosts': cluster_pillar.pgsync.get_zk_hosts_as_str(),
            'extension-dependencies': str({k: list(v) for k, v in EXTENSION_DEPENDENCIES.items()}),
            'new-extensions': str(sorted(new_extensions)),
            'deleted-extensions': str(sorted(deleted_extensions)),
        },
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.DELETE)
def delete_postgresql_database(cluster, database_name, **_):
    """
    Deletes postgresql database. Returns task for worker
    """
    cluster_pillar = get_cluster_pillar(cluster)
    cluster_pillar.databases.delete_database(database_name)
    metadb.update_cluster_pillar(cluster['cid'], cluster_pillar)

    return create_operation(
        task_type=PostgresqlTasks.database_delete,
        operation_type=PostgresqlOperations.database_delete,
        metadata=DeleteDatabaseMetadata(database_name=database_name),
        cid=cluster['cid'],
        task_args={'target-database': database_name},
    )


@register_request_handler(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.INFO)
def get_postgresql_database(cluster, database_name, **_):
    """
    Returns PostgreSQL database.
    """
    cluster_pillar = get_cluster_pillar(cluster)
    db = cluster_pillar.databases.database(database_name)
    return convertors.database_to_spec(db, cluster['cid'])


@register_request_handler(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.LIST)
def list_postgresql_database(cluster, **_):
    """
    Returns list of postgresql databases
    """
    cluster_pillar = get_cluster_pillar(cluster)
    return {
        MY_OBJ: [convertors.database_to_spec(db, cluster['cid']) for db in cluster_pillar.databases.get_databases()],
    }

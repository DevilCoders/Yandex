# -*- coding: utf-8 -*-
"""
DBaaS exception definitions.
"""
from typing import Sequence

from werkzeug.exceptions import HTTPException


class DbaasError(Exception):
    """
    Generic DBaaS error.
    """


class FolderResolveError(DbaasError):
    """
    Error occurred during resolving folder for request
    """


class AlertGroupResolveError(DbaasError):
    """
    Error occurred during resolving folder for request
    """


class CloudResolveError(DbaasError):
    """
    Error occurred during resolving cloud for request
    """


class UnsupportedHandlerError(DbaasError):
    """Raised when there is no handler for the given type of cluster"""


class UnsupportedAuthActionError(DbaasError):
    """Raised when there is no auth action for the given type of cluster, resource and operation"""


class BackupError(DbaasError):
    """
    Base backup error
    """


class BackupListingError(BackupError):
    """
    Unable to fetch backups into from s3
    """


class MalformedBackup(BackupError):
    """
    Malformed backup structure
    """


class DbaasHttpError(HTTPException):
    """
    Generic DBaaS HTTP-error. Designed for catching by middleware.
    Middleware must convert this type of exception to HTTP code.
    """

    code = 503

    _http_grpc_error_map = {
        400: 3,  # 400 Bad Request:             INVALID_ARGUMENT
        401: 16,  # 401 Unauthorized:           UNAUTHENTICATED
        403: 7,  # 403 Forbidden:               PERMISSION_DENIED
        404: 5,  # 404 Not Found:               NOT_FOUND
        409: 6,  # 409 Conflict:                ALREADY_EXISTS
        422: 3,  # 422 Unprocessable Entity:    INVALID_ARGUMENT
        501: 12,  # 501 Not Implemented:        UNIMPLEMENTED
        503: 14,  # 503 Service unavailable:    UNAVAILABLE
    }

    def __init__(self, message, code=None, grpc_code=None, details=None):
        super().__init__(description=message)
        if code:
            self.code = code
        self._grpc_code = grpc_code
        if details is None:
            details = []
        self.details = details

    @property
    def message(self):
        """
        Error message.
        """
        return self.description

    @property
    def grpc_code(self):
        """
        google.rpc.Code value.
        """
        if self._grpc_code:
            return self._grpc_code

        return self._http_grpc_error_map.get(self.code, 503)

    @property
    def data(self):
        """
        Error representation in flask_restful error handler.
        """
        err = {
            'code': self.grpc_code,
            'message': self.message,
        }
        if self.details:
            err['details'] = self.details
        return err


class DbaasForbidden(DbaasHttpError):
    """
    Raised on attempt to create multiple hosts at once.
    """

    code = 403

    def __init__(self, message=None):
        if not message:
            message = "Operation is forbidden"
        super().__init__(message)


class DbaasNotImplementedError(DbaasHttpError):
    """
    Error to raise for not implemented features
    """

    code = 501

    def __init__(self, message):
        super().__init__(message)


class BatchHostCreationNotImplementedError(DbaasNotImplementedError):
    """
    Raised on attempt to create multiple hosts at once.
    """

    def __init__(self):
        super().__init__('Adding multiple hosts at once is not supported yet')


class BatchHostDeletionNotImplementedError(DbaasNotImplementedError):
    """
    Raised on attempt to delete multiple hosts at once.
    """

    def __init__(self):
        super().__init__('Deleting multiple hosts at once is not supported yet')


class BatchHostModifyNotImplementedError(DbaasNotImplementedError):
    """
    Raised on attempt to modify multiple hosts at once.
    """

    def __init__(self):
        super().__init__('Updating multiple hosts at once is not supported yet')


class DbaasClientError(DbaasHttpError):
    """
    Error to raise for invalid arguments
    """

    code = 422


class PreconditionFailedError(DbaasClientError):
    """
    The operation was rejected because the system is not in a state required
    for the operation's execution.
    """

    def __init__(self, message):
        super().__init__(message, grpc_code=9)


class ParseConfigError(DbaasClientError):
    """
    Raised when errors detected in config spec
    """

    def __init__(self, err):
        super().__init__('Error parsing configSpec: {0}'.format(err))


class FeatureUnavailableError(PreconditionFailedError):
    """
    Raised when required feature flag is disabled.
    """

    def __init__(self):
        super().__init__('Requested feature is not available')


class ClusterNotFound(DbaasHttpError):
    """
    Error occurred if cluster does not exist.
    """

    code = 404

    def __init__(self, cluster_id):
        super().__init__('Cluster \'{0}\' not found'.format(cluster_id))


class ClusterAlreadyExistsError(DbaasHttpError):
    """
    Error occurred on cluster creation if cluster already exists.
    """

    code = 409

    def __init__(self, name):
        super().__init__('Cluster \'{name}\' already exists'.format(name=name))


class ActiveTasksExistError(DbaasHttpError):
    """
    Error occurred on cluster deletion if cluster has active tasks.
    """

    code = 409

    def __init__(self, cid):
        super().__init__('Cluster \'{cid}\' has active tasks'.format(cid=cid))


class PgDatabasesAlreadyExistError(DbaasHttpError):
    """
    Error occurred on cluster creation if database already exists.
    """

    code = 409

    def __init__(self, names):
        pretty_names = ', '.join("'%s'" % name for name in names)
        super().__init__('One of specified databases already exist: {names}'.format(names=pretty_names))


class HostHasActiveReplicsError(DbaasHttpError):
    """
    Error occurred on host deletion if it has active replicas.
    """

    code = 409

    def __init__(self, name, slaves):
        super().__init__('Host \'{name}\' has active replicas \'{slaves}\''.format(name=name, slaves=slaves))


class LastHostInHaGroupError(DbaasHttpError):
    """
    Error occurred on deletion host from HA group if it was last.
    """

    code = 422

    def __init__(self):
        super().__init__('At least one HA host is required')


class InvalidFailoverTarget(DbaasHttpError):
    """
    Error occured on failover if target host is not in HA group
    """

    code = 422

    def __init__(self, fqdn):
        super().__init__('Target host \'{fqdn}\' is not in HA group'.format(fqdn=fqdn))


class NotEnoughHAHosts(DbaasHttpError):
    """
    Error occured on failover if the size of HA group is too small
    """

    code = 422

    def __init__(self):
        super().__init__('HA group is too small to perform failover')


class MalformedReplicationChain(DbaasHttpError):
    """
    Error occurred on bad replication chain (cycle for example)
    """

    code = 422

    def __init__(self):
        super().__init__('Replication chain should end on HA host')


class ReplicationSourceOnCreateCluster(DbaasHttpError):
    """
    Error occurred on cluster creating if host replication source provided
    """

    code = 422

    def __init__(self):
        super().__init__('Cannot provide replication source')


class UserExistsError(DbaasHttpError):
    """
    Error occurred on user creation if it already exists.
    """

    code = 409

    def __init__(self, name):
        super().__init__(f'User \'{name}\' already exists')


class UserNotExistsError(DbaasHttpError):
    """
    Error occurred on user change if it does not exist.
    """

    code = 404

    def __init__(self, name):
        super().__init__(f'User \'{name}\' does not exist')


class HadoopJobNotExistsError(DbaasClientError):
    """
    Error occurred on Dataproc job change if it does not exist.
    """

    code = 404

    def __init__(self, job_id):
        super().__init__(f'Data Proc job \'{job_id}\' does not exist')


class UserInvalidError(DbaasClientError):
    """
    Error occurred on user change if it is not allowed.
    """


class DatabaseInvalidError(DbaasClientError):
    """
    Error occurred on database change if it is not allowed.
    """


class DatabaseExistsError(DbaasHttpError):
    """
    Error occurred on user creation if it already exists.
    """

    code = 409

    def __init__(self, name):
        super().__init__('Database \'{0}\' already exists'.format(name))


class DatabaseNotExistsError(DbaasHttpError):
    """
    Error occurred on user change if it does not exist.
    """

    code = 404

    def __init__(self, name, code=None):
        super().__init__('Database \'{0}\' does not exist'.format(name), code)


class IncorrectTemplateError(DbaasHttpError):
    """
    Error occurred on create database with wrong LC_* setting
    """

    code = 422

    def __init__(self, name, template, setting, code=None):
        super().__init__(
            '{0} of database \'{1}\' and template database \'{2}\' should be equal'.format(setting, name, template),
            code,
        )


class DatabaseAccessNotExistsError(DbaasHttpError):
    """
    Error occurred on attempt to revoke user permission that does not exist.
    """

    code = 409

    def __init__(self, user_name, db_name):
        super().__init__('User \'{0}\' has no access to the database \'{1}\''.format(user_name, db_name))


class DatabaseRolesNotExistError(DbaasHttpError):
    """
    Error occurred on attempt to revoke role that does not exist.
    """

    code = 409

    def __init__(self, user_name, roles, db_name):
        if len(roles) == 1:
            roles_str = 'role ' + roles[0]
        else:
            roles_str = 'roles ' + ','.join(roles)
        super().__init__('User \'{0}\' has no {1} in database \'{2}\''.format(user_name, roles_str, db_name))


class DatabaseAccessExistsError(DbaasHttpError):
    """
    Error occurred on attempt to add duplicated user permission.
    """

    code = 409

    def __init__(self, user_name, db_name):
        super().__init__('User \'{0}\' already has access to the database' ' \'{1}\''.format(user_name, db_name))


class DatabasePermissionsError(DbaasClientError):
    """
    Error occured due to an invalid or disallowed db-to-perms combination.
    """

    def __init__(self, dbname, permission):
        msg = 'Invalid permission combination: cannot assign \'{0}\' to DB \'{1}\''
        super().__init__(msg.format(permission, dbname))


class MlModelExistsError(DbaasHttpError):
    """
    Error occurred on ML model creation if it already exists.
    """

    code = 409

    def __init__(self, name):
        super().__init__(f'ML model \'{name}\' already exists')


class MlModelNotExistsError(DbaasHttpError):
    """
    Error occurred when referencing ML model that does not exist.
    """

    code = 404

    def __init__(self, name):
        super().__init__(f'ML model \'{name}\' does not exist')


class FormatSchemaExistsError(DbaasHttpError):
    """
    Error occurred on format schema creation if it already exists.
    """

    code = 409

    def __init__(self, name):
        super().__init__(f'Format schema \'{name}\' already exists')


class FormatSchemaNotExistsError(DbaasHttpError):
    """
    Error occurred when referencing format schema that does not exist.
    """

    code = 404

    def __init__(self, name):
        super().__init__(f'Format schema \'{name}\' does not exist')


class ShardGroupNotExistsError(DbaasHttpError):
    """
    Error occurred when referencing shard group that does not exist.
    """

    code = 404

    def __init__(self, name):
        super().__init__(f'Shard group \'{name}\' does not exist')


class HostExistsError(DbaasHttpError):
    """
    Error occurred on host creation if it already exists.
    """

    code = 409

    def __init__(self, name):
        super().__init__('Host \'{0}\' already exists'.format(name))


class HostNotExistsError(DbaasHttpError):
    """
    Error occurred on host change if it does not exist.
    """

    code = 404

    def __init__(self, name):
        super().__init__('Host \'{0}\' does not exist'.format(name))


class UnknownClusterForHostError(DbaasHttpError):
    """
    Error occurred if cluster does not exists for host.
    """

    code = 404

    def __init__(self, fqdn):
        super().__init__('Unknown cluster for host {}'.format(fqdn))


class ShardExistsError(DbaasHttpError):
    """
    Error occurred on shard creation if it already exists.
    """

    code = 409

    def __init__(self, name):
        super().__init__('Shard \'{0}\' already exists'.format(name))


class ShardNameCollisionError(DbaasHttpError):
    """
    Error occurred if there exists a shard with a name that differs only by case.
    """

    code = 409

    def __init__(self, name, conflicting_name):
        msg = 'Cannot create shard \'{0}\': shard with name \'{1}\' already exists'
        super().__init__(msg.format(name, conflicting_name))


class ShardNotExistsError(DbaasHttpError):
    """
    Error occurred if shard does not exist.
    """

    code = 404

    def __init__(self, name):
        super().__init__('Shard \'{0}\' does not exist'.format(name))


class SubclusterNotExistsError(DbaasHttpError):
    """
    Error occurred if subcluster does not exist.
    """

    code = 404

    def __init__(self, subcid):
        super().__init__('Subcluster \'{0}\' does not exist'.format(subcid))


class MongoDBMixedShardingInfraConfigurationError(DbaasClientError):
    """
    Client tried to use MongoCFG + MongoInfra in same cluster.
    """

    def __init__(self):
        super().__init__('Using MongoCFG instances together with MongoInfra is not supported')


class NoChangesError(DbaasClientError):
    """
    Error occurred when no parameters to change are specified.
    """

    code = 400

    def __init__(self):
        super().__init__('No changes detected')


class ClusterTypeMismatchError(DbaasClientError):
    """
    Client requested one type of cluster in URL, but provided ID for another.
    """

    def __init__(self, cluster_id):
        super().__init__('Wrong cluster type for ID \'{0}\''.format(cluster_id))


class ResourcePresetNotExists(DbaasClientError):
    """
    Client used non-existent resource preset ID.
    """

    def __init__(self, resource_preset_id):
        super().__init__('Resource preset \'{0}\' does not exist'.format(resource_preset_id))


class ResourcePresetIsDecommissioned(DbaasClientError):
    """
    Client used decommissioning resource preset ID.
    """

    def __init__(self, resource_preset_id):
        super().__init__('Resource preset \'{0}\' is decommissioned'.format(resource_preset_id))


class BackupNotExistsError(DbaasHttpError):
    """
    Error to raise for non existed backups
    """

    code = 404

    def __init__(self, backup_id):
        super().__init__('Backup \'{0}\' does not exist'.format(backup_id))


class AlertGroupNotExistsError(DbaasHttpError):
    """
    Error to raise for non existed alert group
    """

    code = 404

    def __init__(self, ag_id):
        super().__init__('Alert group \'{0}\' does not exist'.format(ag_id))


class NoVersionError(DbaasClientError):
    """
    Error to raise when version is not specified.
    """

    def __init__(self) -> None:
        super().__init__('Version is not specified')


class VersionNotExistsError(DbaasClientError):
    """
    Error to raise for unsupported version.
    """

    def __init__(self, version, allowed: Sequence[str] = None):
        message = 'Version \'{0}\' is not available'.format(version)
        if allowed:
            message += ', allowed versions are: {0}'.format(', '.join(allowed))
        super().__init__(message)


class ConfigTargetVersionMismatchError(DbaasClientError):
    """
    Error to raise when explicitly specified version mismatches config version.
    """

    def __init__(self, version: str, config_key: str) -> None:
        super().__init__(
            'Explicitly specified version ({0}) mismatches target version ({1}) of the specified config'.format(
                version, config_key
            )
        )


class OperationNotExistsError(DbaasHttpError):
    """
    Error to raise for non existed operation
    """

    code = 404

    def __init__(self, operation_id):
        super().__init__('Operation by that ID was not found \'{0}\''.format(operation_id))


class OperationTypeMismatchError(DbaasClientError):
    """
    Client requested one type of cluster in URL,
    but provided operation ID from another cluster_type
    """

    def __init__(self, operation_id):
        super().__init__('Wrong cluster type for operation ID \'{0}\''.format(operation_id))


class SubnetNotFound(DbaasClientError):
    """
    Client requested creation of host in subnet,
    but we were unable to find it
    """

    def __init__(self, subnet_id, zone_id):
        self.zone_id = zone_id
        super().__init__('Unable to find subnet with id \'{0}\' in zone \'{1}\''.format(subnet_id, zone_id))


class NoSubnetInZone(DbaasClientError):
    """
    Client requested creation of host in zone,
    but we were unable to find subnet for this cluster
    """

    def __init__(self, zone_id):
        self.zone_id = zone_id
        super().__init__('Unable to find subnet in zone \'{zone_id}\''.format(zone_id=zone_id))


class NonUniqueSubnetInZone(DbaasClientError):
    """
    Client requested creation of host in zone without providing subnet_id,
    but there are multiple subnets in this zone
    """

    def __init__(self, zone_id):
        self.zone_id = zone_id
        super().__init__('Multiple subnets in zone \'{zone_id}\''.format(zone_id=zone_id))


class QuotaViolationHttpError(DbaasClientError):
    """
    Quota violation DBaaS HTTP-error.
    """

    code = 422

    def __init__(self, message, cloud_ext_id, violations):
        super().__init__(message, grpc_code=9)
        self.details = [
            {
                "@type": "type.private-api.yandex-cloud.ru/quota.QuotaFailure",
                "cloud_id": cloud_ext_id,
                "violations": violations,
            }
        ]


class HadoopJobLogNotFoundError(DbaasClientError):
    """
    Error occurred on Dataproc job log get if it doesn't found on bucket
    """

    code = 404

    def __init__(self, job_id, bucket):
        super().__init__(f'Data Proc job log \'{job_id}\' not found in bucket {bucket}')


class CloudStorageVersionNotSupportedError(DbaasClientError):
    """
    Error indicates that ClickHouse version is not supported for clusters with cloud storage.
    """

    def __init__(self, version):
        super().__init__(f'Minimum required version for clusters with cloud storage is {version}')


class ClickhouseKeeperVersionNotSupportedError(DbaasClientError):
    """
    Error indicates that ClickHouse version is not supported for clusters with ClickHouse Keeper.
    """

    def __init__(self, version):
        super().__init__(f'Minimum required version for clusters with ClickHouse Keeper is {version}')


class CloudStorageBackupsNotSupportedError(DbaasClientError):
    """
    Error indicates that backups are not supported for clusters with enabled cloud storage.
    """

    def __init__(self):
        super().__init__('Backups are currently not supported for clusters with enabled cloud storage')


class DiskTypeIdError(DbaasHttpError):
    """
    Error occurred on host change if it does not exist.
    """

    code = 404

    def __init__(self, disk_type_id, disk_types):
        super().__init__(f'Disk type with \'{disk_type_id}\' not found in {disk_types}')


class UserAPIDisabledError(DbaasClientError):
    """
    Error indicates that SQL user management enabled, but user tried to update ClickHouse user via API.
    """

    def __init__(self):
        super().__init__(
            'The requested functionality is not available for clusters with enabled SQL user management.'
            ' Please use SQL interface to manage users.'
        )


class DatabaseAPIDisabledError(DbaasClientError):
    """
    Error indicates that SQL database management enabled, but user tried to update ClickHouse database via API.
    """

    def __init__(self):
        super().__init__(
            'The requested functionality is not available for clusters with enabled SQL database '
            'management. Please use SQL interface to manage databases.'
        )


class ClusterDeleteProtectionError(PreconditionFailedError):
    """
    The operation was rejected because cluster has 'deletion_protection' = ON
    """

    def __init__(self):
        super().__init__("The operation was rejected because cluster has 'deletion_protection' = ON")


class AlertDoesNotExists(DbaasHttpError):
    """
    Error occurred on alert creation if no such service provided metric exists.
    """

    code = 404

    def __init__(self, name):
        super().__init__('Alert metric \'{name}\' does not exists'.format(name=name))


class AlertGroupDoesNotExists(DbaasHttpError):
    """
    Error occurred if no such alert group
    """

    code = 404

    def __init__(self, ag_id):
        super().__init__('Alert group \'{ag_id}\' does not exists'.format(ag_id=ag_id))


class AlertCreationFail(PreconditionFailedError):
    """
    Error occurred on alert creation if no thresholds are provided
    """

    def __init__(self, template_id):
        super().__init__(f'Invalid alert {template_id} specification')


class ManagedAlertGroupDeletionError(PreconditionFailedError):
    """
    Error occurred on alert group deletion if group is managed
    """

    def __init__(self, name):
        super().__init__('Deletion of managed alert group \'{name}\' is prohibited'.format(name=name))


class IdmUserEditError(DbaasForbidden):
    """
    Error occured on idm user change
    """

    def __init__(self, user_name):
        super().__init__("IDM user \'{user_name}\' editing is forbidden".format(user_name=user_name))


class IdmSysUserPropertyEditError(DbaasForbidden):
    """
    Error occured on idm user change
    """

    def __init__(self, user_name, property_name):
        super().__init__(
            "IDM system user \'{user_name}\' \'{property_name}\' editing is forbidden".format(
                user_name=user_name, property_name=property_name
            )
        )

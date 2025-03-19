"""
Operations API helpers for response construction
"""
from abc import ABCMeta, abstractmethod, abstractproperty
from typing import Any, Dict, Type  # noqa

from dbaas_common import tracing
from typing_extensions import Protocol

from ..core.exceptions import (
    DatabaseNotExistsError,
    HadoopJobNotExistsError,
    ShardNotExistsError,
    ShardGroupNotExistsError,
    SubclusterNotExistsError,
    UnsupportedHandlerError,
    UserNotExistsError,
    MlModelNotExistsError,
    FormatSchemaNotExistsError,
)
from ..core.types import ResponseType
from ..modules.hadoop.jobs import get_job_handler
from ..utils import metadb
from ..utils.logs import log_warn
from ..utils.register import DbaasOperation, Resource, get_request_handler, get_response_schema
from .info import render_typed_cluster


class UnsupportedResponserException(Exception):
    """
    Unsupported responser for given cluster_type
    """


class _Cached(metaclass=ABCMeta):
    """
    Simple cacher
    """

    operation = None  # type: DbaasOperation
    resource = None  # type: Resource

    def __init__(self, cluster_type: str) -> None:
        self._cache = {}  # type: Dict[tuple, Any]
        self.cluster_type = cluster_type
        if self.operation and self.resource:
            try:
                self._schema = get_response_schema(self.cluster_type, self.resource, self.operation)()
            except KeyError:
                log_warn('No %s resource for %s', self.resource, self.cluster_type)
                self._schema = None
        else:
            self._schema = None

    @tracing.trace('Get Operation Cached Response')
    def get(self, key: tuple) -> Any:
        """
        Get response by key
        """
        tracing.set_tag('response.operation', self.operation)
        tracing.set_tag('response.resource', self.resource)
        tracing.set_tag('response.key', key)

        cached = True
        if key not in self._cache:
            cached = False
            self._cache[key] = self._get(key)
        tracing.set_tag('response.cached', cached)
        return self._cache[key]

    @abstractmethod
    def _get(self, key: tuple) -> Any:
        pass


class ClusterResponser(_Cached):
    """
    Get cluster responser
    """

    resource = Resource.CLUSTER
    operation = DbaasOperation.INFO

    def _get(self, key: tuple) -> Any:
        try:
            (cluster_id,) = key
        except ValueError as exc:
            log_warn('Got empty response_key %r: %s, ' 'while try make cluster response for operation ', key, exc)
            return None

        cluster_dict_from_db = metadb.get_cluster(cid=cluster_id, cluster_type=self.cluster_type)

        if cluster_dict_from_db is None:
            # probably deleted cluster
            return None

        cluster_dict_from_handler = render_typed_cluster(cluster_dict_from_db)
        cluster_out = self._schema.dump(cluster_dict_from_handler)
        if cluster_out.errors:
            log_warn('Got %s while apply %s to cluster %r', cluster_out.errors, self._schema, cluster_id)
        return cluster_out.data


class _ObjectResponser(_Cached):
    """
    Get object responser
    """

    def __init__(self, cluster_type: str) -> None:
        super().__init__(cluster_type)
        try:
            self._handler = get_request_handler(self.cluster_type, self.resource, self.operation)
        except UnsupportedHandlerError as exc:
            raise UnsupportedResponserException from exc

    @abstractproperty
    def not_exists_exc(self) -> Type[Exception]:
        """
        Child must override it
        """

    def _get(self, key: tuple) -> Any:
        try:
            cluster_id, object_name = key
        except ValueError as exc:
            log_warn(
                'Got empty response_key %r: %s, ' 'while try make %s response for operation',
                key,
                exc,
                self.resource.value,
            )
            return None

        cluster_dict = metadb.get_cluster(cid=cluster_id, cluster_type=self.cluster_type)

        if cluster_dict is None:
            # probably deleted cluster
            return None

        try:
            data = self._handler(cluster_dict, object_name)
        except self.not_exists_exc:
            # probably deleted database
            return None

        if self._schema is None:
            return data

        cluster_out = self._schema.dump(data)
        if cluster_out.errors:
            log_warn('Got %s while apply %s to cluster %r', cluster_out.errors, self._schema, cluster_id)
        return cluster_out.data


class SubclusterResponser(_ObjectResponser):
    """
    Subcluster responser
    """

    resource = Resource.SUBCLUSTER
    operation = DbaasOperation.INFO
    not_exists_exc = SubclusterNotExistsError


class ShardResponser(_ObjectResponser):
    """
    Shard responser
    """

    resource = Resource.SHARD
    operation = DbaasOperation.INFO
    not_exists_exc = ShardNotExistsError


class DatabaseResponser(_ObjectResponser):
    """
    Database responser
    """

    resource = Resource.DATABASE
    operation = DbaasOperation.INFO
    not_exists_exc = DatabaseNotExistsError


class UserResponser(_ObjectResponser):
    """
    User responser
    """

    resource = Resource.USER
    operation = DbaasOperation.INFO
    not_exists_exc = UserNotExistsError


class HadoopJobResponser(_Cached):
    """
    HadoopJob responser
    """

    resource = Resource.HADOOP_JOB
    operation = DbaasOperation.INFO
    not_exists_exc = HadoopJobNotExistsError

    def _get(self, key: tuple) -> Any:
        cluster_id, dataproc_job_id = None, None
        try:
            cluster_id, dataproc_job_id = key
            if not cluster_id or not dataproc_job_id:
                raise ValueError()
        except ValueError as exception:
            log_warn(
                f'Got an unexpected response_key {cluster_id}:{dataproc_job_id}'
                f' while trying to make Data Proc job response for operation {exception}'
            )
            return None
        job = get_job_handler({}, dataproc_job_id)
        job_out = self._schema.dump(job)
        if job_out.errors:
            log_warn(f'Got {job_out.errors} while apply {self._schema} to job {dataproc_job_id} cluster {cluster_id}')
        job_out.data['@type'] = "yandex.cloud.dataproc.v1.Job"
        return job_out.data


class ClickhouseMlModelResponser(_ObjectResponser):
    """
    Clickhouse machine learning model responser
    """

    resource = Resource.ML_MODEL
    operation = DbaasOperation.INFO
    not_exists_exc = MlModelNotExistsError


class ClickhouseFormatSchemaResponser(_ObjectResponser):
    """
    Clickhouse format schema responser
    """

    resource = Resource.FORMAT_SCHEMA
    operation = DbaasOperation.INFO
    not_exists_exc = FormatSchemaNotExistsError


class ClickhouseShardGroupResponser(_ObjectResponser):
    """
    Clickhouse shard group schema responser
    """

    resource = Resource.SHARD_GROUP
    operation = DbaasOperation.INFO
    not_exists_exc = ShardGroupNotExistsError


class _Responser(Protocol):
    """
    Responser protocol
    """

    # pylint: disable=unused-argument
    def __init__(self, cluster_type: str) -> None:
        super().__init__()

    def get(self, key: tuple) -> Any:
        """
        Get response by key
        """


class OperationsResponser:
    """
    Operations responser
    """

    _responsers_map = {
        ResponseType.cluster: ClusterResponser,
        ResponseType.shard: ShardResponser,
        ResponseType.database: DatabaseResponser,
        ResponseType.user: UserResponser,
    }  # type: Dict[ResponseType, Type[_Responser]]

    def __init__(self, cluster_type: str) -> None:
        self.cluster_type = cluster_type
        # When mypy define self._responsers type
        # it compute it as _Cached
        # and complaint that it can't be
        # instanciated cause has abstract methods
        self._responsers = dict()  # type: Dict[ResponseType, _Responser]
        for response_type, responser_cls in self._responsers_map.items():
            try:
                responser = responser_cls(cluster_type)
                self._responsers[response_type] = responser
            except UnsupportedResponserException:
                pass
        if cluster_type == 'hadoop_cluster':
            self._responsers[ResponseType.hadoop_job] = HadoopJobResponser(cluster_type)
            self._responsers[ResponseType.subcluster] = SubclusterResponser(cluster_type)
        if cluster_type == 'clickhouse_cluster':
            self._responsers[ResponseType.ml_model] = ClickhouseMlModelResponser(cluster_type)
            self._responsers[ResponseType.format_schema] = ClickhouseFormatSchemaResponser(cluster_type)
            self._responsers[ResponseType.shard_group] = ClickhouseShardGroupResponser(cluster_type)

    @tracing.trace('Get Operation Response')
    def get(self, response_type: ResponseType, response_key: tuple) -> Any:
        """
        Get response by its type and key
        """
        if response_type is ResponseType.empty:
            return {}
        if response_type not in self._responsers:
            log_warn('Response %r not supported by %s', response_type, self.cluster_type)
            return {}
        return self._responsers[response_type].get(response_key)

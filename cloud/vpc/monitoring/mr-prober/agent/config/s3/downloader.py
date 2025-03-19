import datetime
import io
import itertools
import logging
import os
import pathlib
import re
from copy import copy
from typing import Any, Dict, Tuple, Type, TypeVar, Optional, List, Iterable

import boto3
import pydantic
from botocore.exceptions import ClientError

from agent.config.models import *
from common.substitution import substitute, SubstituteKeyError

__all__ = ["AgentConfigS3Downloader", "produce_prober_config_variable_combinations_by_matrix"]

T = TypeVar("T")

CLUSTER_OBJECT_FIELDS = ["id", "name", "slug", "variables"]
PROBER_CONFIG_OBJECT_FIELDS = ["prober_id", "interval_seconds", "timeout_seconds", "s3_logs_policy"]


class AgentConfigS3Downloader:
    """
    Downloads configuration of probers which are needed to run on this host in this cluster.
    Configuration is stored in Yandex Object Storage (aka S3) —
    https://console.cloud.yandex.ru/folders/yc.vpc.mr-prober/storage

    Any S3 object will be downloaded once during the first use,
    then it will be downloaded only if it has changed in S3.
    """

    def __init__(self, endpoint: str, access_key_id: str, secret_access_key: str, bucket: str, prefix: str = ""):
        self.prefix = prefix
        self.bucket = bucket

        session = boto3.session.Session()
        self.s3 = session.client(
            service_name="s3",
            endpoint_url=endpoint,
            aws_access_key_id=access_key_id,
            aws_secret_access_key=secret_access_key,
        )
        # cache for downloaded s3 objects
        self._s3_objects_cache: Dict[str, S3CacheEntity] = {}

        # cache for cluster configs: cluster_id -> AgentConfig
        self._cluster_configs_cache: Dict[int, AgentConfig] = {}

        # FIXME (nuraev): add cache cleanup

    def get_config(self, cluster_id: int, hostname: str) -> AgentConfig:
        """
        This method uses cache for AgentConfigs.
        New configuration for cluster is created only at the first use or
        if the cluster configuration has changed in s3.
        """
        cluster, from_cache = self._get_cluster(cluster_id)
        if from_cache and cluster_id in self._cluster_configs_cache:
            return self._cluster_configs_cache[cluster_id]

        effective_configs = {}
        for prober_config in cluster.prober_configs:
            if prober_config.hosts_re is not None and not self._is_matched_with_hosts_re(hostname, prober_config):
                continue
            if prober_config.prober_id not in effective_configs:
                effective_configs[prober_config.prober_id] = make_default_prober_config(prober_config.prober_id)
            effective_configs[prober_config.prober_id].update_with(prober_config)

        variables_map = {
            "cluster": {name: getattr(cluster, name) for name in CLUSTER_OBJECT_FIELDS},
        }
        configuration = AgentConfig(cluster=ClusterMetadata(cluster), probers=[])
        for prober_id, prober_config in effective_configs.items():
            # Matrix duplicates prober config into a lot of configs with different sets of variable values
            combinations = produce_prober_config_variable_combinations_by_matrix(prober_config)

            variables_map["config"] = {name: getattr(prober_config, name) for name in PROBER_CONFIG_OBJECT_FIELDS}
            prober, _ = self._get_prober(prober_id)
            for matrix_variable_values in combinations:
                real_prober_config = prober_config.copy(deep=True)
                if prober_config.variables:
                    variables_map["variables"] = variables_map["config"]["variables"] = {}
                    variables_map["matrix"] = matrix_variable_values
                    real_prober_config.variables = expand_prober_config_variables(prober_config, variables_map)
                    real_prober_config.additional_metric_labels.update({
                        f"matrix_{name}": value
                        for name, value in matrix_variable_values.items()
                    })

                # Key should be unique for each matrix combination,
                # i.e. (1, "yandex.ru", "ipv4"), (1, "yandex.ru", "ipv6"), etc.
                unique_key = (prober.id, *matrix_variable_values.values())
                prober_with_variables_in_name = prober.copy(
                    update={"name": self._get_prober_name_with_matrix_variables(prober.name, matrix_variable_values)}
                )
                prober_with_config = ProberWithConfig(
                    unique_key=unique_key, prober=prober_with_variables_in_name, config=real_prober_config
                )
                configuration.probers.append(prober_with_config)

        self._cluster_configs_cache[cluster_id] = configuration
        return configuration

    def download_and_store_prober_file(self, prober: Prober, file: ProberFile, dirname: pathlib.Path) -> os.PathLike:
        path = dirname / file.relative_file_path
        pathlib.Path(path.parent).mkdir(parents=True, exist_ok=True)

        s3_file_path = f"{self.prefix}probers/{prober.id}/files/{file.id}"
        logging.info("Downloading prober file from %r to %s", s3_file_path, path.as_posix())

        content = io.BytesIO()
        self.s3.download_fileobj(self.bucket, s3_file_path, content)
        content.seek(0)

        # FIXME (syringa) check md5?
        path.write_bytes(content.read())
        return path

    @staticmethod
    def _is_matched_with_hosts_re(hostname: str, prober_config: ProberConfig) -> bool:
        try:
            if re.match(prober_config.hosts_re, hostname, re.IGNORECASE):
                return True
        except re.error as ex:
            logging.warning(
                "Processing the hosts regexp for prober config with id %d failed: %r",
                prober_config.prober_id, ex
            )
        return False

    def _get_cluster(self, cluster_id: int) -> Tuple[Cluster, bool]:
        s3_object_path = f"{self.prefix}clusters/{cluster_id}/cluster.json"
        return self._get_object(s3_object_path, Cluster)

    def _get_prober(self, prober_id: int) -> Tuple[Prober, bool]:
        s3_object_path = f"{self.prefix}probers/{prober_id}/prober.json"
        return self._get_object(s3_object_path, Prober)

    def _get_object(self, s3_object_path: str, object_type: Type[T]) -> Tuple[T, bool]:
        """
        In order not to re-download objects that are already in the cache and
        at the same time be able to download the object if it has changed,
        we use header `IfModifiedSince`.

        If the object has not been modified since the time
        specified in the `IfModifiedSince` header, we get 304 and just return object from cache.
        """
        head_request_args = {
            "Bucket": self.bucket,
            "Key": s3_object_path,
        }
        if s3_object_path in self._s3_objects_cache:
            head_request_args["IfModifiedSince"] = self._s3_objects_cache[s3_object_path].last_modified

        try:
            s3_object_info = self.s3.head_object(**head_request_args)
        except ClientError as ex:
            if ex.response["Error"]["Code"] == "304":
                return self._s3_objects_cache[s3_object_path].object, True
            raise

        new_object = object_type.parse_raw(self._download_content(s3_object_path))
        self._s3_objects_cache[s3_object_path] = S3CacheEntity(
            last_modified=s3_object_info["LastModified"], object=new_object
        )
        return new_object, False

    def _download_content(self, filename: str) -> bytes:
        content = io.BytesIO()
        self.s3.download_fileobj(self.bucket, filename, content)
        content.seek(0)
        return content.read()

    @staticmethod
    def _get_prober_name_with_matrix_variables(prober_name: str, matrix_variable_values: Dict[str, Any]) -> str:
        """
        Concatenates prober name and matrix variable values
        to produce something like "Ping(host='yandex.ru', ip_version=4)"
        """
        if not matrix_variable_values:
            return prober_name

        return f"{prober_name}({', '.join(f'{name}={value!r}' for name, value in matrix_variable_values.items())})"


class S3CacheEntity(pydantic.BaseModel):
    last_modified: datetime.datetime
    object: Any


def produce_prober_config_variable_combinations_by_matrix(prober_config: ProberConfig) -> Iterable[Dict[str, Any]]:
    """
    Yields all combinations for matrix variables. If prober config has matrix variables
    'host' with possible values "yandex.ru" and "google.com", and 'ip_version' with values "ipv4" and "ipv6",
    then this function will return 4 dicts:
    {"host": "yandex.ru", "ip_version": "ipv4"}
    {"host": "yandex.ru", "ip_version": "ipv6"}
    {"host": "google.com", "ip_version": "ipv4"}
    {"host": "google.com", "ip_version": "ipv6"}
    """
    matrix_variables = prober_config.matrix_variables if prober_config.matrix_variables else {}

    matrix_variable_names = matrix_variables.keys()
    for combination in itertools.product(*matrix_variables.values()):
        yield dict(zip(matrix_variable_names, combination))


def expand_prober_config_variables(prober_config: ProberConfig, variables_map: Dict[str, Any]) -> Dict[str, Any]:
    """
    Returns new dictionary with substituted variables.
    """
    result_variables = {}
    for var_name, var_value in prober_config.variables.items():
        if var_name in variables_map["config"]["variables"]:
            # if the variable has already been set, for example, inside the recursion, just put it in the result
            result_variables[var_name] = variables_map["config"]["variables"][var_name]
            continue

        try:
            result_variables[var_name] = expand_variable(prober_config, var_name, var_value, variables_map)
        except KeyError as ex:
            raise Exception(f"variable '{var_name}' can not be substituted: variable '{ex.args[0]}' not found")
        except Exception as ex:
            raise Exception(f"variable '{var_name}' can not be substituted: {ex}")

    return result_variables


def expand_variable(
    prober_config: ProberConfig,
    var_name,
    var_value,
    variables_map,
    call_log: Optional[List[str]] = None,
    depth: int = 0,
) -> Any:
    """
    This function tries to substitute values in the specified variable.

    Since the function is run recursively, 'call_log' protects against cyclic dependencies.
    'depth' shows the value of the nesting of a variable. Necessary for limit the depth of nesting of variables.
    """
    if not isinstance(var_value, str):
        variables_map["config"]["variables"][var_name] = var_value
        return var_value

    if call_log and var_name in call_log:
        if len(call_log) == 1:
            raise Exception(f"the variable '{var_name}' refer to itself")
        raise Exception(f"variables '{', '.join(call_log)}' have a cyclic dependence")

    try:
        new_var_value = substitute(var_value, variables_map, depth=depth)
        variables_map["config"]["variables"][var_name] = new_var_value
        return new_var_value
    except SubstituteKeyError as ex:
        # NOTE: here we are looking only a local variables (for example, prober config variables),
        # the rest of the variables (for example, cluster variables) should already be
        if ex.key.startswith("config.variables") or ex.key.startswith("variables."):
            key = ex.key.split(".")[-1]
            if key not in prober_config.variables:
                raise KeyError(ex.key)
            expand_variable(
                prober_config, key, prober_config.variables[key], variables_map,
                call_log=(copy(call_log) if call_log else []) + [var_name]
            )
            return expand_variable(prober_config, var_name, ex.value, variables_map, call_log, ex.depth)
        raise KeyError(ex.key)

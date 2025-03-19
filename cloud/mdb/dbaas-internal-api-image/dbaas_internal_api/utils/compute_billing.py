# -*- coding: utf-8 -*-
"""
DBaaS Internal API utils for Cloud Compute API
"""
import time
from abc import ABC, abstractmethod
from threading import Lock, local
from typing import Any, Dict, List, Optional

from flask import current_app

from dbaas_common import tracing

from cloud.mdb.internal.python import grpcutil
from cloud.mdb.internal.python.compute import instances
from cloud.mdb.internal.python.grpcutil.exceptions import FailedPreconditionError, NotFoundError
from cloud.mdb.internal.python.logs import MdbLoggerAdapter

from .logs import get_logger
from ..core.exceptions import PreconditionFailedError
from ..utils.request_context import get_x_request_id
from ..utils import iam_jwt

PROC_CONTEXT = {}  # type: Dict[str, Any]
THREAD_CONTEXT = local()


class YCComputeInstanceResources:
    """
    Wrapper around Compute ResourceSpec
    """

    def __init__(self, memory: int, cores: int, core_fraction: int, io_cores: int, gpus: int, platform_id: str) -> None:
        self.memory = memory
        self.cores = cores
        self.core_fraction = core_fraction
        self.io_cores = io_cores
        self.gpus = gpus
        self.platform_id = platform_id


class YCComputeDisk:
    """
    Wrapper around Compute AttachedDiskSpec.DiskSpec
    """

    def __init__(self, type_id: Optional[str], size: Optional[int], image_id: str) -> None:
        self.type_id = type_id
        self.size = size
        self.image_id = image_id


class YCComputeInstance:  # pylint: disable=too-few-public-methods
    """
    Wrapper around Compute Instance
    """

    def __init__(
        self,
        folder_id: str,
        zone_id: Optional[str],
        resources: YCComputeInstanceResources,
        subnet_id: Optional[str],
        boot_disk: YCComputeDisk,
        host_group_ids: Optional[List[str]],
    ) -> None:
        self.folder_id = folder_id
        self.zone_id = zone_id
        self.resources = resources
        self.subnet_id = subnet_id
        self.boot_disk = boot_disk
        self.host_group_ids = host_group_ids if host_group_ids else []


# pylint: disable=too-few-public-methods
class ComputeBillingProvider(ABC):
    """
    Abstract Compute billing Provider
    """

    @abstractmethod
    def get_billing_metrics(self, instance: YCComputeInstance):
        """
        Get billing metrics for compute instance
        """


class YCComputeBillingProvider(ComputeBillingProvider):
    """
    YC.Compute Provider
    """

    def __init__(self, config):
        self.cache_ttl = 300
        self.cache_size = 1024
        self.iam_jwt = iam_jwt.get_provider()

        logger = MdbLoggerAdapter(
            get_logger(),
            extra={
                'request_id': get_x_request_id(),
            },
        )
        self._instances = instances.InstancesClient(
            config=instances.InstancesClientConfig(
                transport=grpcutil.Config(
                    url=config['url'],
                    cert_file=config['ca_path'],
                ),
                timeout=config.get('timeout', 5.0),
            ),
            logger=logger,
            token_getter=lambda: self.iam_jwt.get_iam_token(),
            error_handlers={},
        )

    def _gc_cache(self, ref_time):
        self._cache_exists()
        if (
            len(PROC_CONTEXT['permission_cache']) <= self.cache_size
            or PROC_CONTEXT['permission_cache_gc_lock'].locked()
        ):
            return
        with PROC_CONTEXT['permission_cache_gc_lock']:
            for item in sorted(PROC_CONTEXT['permission_cache'].items(), key=lambda i: i[1][0]):
                if ref_time - item[1][0] > self.cache_ttl:
                    del PROC_CONTEXT['permission_cache'][item[0]]
                elif len(PROC_CONTEXT['permission_cache']) > self.cache_size:
                    del PROC_CONTEXT['permission_cache'][item[0]]
                else:
                    break

    def _get_billing_metrics_uncached(self, instance: YCComputeInstance):
        zone_id = instance.zone_id if instance.zone_id else ''
        subnets = []
        if instance.subnet_id:
            subnet = instances.SubnetRequest(v4_cidr=True, v6_cidr=False, subnet_id=instance.subnet_id)
            subnets.append(subnet)

        try:
            metrics = self._instances.create_instance(
                zone_id=zone_id,
                fqdn='',
                boot_disk_size=instance.boot_disk.size,
                image_id=instance.boot_disk.image_id,
                platform_id=instance.resources.platform_id,
                cores=instance.resources.cores,
                core_fraction=instance.resources.core_fraction,
                gpus=instance.resources.gpus,
                io_cores=instance.resources.io_cores,
                memory=instance.resources.memory,
                assign_public_ip=False,
                security_group_ids=[],
                disk_type_id='',
                disk_size=None,
                boot_disk_type_id=instance.boot_disk.type_id,
                folder_id=instance.folder_id,
                metadata={},
                service_account_id=None,
                idempotence_id='',
                name='',
                secondary_disk_specs=[],
                boot_disk_spec=None,
                subnets=subnets,
                labels={},
                references=[],
                host_group_ids=instance.host_group_ids,
                disk_placement_group_id=None,
                simulate_billing=True,
            )
        except (FailedPreconditionError, NotFoundError) as ex:
            raise PreconditionFailedError(ex.message)
        result = []
        for metric in metrics:
            result.append(
                {
                    'schema': metric.schema,
                    'tags': metric.tags,
                    'folder_id': metric.folder_id,
                    'cloud_id': metric.cloud_id,
                }
            )
        return result

    def _cache_exists(self):
        if 'permission_cache' not in PROC_CONTEXT:
            PROC_CONTEXT['permission_cache'] = {}
        if 'permission_cache_gc_lock' not in PROC_CONTEXT:
            PROC_CONTEXT['permission_cache_gc_lock'] = Lock()

    def _cache_get(self, key):
        self._cache_exists()
        now = time.time()
        req_time, result = PROC_CONTEXT['permission_cache'].get(key, (0, []))
        if now - req_time >= self.cache_ttl:
            return None
        return result

    def _cache_put(self, key, value):
        self._cache_exists()
        PROC_CONTEXT['permission_cache'][key] = (time.time(), value)

    @tracing.trace('Compute SimulateBillingMetrics')
    def get_billing_metrics(self, instance: YCComputeInstance):
        """
        Get billing metrics for compute instance
        """
        result = self._cache_get(instance)
        if not result:
            result = self._get_billing_metrics_uncached(instance)
            self._cache_put(instance, result)
        self._gc_cache(time.time())
        return result


def get_provider() -> ComputeBillingProvider:
    """
    Get compute provider according to config and flags
    """
    return current_app.config['COMPUTE_BILLING_PROVIDER'](current_app.config['COMPUTE_GRPC'])


def get_billing_metrics(instance: YCComputeInstance):
    """
    Get billing metrics for instance
    """
    return get_provider().get_billing_metrics(instance)

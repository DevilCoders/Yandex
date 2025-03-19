# -*- coding: utf-8 -*-
"""
DBaaS Internal API Hadoop cluster billing estimate helpers
"""

from typing import List, Optional

from flask import g

from ...core import exceptions as errors
from ...utils import metadb, config
from ...utils.compute_billing import (
    YCComputeDisk,
    YCComputeInstance,
    YCComputeInstanceResources,
    get_billing_metrics,
)
from ...utils.register import DbaasOperation
from ...utils.register import Resource as ResourceEnum
from ...utils.register import register_request_handler
from ...utils.types import RequestedHostResources
from .constants import MY_CLUSTER_TYPE
from .traits import ClusterService
from .types import HadoopConfigSpec
from .version import (
    get_latest_dataproc_version_by_prefix,
    get_dataproc_config_with_minor_versions,
    get_dataproc_default_version_prefix,
    get_dataproc_images_config,
)


def generate_metric(
    resources: RequestedHostResources,
    resource_preset_obj: dict,
    subnet_id: Optional[str],
    image_id: str,
    zone_id: Optional[str],
    folder_id: str,
    host_group_ids: List[str] = None,
) -> List[dict]:
    """
    Generate billing metric (without usage part)
    """
    # TODO: Remove crutch with platform_id after MDB-4953
    platform_id = resource_preset_obj['platform_id'].replace('mdb-', 'standard-')
    compute_resources = YCComputeInstanceResources(
        resource_preset_obj['memory'],
        resource_preset_obj['cores'],
        resource_preset_obj['core_fraction'],
        resource_preset_obj['io_cores'],
        resource_preset_obj.get('gpus', 0),
        platform_id,
    )
    disk = YCComputeDisk(resources.disk_type_id, resources.disk_size, image_id)
    instance = YCComputeInstance(folder_id, zone_id, compute_resources, subnet_id, disk, host_group_ids)
    return get_billing_metrics(instance)


@register_request_handler(MY_CLUSTER_TYPE, ResourceEnum.CLUSTER, DbaasOperation.BILLING_CREATE)
def billing_create_hadoop_cluster(
    config_spec: dict, zone_id: Optional[str] = None, host_group_ids: List[str] = None, **_
):
    """
    Returns billing metrics for cost estimator
    """
    try:
        # Load and validate config spec
        hadoop_config_spec = HadoopConfigSpec(config_spec)
        version_prefix = config_spec.get('version_id', get_dataproc_default_version_prefix())
        image_version, image_id, image_config = get_latest_dataproc_version_by_prefix(version_prefix)
        for subcluster in hadoop_config_spec.get_subclusters():
            resources = subcluster.get_resources()
            resource_preset_obj = metadb.get_resource_preset_by_id(MY_CLUSTER_TYPE, resources.resource_preset_id)
            subnet_id = subcluster.get_subnet_id()  # type: Optional[str]
            if not resource_preset_obj:
                raise errors.DbaasClientError(
                    'Not found resource_preset {preset}'.format(preset=resources.resource_preset_id)
                )
            instance_metrics = generate_metric(
                resources, resource_preset_obj, subnet_id, image_id, zone_id, g.folder['folder_ext_id'], host_group_ids
            )
            for metric in instance_metrics * subcluster.get_hosts_count():
                yield metric
    except ValueError as err:
        raise errors.ParseConfigError(err)


@register_request_handler(MY_CLUSTER_TYPE, ResourceEnum.CONSOLE_CLUSTERS_CONFIG, DbaasOperation.INFO)
def get_hadoop_clusters_config(res):
    """
    Performs Hadoop specific logic for 'get_clusters_config'
    """

    res['images'] = []
    res['decommission_timeout'] = config.get_decommission_timeout_boundaries()
    res['default_resources'] = config.get_default_resources()
    res['default_version'] = get_dataproc_default_version_prefix()

    dataproc_config_with_minor_versions = get_dataproc_config_with_minor_versions(
        dataproc_images_config=get_dataproc_images_config(with_deprecated=True),
        old_compatibility_versions=res['versions'].copy(),
    )

    res['versions'] = []
    res['available_versions'] = []
    for version, image_config in dataproc_config_with_minor_versions.items():
        res['versions'].append(version)
        res['available_versions'].append(
            {
                'id': version,
                'name': version,
                'deprecated': image_config.get('deprecated', False),
                'updatableTo': [],
            }
        )

        services = []
        dependencies = []
        for service_name, service in image_config['services'].items():
            deps = ClusterService.to_enums(service.get('deps', []))
            services.append(
                {
                    'service': ClusterService.to_enum(service_name),
                    'deps': deps,
                    'default': service.get('default', False),
                    'version': service['version'],
                }
            )
            if deps:
                dependencies.append(
                    {
                        'service': ClusterService.to_enum(service_name),
                        'deps': deps,
                    }
                )

        res['images'].append(
            {
                'name': version,
                'available_services': ClusterService.to_enums(image_config['services'].keys()),
                'service_deps': dependencies,
                'min_size': image_config['imageMinSize'],
                'services': services,
            }
        )

    # temporary UI-compatibility until CLOUDFRONT-9681 is done
    res['host_count_limits']['min_host_count'] = 1
    for host_type in res['host_types']:
        resource_presets = []
        for resource_preset in host_type['resource_presets']:
            resource_presets.append(resource_preset)
        host_type['resource_presets'] = resource_presets

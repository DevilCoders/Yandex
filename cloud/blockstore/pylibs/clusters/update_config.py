from dataclasses import dataclass
from typing import Optional


@dataclass
class ClusterUpdateConfig:
    name: str

    z2_group: str
    z2_meta_group: str
    z2_conf_meta_group: str

    z2_control_group: Optional[str] = None
    z2_control_meta_group: Optional[str] = None
    z2_control_conf_meta_group: Optional[str] = None


_CLUSTERS_UPDATE_CONFIGS = {
    'blockstore': {
        'hw-nbs-dev-lab': ClusterUpdateConfig(
            name='hw-nbs-dev-lab',

            z2_group='YCLOUD_HW_NBS_DEV_LAB_NBS_GLOBAL',
            z2_meta_group='YCLOUD_HW_NBS_DEV_LAB_NBS',
            z2_conf_meta_group='YCLOUD_HW_NBS_DEV_LAB_NBS_GLOBAL_CONF'
        ),
        'hw-nbs-stable-lab': ClusterUpdateConfig(
            name='hw-nbs-stable-lab',

            z2_group='YCLOUD_HW_NBS_STABLE_LAB_NBS_GLOBAL',
            z2_meta_group='YCLOUD_HW_NBS_STABLE_LAB_NBS',
            z2_conf_meta_group='YCLOUD_HW_NBS_STABLE_LAB_NBS_GLOBAL_CONF',

            z2_control_group='YCLOUDVM_HW_NBS_STABLE_LAB_NBS_CONTROL_GLOBAL',
            z2_control_meta_group='YCLOUDVM_HW_NBS_STABLE_LAB_NBS_CONTROL',
            z2_control_conf_meta_group='YCLOUDVM_HW_NBS_STABLE_LAB_NBS_CONTROL_GLOBAL_CONF',
        ),
    },

    'filestore': {
        'hw-nbs-dev-lab': ClusterUpdateConfig(
            name='hw-nbs-dev-lab',

            z2_group='YCLOUD_HW_NBS_DEV_LAB_NFS_GLOBAL',
            z2_meta_group='YCLOUD_HW_NBS_DEV_LAB_NFS',
            z2_conf_meta_group='YCLOUD_HW_NBS_DEV_LAB_NFS_GLOBAL_CONF'
        ),
        'hw-nbs-stable-lab': ClusterUpdateConfig(
            name='hw-nbs-stable-lab',

            z2_group='YCLOUD_HW_NBS_STABLE_LAB_NFS_GLOBAL',
            z2_meta_group='YCLOUD_HW_NBS_STABLE_LAB_NFS',
            z2_conf_meta_group='YCLOUD_HW_NBS_STABLE_LAB_NFS_GLOBAL_CONF',

            z2_control_group='YCLOUDVM_HW_NBS_STABLE_LAB_NFS_CONTROL_GLOBAL',
            z2_control_meta_group='YCLOUDVM_HW_NBS_STABLE_LAB_NFS_CONTROL',
            z2_control_conf_meta_group='YCLOUDVM_HW_NBS_STABLE_LAB_NFS_CONTROL_GLOBAL_CONF',
        ),
    },

    'snapshot': {
        'hw-nbs-dev-lab': ClusterUpdateConfig(
            name='hw-nbs-dev-lab',

            z2_group='YCLOUD_HW_NBS_DEV_LAB_SNAPSHOT_GLOBAL',
            z2_meta_group='YCLOUD_HW_NBS_DEV_LAB_SNAPSHOT',
            z2_conf_meta_group=None,
        ),
        'hw-nbs-stable-lab': ClusterUpdateConfig(
            name='hw-nbs-stable-lab',

            z2_group='YCLOUDVM_HW_NBS_STABLE_LAB_SNAPSHOT_GLOBAL',
            z2_meta_group='YCLOUDVM_HW_NBS_STABLE_LAB_SNAPSHOT',
            z2_conf_meta_group=None,
        ),
    },
}


def get_cluster_update_config(cluster: str, service: str) -> ClusterUpdateConfig:
    return _CLUSTERS_UPDATE_CONFIGS[service][cluster]

"""Models shared between blueprints"""


import flask_restplus


empty_model = flask_restplus.model.Model("EmptyModel")


ipv4_config_model = flask_restplus.model.Model("Ipv4Config", {
    "addr": flask_restplus.fields.String(
        required=True, description="Ipv4 address"
    ),
    "mask": flask_restplus.fields.Integer(
        description="Net mask (number of bits)", allow_null_in_schema=True
    ),
    "gw": flask_restplus.fields.String(
        description="Gateway address", allow_null_in_schema=True
    ),
}, allow_null_in_schema=True)


ipv6_config_model = flask_restplus.model.Model("Ipv6Config", {
    "addr": flask_restplus.fields.String(
        required=True, description="Ipv4 address",
    ),
    "mask": flask_restplus.fields.Integer(
        description="Net mask (number of bits)", allow_null_in_schema=True
    ),
}, allow_null_in_schema=True)


# ==================================== HOST MODELS START ===================================
host_dynamic_config_model = flask_restplus.model.Model("HostDynamicConifg", {
    "ipv6": flask_restplus.fields.Nested(
        ipv6_config_model, allow_null=True, description="Host ipv6 config",
    ),
    "ipv4": flask_restplus.fields.Nested(
        ipv4_config_model, allow_null=True, description="Host ipv4 config"
    ),
}, allow_null_in_schema=True)


update_host_model = flask_restplus.model.Model("UpdateHostConfig", {
    "stand": flask_restplus.fields.String(
        description="Host stand", allow_null_in_schema=True
    ),
    "dynamic_config": flask_restplus.fields.Nested(
        host_dynamic_config_model, allow_null=True, description="Host dynamic config"
    )
}, allow_null_in_schema=True)


batch_update_host_model = flask_restplus.model.Model("BatchUpdateHostConfig", {
    "fqdn": flask_restplus.fields.String(
        required=True, description="Host fqdn"
    ),
    "stand": flask_restplus.fields.String(
        description="Host stand", allow_null_in_schema=True
    ),
    "dynamic_config": flask_restplus.fields.Nested(
        host_dynamic_config_model, allow_null=True, description="Host dynamic config"
    )
})


add_host_model = flask_restplus.model.Model("AddHostConfig", {
    "fqdn": flask_restplus.fields.String(
        required=True, description="Host fqdn"
    ),
    "stand": flask_restplus.fields.String(
        required=True, description="Host stand", allow_null_in_schema=True
    ),
    "dynamic_config": flask_restplus.fields.Nested(
        host_dynamic_config_model, required=True, description="Host dynamic config"
    )
}, allow_null_in_schema=True)


batch_delete_host_model = flask_restplus.model.Model("BatchDeleteHostConfig", {
    "fqdns": flask_restplus.fields.List(
        flask_restplus.fields.String, required=True, min_items=1, description="List of hosts to delete",
    ),
})


host_model = flask_restplus.model.Model("HostConfig", {
    "id": flask_restplus.fields.Integer(
        required=True, description="Unique host id, allocated automatically",
    ),
    "fqdn": flask_restplus.fields.String(
        required=True, description="Host fqdn"
    ),
    "stand": flask_restplus.fields.String(
        description="Host stand", allow_null_in_schema=True
    ),
    "dynamic_config": flask_restplus.fields.Nested(
        host_dynamic_config_model, allow_null=True, description="Host dynamic config"
    )
})
# ==================================== HOST MODELS END ===================================


# ==================================== SVM MODELS START ===================================
svm_dynamic_config_model = flask_restplus.model.Model("SvmDynamicConifg", {
    "ipv6": flask_restplus.fields.Nested(
        ipv6_config_model, allow_null=True, description="Svm ipv6 config",
    ),
    "ipv4": flask_restplus.fields.Nested(
        ipv4_config_model, allow_null=True, description="Svm ipv4 config"
    ),
    "placement": flask_restplus.fields.String(
        description="Svm placement", allow_null_in_schema=True
    ),
}, allow_null_in_schema=True)


update_svm_model = flask_restplus.model.Model("UpdateSvmConfig", {
    "stand": flask_restplus.fields.String(
        description="Host stand", allow_null_in_schema=True
    ),
    "dynamic_config": flask_restplus.fields.Nested(
        svm_dynamic_config_model, allow_null=True, description="Svm dynamic config"
    )
}, allow_null_in_schema=True)


batch_update_svm_model = flask_restplus.model.Model("BatchUpdateSvmConfig", {
    "fqdn": flask_restplus.fields.String(
        required=True, description="Svm fqdn"
    ),
    "stand": flask_restplus.fields.String(
        description="Host stand", allow_null_in_schema=True
    ),
    "dynamic_config": flask_restplus.fields.Nested(
        svm_dynamic_config_model, allow_null=True, description="Svm dynamic config"
    )
})


add_svm_model = flask_restplus.model.Model("AddSvmConfig", {
    "fqdn": flask_restplus.fields.String(
        required=True, description="Svm fqdn"
    ),
    "stand": flask_restplus.fields.String(
        required=True, description="Host stand", allow_null_in_schema=True
    ),
    "dynamic_config": flask_restplus.fields.Nested(
        svm_dynamic_config_model, required=True, description="Svm dynamic config"
    )
}, allow_null_in_schema=True)


batch_delete_svm_model = flask_restplus.model.Model("BatchDeleteSvmConfig", {
    "fqdns": flask_restplus.fields.List(
        flask_restplus.fields.String, required=True, min_items=1, description="List of svms to delete",
    ),
})


svm_model = flask_restplus.model.Model("SvmConfig", {
    "id": flask_restplus.fields.Integer(
        required=True, description="Unique Svm id, allocated automatically",
    ),
    "fqdn": flask_restplus.fields.String(
        required=True, description="Svm fqdn"
    ),
    "stand": flask_restplus.fields.String(
        required=True, description="Host stand", allow_null_in_schema=True
    ),
    "dynamic_config": flask_restplus.fields.Nested(
        svm_dynamic_config_model, allow_null=True, description="Svm dynamic config"
    )
})
# ===================================== SVM MODELS END ====================================


# =============================== HOSTS AND SVM MODELS START ==============================
hosts_and_svms_model = flask_restplus.model.Model("HostsAndSvmsConfig", {
    "hosts": flask_restplus.fields.List(
        flask_restplus.fields.Nested(host_model, allow_null=True),
        required=True, description="List of hosts",
    ),
    "svms": flask_restplus.fields.List(
        flask_restplus.fields.Nested(svm_model, allow_null=True),
        required=True, description="List of svms",
    ),
    "cluster_configs_version": flask_restplus.fields.Integer(
        required=True, description="Host configs verison (autoincremented on all hosts/svms updates)",
    ),
})
# ================================ HOSTS AND SVM MODELS END ===============================


# ===================================== LOCK MODELS START =================================
add_lock_model = flask_restplus.model.Model("AddLockConfig", {
    "hosts": flask_restplus.fields.List(
        flask_restplus.fields.String, required=True, min_items=1, description="List of host names, locked by lock.",
    ),
    "description": flask_restplus.fields.String(
        required=True, description="Lock description. Could include yc-bootstrap command, which got this lock.",
    ),
    "hb_timeout": flask_restplus.fields.Integer(
        required=True, min=1, description=(
            "Hearbeat timeout (in seconds). Lock should be extended periodically to avoid stale locks cleanup."
        ),
    ),
})


lock_model = flask_restplus.model.Model.inherit("LockConfig", add_lock_model, {
    "id": flask_restplus.fields.Integer(
        required=True, description="Lock id, allocated automatically. Any two different locks have different ids.",
    ),
    "owner": flask_restplus.fields.String(
        required=True, description="Lock owner."
    ),
    "expired_at": flask_restplus.fields.String(
        required=True, description="Lock expiration time."
    ),
})
# ===================================== LOCK MODELS END ====================================


# =================================== STAND MODELS START ====================================
add_stand_model = flask_restplus.model.Model("AddStandConfig", {
    "name": flask_restplus.fields.String(
        required=True, description="Stand name",
    ),
}, allow_null_in_schema=True)


stand_model = flask_restplus.model.Model("StandConfig", {
    "id": flask_restplus.fields.Integer(
        required=True, description="Stand id, allocated automatically. Any two different stands have different ids.",
    ),
    "name": flask_restplus.fields.String(
        required=True, description="Stand name",
    ),
})

# ===================================== STAND MODELS END ====================================


# ============================= STAND CLUSTER_MAP MODELS START ==============================
stand_cluster_map_model = flask_restplus.model.Model("StandClusterMapConfig", {
    "id": flask_restplus.fields.Integer(
        required=True, description="Id, allocated automatically.",
    ),
    "stand": flask_restplus.fields.String(
        required=True, description="Stand name",
    ),
    "grains":  flask_restplus.fields.Raw(
        required=True, description="Stand cluster_map", allow_null_in_schema=True
    ),
    "cluster_configs_version": flask_restplus.fields.Integer(
        required=True, description="Host configs version, incremented on every change of host configs.",
        allow_null_in_schema=True,
    ),
    "yc_ci_version": flask_restplus.fields.String(
        required=True, description="Yc-ci version, used to generate grains (commit hash).",
        allow_null_in_schema=True,
    ),
    "bootstrap_templates_version": flask_restplus.fields.String(
        required=True, description="Bootstrap-templates version, used to generate grains (commit hash).",
        allow_null_in_schema=True,
    ),
})

stand_cluster_map_version_model = flask_restplus.model.Model("StandClusterMapVersionConfig", {
    "cluster_configs_version": flask_restplus.fields.Integer(
        required=True, description="Host configs version, incremented on every change of host configs.",
        allow_null_in_schema=True,
    ),
    "yc_ci_version": flask_restplus.fields.String(
        required=True, description="Yc-ci version, used to generate grains (commit hash).",
        allow_null_in_schema=True,
    ),
    "bootstrap_templates_version": flask_restplus.fields.String(
        required=True, description="Bootstrap-templates version, used to generate grains (commit hash).",
        allow_null_in_schema=True,
    ),
})

update_stand_cluster_map_model = flask_restplus.model.Model("UpdateStandClusterMapConfig", {
    "grains":  flask_restplus.fields.Raw(
        description="Stand cluster_map", allow_null_in_schema=True
    ),
    "cluster_configs_version": flask_restplus.fields.Integer(
        description="Host configs version, incremented on every change of host configs.",
        allow_null_in_schema=True,
    ),
    "yc_ci_version": flask_restplus.fields.String(
        description="Yc-ci version, used to generate grains (commit hash).",
        allow_null_in_schema=True,
    ),
    "bootstrap_templates_version": flask_restplus.fields.String(
        description="Bootstrap-templates version, used to generate grains (commit hash).",
        allow_null_in_schema=True,
    ),
})
# ============================== STAND CLUSTER_MAP MODELS END ===============================


# =============================== INSTANCE GROUPS MODELS START ==============================
add_instance_group_model = flask_restplus.model.Model("AddInstanceGroupConfig", {
    "name": flask_restplus.fields.String(
        required=True, description="Instance Group name",
    ),
}, allow_null_in_schema=True)

instance_group_model = flask_restplus.model.Model("InstanceGroupConfig", {
    "id": flask_restplus.fields.Integer(
        required=True, description=("Instance Group id, allocated automatically. Any two different instance groups "
                                    "have different ids."),
    ),
    "name": flask_restplus.fields.String(
        required=True, description="Instance Group name",
    ),
    "stand": flask_restplus.fields.String(
        required=True, description="Instance Group stand"
    ),
})
# ================================ INSTANCE GROUPS MODELS END ===============================

# =========================== INSTANCE GROUP RELEASES MODELS START ==========================
put_instance_group_release_model = flask_restplus.model.Model("AddInstanceGroupReleaseConfig", {
    "url": flask_restplus.fields.String(
        required=True, description="S3 url to download image from",
        allow_null_in_schema=True,
    ),
    "image_id": flask_restplus.fields.String(
        required=True, description="Image id in corresponding stand (if image is already uploaded)",
        allow_null_in_schema=True,
    ),
}, allow_null_in_schema=True)

instance_group_release_model = flask_restplus.model.Model("InstanceGroupReleaseConfig", {
    "id": flask_restplus.fields.Integer(
        required=True, description="Instance Group release id, allocated automatically",
    ),
    "url": flask_restplus.fields.String(
        required=True, description="S3 url to download image from",
    ),
    "image_id": flask_restplus.fields.String(
        required=True, description="Image id in corresponding stand (if image is already uploaded)",
        allow_null_in_schema=True,
    ),
    "stand": flask_restplus.fields.String(
        required=True, description="Stand name"
    ),
    "instance_group": flask_restplus.fields.String(
        required=True, description="Instance Group name"
    ),
})
# ============================ INSTANCE GROUP RELEASES MODELS END ===========================

# ================================== SALT ROLE MODEL START ==================================
add_salt_role_model = flask_restplus.model.Model("AddSaltRoleConfig", {
    "name": flask_restplus.fields.String(
        required=True, description="Salt role name",
    ),
})

salt_role_model = flask_restplus.model.Model("SaltRoleConfig", {
    "id": flask_restplus.fields.Integer(
        required=True, description="Id, allocated automatically",
    ),
    "name": flask_restplus.fields.String(
        required=True, description="Salt role name",
    ),
})
# =================================== SALT ROLE MODEL END ===================================

# ============================== SALT ROLE RELEASES MODEL START =============================
instance_salt_role_package_model = flask_restplus.model.Model("InstanceSaltRolePackageConfig", {
    "id": flask_restplus.fields.Integer(
        required=True, description="Id, allocated automatically",
    ),
    "salt_role": flask_restplus.fields.String(
        required=True, description="Salt role",
    ),
    "package_name": flask_restplus.fields.String(
        required=True, description="Package name (yc-salt-formula or any other package from cluster-configs)",
    ),
    "target_version": flask_restplus.fields.String(
        required=True, description="Package version",
    )
})

update_instance_salt_role_package_model = flask_restplus.model.Model("UpdateInstanceSaltRolePackagesModel", {
    "salt_role": flask_restplus.fields.String(
        required=True, description="Salt role",
    ),
    "package_name": flask_restplus.fields.String(
        required=True, description="Package name (yc-salt-formula or any other package from cluster-configs)",
    ),
    "target_version": flask_restplus.fields.String(
        required=True, description="Package version",
    ),
})
# =============================== SALT ROLE RELEASES MODEL END ==============================

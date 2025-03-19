locals {
    subnets = {
        vla = "bucpba0hulgrkgpd58qp"
        sas = "bltueujt22oqg5fod2se"
        myt = "fo27jfhs8sfn4u51ak2s"
    }

    domain = "ycp.cloud-preprod.yandex.net"
    ig_size = 3

    # https://c.yandex-team.ru/groups/cloud_preprod_api-gateway_tf
    conductor_group = "api-gateway_tf"
    osquery_tag = "ycloud-svc-api_gateway"

    instance_config = {
        platform_id = "standard-v2"
        cores = 2
        core_fraction = 20
        memory = 4
        boot_disk_size = 10
        secondary_disk_size = 32
        boot_disk_type = "network-hdd"
        secondary_disk_type = "network-ssd"
    }

    labels = {
        environment = "preprod"
        env = "pre-prod"
    }

    blue_group = {
        name = "Blue"
        target_group_id   = module.group_blue.target_group_id
        instance_group_id = module.group_blue.instance_group_id
    }

    green_group = {
        name = "Green"
        target_group_id   = module.group_green.target_group_id
        instance_group_id = module.group_green.instance_group_id
    }
}

module "group_blue" {
    source = "../../modules/gateway_instance_group"
    folder_id = var.yc_folder
    instance_sa_id = ycp_iam_service_account.gateway_instance_sa.id
    ig_sa_id = ycp_iam_service_account.gateway_ig_sa.id

    subnets = local.subnets

    instance = local.instance_config
    domain = local.domain
    conductor_group = local.conductor_group
    conductor_role = "blue"
    labels = local.labels

    ig_size = local.ig_size
    name = "api-gateway-blue-preprod"
    description = "API Gateway Blue"
    image_id = local.blue_image_id
    extra_instance_metadata = local.metadata_files_blue
}

module "group_green" {
    source = "../../modules/gateway_instance_group"
    folder_id = var.yc_folder
    instance_sa_id = ycp_iam_service_account.gateway_instance_sa.id
    ig_sa_id = ycp_iam_service_account.gateway_ig_sa.id

    subnets = local.subnets

    instance = local.instance_config
    domain = local.domain
    conductor_group = local.conductor_group
    conductor_role = "green"
    labels = local.labels

    ig_size = local.ig_size
    name = "api-gateway-green-preprod"
    description = "API Gateway Green"
    image_id = local.green_image_id
    extra_instance_metadata = local.metadata_files_green
}

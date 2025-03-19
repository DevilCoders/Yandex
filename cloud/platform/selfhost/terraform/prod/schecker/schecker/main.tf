terraform {
  required_providers {
    ycp = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
    }
  }
  required_version = ">= 0.13"
}

module "common" {
  source = "../common"
}

provider "ycp" {
  prod        = false
  ycp_profile = module.common.ycp_profile
  folder_id   = var.yc_folder
  zone        = module.common.yc_zone
}

data "template_file" "docker_json" {
  template = file("${path.module}/../common/files/docker.tpl.json")

  vars = {
    docker_auth = module.lockbox-secret-sa-yc-schecker-cr.secret
  }
}

data "template_file" "infra_configs" {
  template = file("${path.module}/files/infra-configs.tpl")

  vars = {
    push_client_conf    = file("${path.module}/files/push-client/push-client.yaml")
    push_client_iam_key = module.lockbox-logbroker-iam-key.secret
  }
}

data "template_file" "configs" {
  template = file("${path.module}/files/schecker/configs.tpl")
  count    = var.yc_instance_group_size
}

data "template_file" "podmanifest" {
  template = file("${path.module}/files/podmanifest.tpl.yaml")
  count    = var.yc_instance_group_size

  vars = {
    config_digest          = sha256(element(data.template_file.configs.*.rendered, count.index))
    infra_config_digest    = sha256(data.template_file.infra_configs.rendered)
    metadata_version       = var.metadata_image_version
    api_version            = module.common.api_version
    parser_version         = module.common.parser_version
    swiss_knife_version    = module.common.swiss_knife_version
    syncer_version         = module.common.syncer_version
    push_client_version    = module.common.push-client_image_version
    schecker_db_host       = var.yc_schecker_db_connection.host
    schecker_db_port       = var.yc_schecker_db_connection.port
    schecker_db_name       = var.yc_schecker_db_connection.name
    schecker_db_user       = module.lockbox-secret-yc-schecker-db-user.secret
    schecker_splunk_user   = module.lockbox-secret-yc-schecker-splunk-username.secret
    schecker_smtp_user     = module.lockbox-secret-yc-schecker-smtp-user.secret
    schecker_conductor_url = var.yc_schecker_conductor_url
  }
}

module "schecker-instance-group" {
  source          = "../../../modules/kubelet_instance_group_ycp_v2"
  name_prefix     = "schecker"
  hostname_prefix = "schecker"
  hostname_suffix = module.common.hostname_suffix
  role_name       = "schecker"
  osquery_tag     = module.common.osquery_tag

  zones = var.custom_yc_zones

  instance_group_size = var.yc_instance_group_size

  instance_platform_id = module.common.instance_platform_id
  cores_per_instance   = var.instance_cores
  memory_per_instance  = var.instance_memory
  disk_per_instance    = var.instance_disk_size
  disk_type            = var.instance_disk_type
  image_id             = module.common.overlay_image_id

  configs              = data.template_file.configs.*.rendered
  infra-configs        = data.template_file.infra_configs.rendered
  podmanifest          = data.template_file.podmanifest.*.rendered
  docker-config        = data.template_file.docker_json.rendered
  ssh-keys             = module.ssh-keys.ssh-keys
  skip_update_ssh_keys = "false"

  metadata = {
    user-data                    = file("${path.module}/files/cloud-init.yaml")
    selfdns-token                = module.yav-selfdns-token.secret
    schecker-db-password         = module.lockbox-secret-yc-schecker-db-password.secret
    schecker-splunk-password     = module.lockbox-secret-yc-schecker-splunk-password.secret
    schecker-api-token           = module.lockbox-secret-yc-schecker-api-token.secret
    star-trek-oauth-token        = module.lockbox-secret-yc-schecker-startrek-token.secret
    schecker-smtp-password       = module.lockbox-secret-yc-schecker-smtp-password.secret
    schecker-clickhouse-password = module.lockbox-secret-yc-schecker-clickhouse-password.secret
  }

  labels = module.common.instance_labels

  subnets        = var.subnets
  ipv4_addresses = var.ipv4_addresses
  ipv6_addresses = var.ipv6_addresses
  underlay       = false

  service_account_id = var.service_account_id
  host_group         = module.common.host_group
  security_group_ids = var.security_group_ids
}

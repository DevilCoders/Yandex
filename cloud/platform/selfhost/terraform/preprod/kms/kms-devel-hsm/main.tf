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
  prod      = var.ycp_prod
  token     = var.yc_token
  folder_id = var.yc_folder
  zone      = var.yc_zone
}

module "ssh-keys" {
  source       = "../../../modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = module.common.abc_group.abc_service
  abc_service_scopes = module.common.abc_group.abc_service_scopes
}

// selfdns token
module "yav-selfdns-token" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01dsnmbfh6kbepcsyg6npmyyg5"
  value_name = "selfdns-oauth"
}

module "kms-devel-hsm-instance-group" {
  source          = "../../../modules/kubelet_instance_group_ycp_v2"
  name_prefix     = "kms-devel-hsm"
  hostname_prefix = "kms-devel-hsm"
  hostname_suffix = var.hostname_suffix
  role_name       = "kms-devel-hsm"
  osquery_tag     = "ycloud-svc-kms"

  zones = var.yc_zones

  instance_group_size = var.yc_instance_group_size

  instance_platform_id = var.instance_platform_id
  cores_per_instance   = var.instance_cores
  memory_per_instance  = var.instance_memory
  disk_per_instance    = var.instance_disk_size
  disk_type            = var.instance_disk_type
  image_id             = var.image_id

  configs              = ["", "", ""]
  infra-configs        = ""
  podmanifest          = ["", "", ""]
  docker-config        = ""
  ssh-keys             = module.ssh-keys.ssh-keys
  skip_update_ssh_keys = "false"

  metadata = {
    selfdns-token = module.yav-selfdns-token.secret
  }

  labels = {
    layer   = "paas"
    abc_svc = "yckms"
    env     = "pre-prod"
  }

  subnets        = var.subnets
  ipv4_addresses = var.ipv4_addresses
  ipv6_addresses = var.ipv6_addresses
  underlay       = false

  service_account_id = var.service_account_id
  host_group         = var.host_group
  security_group_ids = var.security_group_ids
}

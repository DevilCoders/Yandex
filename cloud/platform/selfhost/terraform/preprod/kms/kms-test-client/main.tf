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

module "ssh-keys" {
  source       = "../../../modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = module.common.abc_group.abc_service
  abc_service_scopes = module.common.abc_group.abc_service_scopes
}

module "kms-test-instance-group" {
  source          = "../../../modules/kubelet_instance_group_ycp_v2"
  name_prefix     = "kms-test"
  hostname_prefix = "kms-test"
  hostname_suffix = module.common.hostname_suffix
  role_name       = "kms-test"
  osquery_tag     = module.common.osquery_tag

  zones = module.common.yc_zones

  instance_group_size = var.yc_instance_group_size

  instance_platform_id = module.common.instance_platform_id
  cores_per_instance   = var.instance_cores
  memory_per_instance  = var.instance_memory
  disk_per_instance    = var.instance_disk_size
  disk_type            = var.instance_disk_type
  image_id             = module.common.underlay_image_id

  configs              = ["", "", ""]
  infra-configs        = ""
  podmanifest          = ["", "", ""]
  docker-config        = ""
  ssh-keys             = module.ssh-keys.ssh-keys
  skip_update_ssh_keys = "false"

  metadata = {}

  labels = module.common.instance_labels

  subnets        = {}
  ipv4_addresses = []
  ipv6_addresses = []
  underlay       = true

  service_account_id = var.service_account_id
}

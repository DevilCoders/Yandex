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
  folder_id   = module.common.folder_id
  zone        = module.common.yc_zone
}

module "ssh-keys" {
  source             = "../../../modules/ssh-keys"
  yandex_token       = var.yandex_token
  abc_service        = module.common.abc_group.abc_service
  abc_service_scopes = module.common.abc_group.abc_service_scopes
}

data "template_file" "application_yaml" {
  template = file("${path.module}/../kms-control-plane/files/kms/application.tpl.yaml")
  count    = var.instance_group_size

  vars = {
    private_api_port = 9443
    id_prefix        = var.id_prefix
  }
}

data "template_file" "run_tool_sh" {
  template = file("${path.module}/files/kms/run-tool.sh")

  vars = {
    tool_version = module.common.tool_version
  }
}

data "template_file" "configs" {
  template = file("${path.module}/files/kms/configs.tpl")
  count    = var.instance_group_size

  vars = {
    application_yaml        = element(data.template_file.application_yaml.*.rendered, count.index)
    yandex_internal_root_ca = file("${path.module}/../../../common/allCAs.pem")
    run_tool_sh             = data.template_file.run_tool_sh.rendered
    run_ydb_sh              = file("${path.module}/files/kms/run-ydb.sh")
  }
}

data "template_file" "podmanifest" {
  template = file("${path.module}/files/podmanifest.tpl.yaml")
  count    = var.instance_group_size

  vars = {
    config_digest    = sha256(element(data.template_file.configs.*.rendered, count.index))
    metadata_version = module.common.metadata_image_version
    tool_version     = module.common.tool_version
  }
}

module "kms-tool-instance-group" {
  source               = "../../../modules/kubelet_instance_group_ycp_v2"
  name_prefix          = var.hostname_prefix
  hostname_prefix      = var.hostname_prefix
  hostname_suffix      = module.common.hostname_suffix
  fqdn_hostname_suffix = ""
  role_name            = var.role_label
  osquery_tag          = module.common.osquery_tag

  zones                = module.common.yc_zones
  host_suffix_for_zone = module.common.yc_zone_suffix
  dns_zone_id          = module.common.dns_zone_id

  instance_group_size = var.instance_group_size

  cores_per_instance   = var.instance_cores
  memory_per_instance  = var.instance_memory
  disk_per_instance    = var.instance_disk_size
  disk_type            = var.instance_disk_type
  image_id             = module.common.overlay_image_id
  instance_platform_id = module.common.instance_platform_id

  configs              = data.template_file.configs.*.rendered
  podmanifest          = data.template_file.podmanifest.*.rendered
  ssh-keys             = module.ssh-keys.ssh-keys
  skip_update_ssh_keys = "false"

  metadata = {
    user-data = module.common.cloud-init-sh
    skm       = file("${path.module}/files/skm/bundle.yaml")
  }

  labels = module.common.instance_labels

  subnets        = module.common.subnets
  ipv4_addresses = []
  ipv6_addresses = []
  underlay       = false

  service_account_id = module.common.service_account_id
}

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
  abc_service  = module.common.abc_group
}

# cr.yandex auth token
module "yav-secret-sa-yc-kms-cr" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01ekn6vxw8gmkt7hrxtmk4ynvh"
  value_name = "docker_auth"
}

// selfdns token
module "yav-selfdns-token" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01dsnmbfh6kbepcsyg6npmyyg5"
  value_name = "selfdns-oauth"
}

data "template_file" "docker_json" {
  template = file("${path.module}/../common/files/docker.tpl.json")

  vars = {
    docker_auth = module.yav-secret-sa-yc-kms-cr.secret
  }
}

data "template_file" "application_yaml" {
  template = file("${path.module}/../kms-control-plane/files/kms/application.tpl.yaml")
  count    = var.yc_instance_group_size

  vars = {
    zone = module.common.yc_zone_suffix[element(module.common.yc_zones, count.index % length(module.common.yc_zones))]
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
  count    = var.yc_instance_group_size

  vars = {
    application_yaml        = element(data.template_file.application_yaml.*.rendered, count.index)
    yandex_internal_root_ca = file("${path.module}/../../../common/allCAs.pem")
    run_tool_sh             = data.template_file.run_tool_sh.rendered
    run_ydb_sh              = file("${path.module}/files/kms/run-ydb.sh")
  }
}

data "template_file" "podmanifest" {
  template = file("${path.module}/files/podmanifest.tpl.yaml")
  count    = var.yc_instance_group_size

  vars = {
    config_digest    = sha256(element(data.template_file.configs.*.rendered, count.index))
    metadata_version = module.common.metadata_image_version
    tool_version     = module.common.tool_version
  }
}

module "kms-tool-instance-group" {
  source          = "../../../modules/kubelet_instance_group_ycp_v2"
  name_prefix     = "kms-tool"
  hostname_prefix = "kms-tool"
  hostname_suffix = module.common.hostname_suffix
  role_name       = "kms-tool"
  osquery_tag     = module.common.osquery_tag

  zones = module.common.yc_zones

  instance_group_size = var.yc_instance_group_size

  cores_per_instance  = var.instance_cores
  memory_per_instance = var.instance_memory
  disk_per_instance   = var.instance_disk_size
  disk_type           = var.instance_disk_type
  image_id            = module.common.overlay_image_id

  configs              = data.template_file.configs.*.rendered
  podmanifest          = data.template_file.podmanifest.*.rendered
  docker-config        = data.template_file.docker_json.rendered
  ssh-keys             = module.ssh-keys.ssh-keys
  skip_update_ssh_keys = "false"

  metadata = {
    selfdns-token = module.yav-selfdns-token.secret
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

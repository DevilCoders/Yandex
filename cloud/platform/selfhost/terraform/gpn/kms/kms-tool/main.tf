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

# selfdns token for private-gpn
module "yav-selfdns-token" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01ecb30bn4m42dtxvxf9m5g5dg"
  value_name = "selfdns-token"
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
    private_api_port = 0 # Fake, we do not start the server.
    instance_num     = count.index + 1
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
    application_yaml = element(data.template_file.application_yaml.*.rendered, count.index)
    run_tool_sh      = data.template_file.run_tool_sh.rendered
    run_ydb_sh       = file("${path.module}/files/kms/run-ydb.sh")
  }
}

data "template_file" "podmanifest" {
  template = file("${path.module}/files/podmanifest.tpl.yaml")
  count    = var.yc_instance_group_size

  vars = {
    config_digest    = sha256(element(data.template_file.configs.*.rendered, count.index))
    metadata_version = module.common.metadata_image_version
    tool_version     = module.common.tool_version
    envoy_version    = module.common.envoy_image_version
  }
}

module "kms-tool-instance-group" {
  source          = "../../../modules/kubelet_instance_group_ycp_v2"
  name_prefix     = "kms-tool"
  hostname_prefix = "kms-tool"
  hostname_suffix = module.common.hostname_suffix
  role_name       = "kms-tool"
  osquery_tag     = module.common.osquery_tag

  zones                = module.common.yc_zones
  host_suffix_for_zone = module.common.yc_zone_suffix

  instance_group_size = var.yc_instance_group_size

  instance_platform_id = module.common.instance_platform_id
  cores_per_instance   = var.instance_cores
  memory_per_instance  = var.instance_memory
  disk_per_instance    = var.instance_disk_size
  disk_type            = var.instance_disk_type
  image_id             = module.common.overlay_image_id

  configs              = data.template_file.configs.*.rendered
  podmanifest          = data.template_file.podmanifest.*.rendered
  docker-config        = data.template_file.docker_json.rendered
  ssh-keys             = module.ssh-keys.ssh-keys
  skip_update_ssh_keys = "false"

  metadata = {
    selfdns-token = module.yav-selfdns-token.secret
    user-data     = file("${path.module}/../common/files/cloud-init.sh")
  }

  labels = module.common.instance_labels

  subnets        = var.subnets
  ipv4_addresses = var.ipv4_addresses
  ipv6_addresses = var.ipv6_addresses

  service_account_id = var.service_account_id
}

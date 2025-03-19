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

// TVM secret for logbroker.yandex.net
module "yav-secret-yckms-tvm" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01dq7mgdtspb6q82pr1jbmvw17"
  value_name = "client_secret"
}

// yc-kms/backup/sa-backup access key
module "yav-secret-sa-backup-access-key" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01e25y33avr14b0rbq2166g6f9"
  value_name = "access_key"
}

// yc-kms/backup/sa-backup secret key
module "yav-secret-sa-backup-secret-key" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01e25y33avr14b0rbq2166g6f9"
  value_name = "secret_key"
}

// robot-yc-kms internal s3-mds access key id
module "yav-secret-mds-s3-access-key" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01dbk6f0m1ev93vqkfd8d2ztnc"
  value_name = "s3_mds_access_key_id"
}

// robot-yc-kms internal s3-mds secret key
module "yav-secret-mds-s3-secret-key" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01dbk6f0m1ev93vqkfd8d2ztnc"
  value_name = "s3_mds_access_secret_key"
}

// @robot-yc-kms OAuth token
module "yav-secret-solomon-oauth-token" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01e20rzyzqjy8agpdsqfrvzs92"
  value_name = "oauth_token"
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
  template = file("${path.module}/files/kms/application.tpl.yaml")
  count    = var.yc_instance_group_size

  vars = {
    zone = module.common.yc_zone_suffix[element(module.common.yc_zones, count.index % length(module.common.yc_zones))]
  }
}

data "template_file" "configs" {
  template = file("${path.module}/files/kms/configs.tpl")
  count    = var.yc_instance_group_size

  vars = {
    application_yaml  = element(data.template_file.application_yaml.*.rendered, count.index)
    run_ydb_dumper_sh = file("${path.module}/files/kms/run-ydb-dumper.sh")
  }
}

data "template_file" "infra_configs" {
  template = file("${path.module}/files/kms/infra-configs.tpl")

  vars = {
    push_client_conf = file("${path.module}/files/push-client/push-client.yaml")
  }
}

data "template_file" "podmanifest" {
  template = file("${path.module}/files/kms/podmanifest.tpl.yaml")
  count    = var.yc_instance_group_size

  vars = {
    config_digest          = sha256(element(data.template_file.configs.*.rendered, count.index))
    infra_config_digest    = sha256(data.template_file.infra_configs.rendered)
    metadata_version       = module.common.metadata_image_version
    push_client_version    = module.common.push-client_image_version
    push_client_tvm_secret = module.yav-secret-yckms-tvm.secret
    application_version    = module.common.ydb-dumper_version
    cloud_s3_access_key    = module.yav-secret-sa-backup-access-key.secret
    cloud_s3_secret_key    = module.yav-secret-sa-backup-secret-key.secret
    mds_s3_access_key      = module.yav-secret-mds-s3-access-key.secret
    mds_s3_secret_key      = module.yav-secret-mds-s3-secret-key.secret
    solomon_oauth_token    = module.yav-secret-solomon-oauth-token.secret
  }
}

module "kms-ydb-dumper-instance-group" {
  source          = "../../../modules/kubelet_instance_group_ycp_v2"
  name_prefix     = "kms-ydb-dumper"
  hostname_prefix = "kms-ydb-dumper"
  hostname_suffix = module.common.hostname_suffix
  role_name       = "kms-ydb-dumper"
  osquery_tag     = module.common.osquery_tag

  zones = module.common.yc_zones

  instance_group_size = var.yc_instance_group_size

  cores_per_instance  = var.instance_cores
  memory_per_instance = var.instance_memory
  disk_per_instance   = var.instance_disk_size
  disk_type           = var.instance_disk_type
  image_id            = module.common.overlay_image_id

  configs              = data.template_file.configs.*.rendered
  infra-configs        = data.template_file.infra_configs.rendered
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
  host_group         = var.custom_host_group
}

terraform {
  required_providers {
    ycp = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
    }
  }
  required_version = ">= 0.13"
}

provider "ycp" {
  prod        = false
  ycp_profile = var.ycp_profile
  folder_id   = var.yc_folder
  zone        = var.yc_zone
}

module "ssh-keys" {
  source       = "../../../modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = "yckms"
}

// sa-yc-lockbox-cr cr.yandex auth token
module "yav-secret-sa-yc-kms-cr" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01et7h3wmcxqkqf50qpr6zz6cn"
  value_name = "docker_auth"
}

// TVM secret for logbroker.yandex.net
module "yav-secret-yckms-tvm" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01dq7mgdtspb6q82pr1jbmvw17"
  value_name = "client_secret"
}

// yc-lockbox/backup/backup access key
module "yav-secret-sa-backup-access-key" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01eskqdx41xfmvwztp6en7ghah"
  value_name = "access_key"
}

// yc-lockbox/backup/backup secret key
module "yav-secret-sa-backup-secret-key" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01eskqdx41xfmvwztp6en7ghah"
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

data "template_file" "docker_json" {
  template = file("${path.module}/../common/docker.tpl.json")

  vars = {
    docker_auth = module.yav-secret-sa-yc-kms-cr.secret
  }
}

data "template_file" "application_yaml" {
  template = file("${path.module}/files/lockbox/application.tpl.yaml")
  count    = var.yc_instance_group_size
}

data "template_file" "configs" {
  template = file("${path.module}/files/lockbox/configs.tpl")
  count    = var.yc_instance_group_size

  vars = {
    application_yaml  = element(data.template_file.application_yaml.*.rendered, count.index)
    run_ydb_dumper_sh = file("${path.module}/files/lockbox/run-ydb-dumper.sh")
  }
}

data "template_file" "infra_configs" {
  template = file("${path.module}/files/lockbox/infra-configs.tpl")

  vars = {
    push_client_yc_logbroker_conf = file("${path.module}/files/push-client/push-client-yc-logbroker.yaml")
  }
}

data "template_file" "podmanifest" {
  template = file("${path.module}/files/lockbox/podmanifest.tpl.yaml")
  count    = var.yc_instance_group_size

  vars = {
    config_digest          = sha256(element(data.template_file.configs.*.rendered, count.index))
    infra_config_digest    = sha256(data.template_file.infra_configs.rendered)
    metadata_version       = var.metadata_image_version
    push_client_version    = var.push-client_image_version
    push_client_tvm_secret = module.yav-secret-yckms-tvm.secret
    application_version    = var.application_version
    cloud_s3_access_key    = module.yav-secret-sa-backup-access-key.secret
    cloud_s3_secret_key    = module.yav-secret-sa-backup-secret-key.secret
    mds_s3_access_key      = module.yav-secret-mds-s3-access-key.secret
    mds_s3_secret_key      = module.yav-secret-mds-s3-secret-key.secret
    solomon_oauth_token    = module.yav-secret-solomon-oauth-token.secret
  }
}

module "lockbox-ydb-dumper-instance-group" {
  source          = "../../../modules/kubelet_instance_group_ycp_v2"
  name_prefix     = "lockbox-ydb-dumper"
  hostname_prefix = "lockbox-ydb-dumper"
  hostname_suffix = var.hostname_suffix
  role_name       = "lockbox-ydb-dumper"
  osquery_tag     = "ycloud-svc-lockbox"

  zones = var.yc_zones

  instance_group_size = var.yc_instance_group_size

  instance_platform_id = var.instance_platform_id
  cores_per_instance   = var.instance_cores
  memory_per_instance  = var.instance_memory
  disk_per_instance    = var.instance_disk_size
  disk_type            = var.instance_disk_type
  image_id             = var.image_id

  configs              = data.template_file.configs.*.rendered
  infra-configs        = data.template_file.infra_configs.rendered
  podmanifest          = data.template_file.podmanifest.*.rendered
  docker-config        = data.template_file.docker_json.rendered
  ssh-keys             = module.ssh-keys.ssh-keys
  skip_update_ssh_keys = "false"

  metadata = {
    skm = file("${path.module}/files/skm/bundle.yaml")
  }

  labels = {
    layer   = "paas"
    abc_svc = "yckms"
    env     = "prod"
  }

  subnets        = var.subnets
  ipv4_addresses = var.ipv4_addresses
  ipv6_addresses = var.ipv6_addresses
  underlay       = false

  service_account_id = var.service_account_id
  host_group         = var.host_group
  security_group_ids = var.security_group_ids
}

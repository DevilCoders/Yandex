terraform {
  required_providers {
    ycp = {
      source = "terraform.storage.cloud-preprod.yandex.net/yandex-cloud/ycp"
      # version = "0.20.0" # optional
    }
  }
  required_version = ">= 0.13"
}

provider "ycp" {
  prod        = true
  ycp_profile = var.ycp_profile
  folder_id   = var.yc_folder
  zone        = var.yc_zone
}

module "ssh-keys" {
  source       = "../../../modules/ssh-keys"
  yandex_token = var.yandex_token
  abc_service  = "yckms"
}

//sa-yc-crypto-e2e-monitoring-cr cr.yandex auth token
module "lockbox-secret-sa-yc-crypto-e2e-monitoring-cr" {
  source     = "../../../modules/lockbox-secret"
  yc_profile = "prod"
  id         = "e6qn6e9upsfkdb0r7kvb"
  value_key  = "docker_auth"
}

// TVM secret for logbroker.yandex.net
module "yav-secret-yckms-tvm" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01dq7mgdtspb6q82pr1jbmvw17"
  value_name = "client_secret"
}

# selfdns token
module "yav-selfdns-token" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01dsnmbfh6kbepcsyg6npmyyg5"
  value_name = "selfdns-oauth"
}

# redis password
module "yav-redis-password" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01f245bcbk42p56063tb24pph1"
  value_name = "password"
}

# AWS keys for the S3
module "yav-s3-access-key" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01fjemnbs04j33v6eq004m4ek5"
  value_name = "accessKey"
}
module "yav-s3-secret-key" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01fjemnbs04j33v6eq004m4ek5"
  value_name = "secretKey"
}

module "certificate-pem" {
  source     = "../../../modules/yav-secret"
  yt_oauth   = var.yandex_token
  id         = "sec-01fjez5j2c3bfz7pc0gdp0cbk1"
  value_name = "certificate_pem"
}

data "template_file" "docker_json" {
  template = file("${path.module}/../common/docker.tpl.json")

  vars = {
    docker_auth = module.lockbox-secret-sa-yc-crypto-e2e-monitoring-cr.secret
  }
}

data "template_file" "application_yaml" {
  template = file("${path.module}/files/application.tpl.yaml")
  count    = var.yc_instance_group_size

  vars = {
  }
}

data "template_file" "configs" {
  template = file("${path.module}/files/configs.tpl")
  count    = var.yc_instance_group_size

  vars = {
    application_yaml = element(data.template_file.application_yaml.*.rendered, count.index)
    e2e_monitoring_certificate_pem = module.certificate-pem.secret
  }
}

data "template_file" "infra_configs" {
  template = file("${path.module}/files/infra-configs.tpl")

  vars = {
    push_client_conf = file("${path.module}/files/push-client.yaml")
    solomon_agent_conf      = file("${path.module}/files/solomon-agent.conf")
  }
}

data "template_file" "podmanifest" {
  template = file("${path.module}/files/podmanifest.tpl.yaml")
  count    = var.yc_instance_group_size

  vars = {
    config_digest             = sha256(element(data.template_file.configs.*.rendered, count.index))
    infra_config_digest       = sha256(data.template_file.infra_configs.rendered)
    metadata_version          = var.metadata_image_version
    solomon_version           = var.solomon_agent_image_version
    push_client_version       = var.push-client_image_version
    push_client_tvm_secret    = module.yav-secret-yckms-tvm.secret
    application_version       = var.application_version
    redis_password            = module.yav-redis-password.secret
    logs_aws_access_key       = module.yav-s3-access-key.secret
    logs_aws_secret_key       = module.yav-s3-secret-key.secret
  }
}

module "e2e-monitoring-instance-group" {
  source          = "../../../modules/kubelet_instance_group_ycp_v2"
  name_prefix     = "crypto-e2e-monitoring"
  hostname_prefix = "crypto-e2e-monitoring"
  hostname_suffix = var.hostname_suffix
  role_name       = "crypto-e2e-monitoring"
  osquery_tag     = "ycloud-svc-crypto-e2e-monitoring"

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
    selfdns-token       = module.yav-selfdns-token.secret
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
  #security_group_ids = var.security_group_ids
}

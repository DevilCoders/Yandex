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

  // used in secrets.tf
  sdk_config {
    name = "preprod"
    prod = false
    ycp_profile = "preprod"
  }
  sdk_config {
    name = "prod"
    prod = false
    ycp_profile = "prod"
  }
}

data "template_file" "docker_json" {
  template = file("${path.module}/../common/files/docker.tpl.json")

  vars = {
    docker_auth = data.ycp_lockbox_secret_reference.lockbox-secret-sa-yc-schecker-cr.reference
  }
}

data "template_file" "swiss_knife_config_yaml" {
  template = file("${path.module}/files/schecker/swiss-knife-config.yaml")
  count    = var.yc_instance_group_size
}

data "template_file" "groups_swiss_knife_config_yaml" {
  template = file("${path.module}/files/schecker/groups-swiss-knife-config.yaml")
  count    = var.yc_instance_group_size
}

data "template_file" "syncer_config_yaml" {
  template = file("${path.module}/files/schecker/syncer-config.yaml")
  count    = var.yc_instance_group_size
}

data "template_file" "groups_syncer_config_yaml" {
  template = file("${path.module}/files/schecker/groups-syncer-config.yaml")
  count    = var.yc_instance_group_size
}

data "template_file" "parser_config_yaml" {
  template = file("${path.module}/files/schecker/parser-config.yaml")
  count    = var.yc_instance_group_size
}

data "template_file" "infra_configs" {
  template = file("${path.module}/files/infra-configs.tpl")

  vars = {
    push_client_conf    = file("${path.module}/files/push-client/push-client.yaml")
    push_client_iam_key = data.ycp_lockbox_secret_reference.lockbox-logbroker-iam-key.reference
  }
}

data "template_file" "configs" {
  template = file("${path.module}/files/schecker/configs.tpl")
  count    = var.yc_instance_group_size
  vars = {
    swiss_knife_config_yaml = element(data.template_file.swiss_knife_config_yaml.*.rendered, count.index)
    groups_swiss_knife_config_yaml = element(data.template_file.groups_swiss_knife_config_yaml.*.rendered, count.index)
    syncer_config_yaml      = element(data.template_file.syncer_config_yaml.*.rendered, count.index)
    groups_syncer_config_yaml      = element(data.template_file.groups_syncer_config_yaml.*.rendered, count.index)
    parser_config_yaml      = element(data.template_file.parser_config_yaml.*.rendered, count.index)
  }
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
    schecker_db_user       = data.ycp_lockbox_secret_reference.lockbox-secret-yc-schecker-db-user.reference
    schecker_splunk_user   = data.ycp_lockbox_secret_reference.lockbox-secret-yc-schecker-splunk-username.reference
    schecker_smtp_user     = data.ycp_lockbox_secret_reference.lockbox-secret-yc-schecker-smtp-user.reference
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
    schecker-db-password         = data.ycp_lockbox_secret_reference.lockbox-secret-yc-schecker-db-password.reference
    schecker-splunk-password     = data.ycp_lockbox_secret_reference.lockbox-secret-yc-schecker-splunk-password.reference
    schecker-api-token           = data.ycp_lockbox_secret_reference.lockbox-secret-yc-schecker-api-token.reference
    star-trek-oauth-token        = data.ycp_lockbox_secret_reference.lockbox-secret-yc-schecker-startrek-token.reference
    schecker-smtp-password       = data.ycp_lockbox_secret_reference.lockbox-secret-yc-schecker-smtp-password.reference
    schecker-clickhouse-password = data.ycp_lockbox_secret_reference.lockbox-secret-yc-schecker-clickhouse-password.reference
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

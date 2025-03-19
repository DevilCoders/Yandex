// It's important to upload access and secret key to YaV: 
// https://yav.yandex-team.ru/secret/sec-01ekye608rtvgts7nbazvg1fv9/
// (see israel_s3_[access|secret]_key secrets).
resource "yandex_iam_service_account_static_access_key" "mr_prober_key" {
  service_account_id = module.mr_prober.mr_prober_sa_id
  description = "Static Access Key for accessing object storage buckets"
}

// Bucket for storing agent configurations. 
// Agents make queries to this bucket to receive configurations
resource "yandex_storage_bucket" "agent_configurations" {
  bucket = "mr-prober-agent-configurations"
  acl = "private"
  access_key = yandex_iam_service_account_static_access_key.mr_prober_key.access_key
  secret_key = yandex_iam_service_account_static_access_key.mr_prober_key.secret_key
}

// Bucket for storing cluster states.
// Creator stores clusters' terraform states there.
resource "yandex_storage_bucket" "cluster_states" {
  bucket = "mr-prober-cluster-states"
  acl = "private"
  access_key = yandex_iam_service_account_static_access_key.mr_prober_key.access_key
  secret_key = yandex_iam_service_account_static_access_key.mr_prober_key.secret_key
}

// Bucket for storing probers' and Creator's logs.
resource "yandex_storage_bucket" "mr_prober_logs" {
  bucket = "mr-prober-logs"
  acl = "private"
  access_key = yandex_iam_service_account_static_access_key.mr_prober_key.access_key
  secret_key = yandex_iam_service_account_static_access_key.mr_prober_key.secret_key

  lifecycle_rule {
    id = "israel-logs"
    enabled = true
    prefix = "israel/"
    expiration {
      days = 90
    }
  }
}

locals {
  // https://st.yandex-team.ru/CLOUD-103283
  // https://racktables.yandex-team.ru/index.php?page=services&tab=projects&project_name=_CLOUD_IL_MR_PROBER_NETS_
  cloud_il_mr_prober_nets = 2415919163  // == 0x9000003b
}

module "mr_prober" {
  source = "../../modules/monitoring/mr_prober/"

  control_network_subnet_zones = var.availability_zones
  control_network_hbf_enabled = false

  // IL1-A - https://netbox.cloud.yandex.net/ipam/prefixes/3516/
  control_network_ipv6_cidrs = {
    il1-a = cidrsubnet("2a11:f740:1::/64", 32, local.cloud_il_mr_prober_nets)
  }
  control_network_ipv4_cidrs = {
    il1-a = "172.16.0.0/16"
  }

  run_creator = true
  creator_zone_id = "il1-a"

  api_zones = var.availability_zones
  api_domain = "api.prober.yandexcloud.co.il"
  api_alb_region = var.region_id
  dns_zone = "prober.yandexcloud.co.il"
  dns_zone_enable_yandex_dns_sync = false  // Israel's DNS zones should not be synced into Slayer DNS

  // Israel has only one AZ, but we need at least 3 hosts in PostgreSQL cluster,
  // so we force creating 3 hosts in one AZ.
  database_zones = concat(var.availability_zones, var.availability_zones, var.availability_zones)
  database_resource_preset = "s3-c2-m8"  // Israel has no s2.small or similar presets

  ycp_profile = var.ycp_profile
  yc_endpoint = var.yc_endpoint

  environment = var.environment
  mr_prober_environment = var.environment

  hc_network_ipv6 = var.hc_network_ipv6

  s3_endpoint = "https://s3.cloudil.com"  // Israel has (and uses) its own Object Storage
  s3_stand = "israel"

  use_conductor = false        // Migrate to EDS-only in Israel
  platform_id = "standard-v3"  // Israel has no platform "standard-v2"

  grpc_iam_api_endpoint = "ts.private-api.yandexcloud.co.il:14282"
  grpc_compute_api_endpoint = "compute.private-api.yandexcloud.co.il:19051"
  // Should be consistent with https://bb.yandex-team.ru/projects/CLOUD/repos/salt-formula/browse/pillar/common/israel/e2e-tests.sls#33
  meeseeks_compute_node_prefixes_cli_param = "--compute-node-prefix il"
}

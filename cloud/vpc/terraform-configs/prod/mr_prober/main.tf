// Container registry for all Mr. Prober's docker containers. 
// It's created only in PROD, and all stands use PROD's registry
// (because not every stand has container registry installed).
resource "yandex_container_registry" "mr_prober" {
   name = "mr-prober"
   folder_id = "yc.vpc.mr-prober"
}

// It's important to upload access and secret key to YaV: 
// https://yav.yandex-team.ru/secret/sec-01ekye608rtvgts7nbazvg1fv9/
// (see prod_s3_[access|secret]_key secrets).
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
    id = "prod-logs"
    enabled = true
    prefix = "prod/"
    expiration {
      days = 90
    }
  }

  lifecycle_rule {
    id = "preprod-logs"
    enabled = true
    prefix = "preprod/"
    expiration {
      days = 30
    }
  }

  lifecycle_rule {
    id = "development-logs"
    enabled = true
    prefix = "development/"
    expiration {
      days = 7
    }
  }
}

// Bucket for storing prober's test data. It's created only in PROD.
resource "yandex_storage_bucket" "probers_test_data" {
  bucket = "mr-prober-probes-test-data"
  acl = "public-read"
  access_key = yandex_iam_service_account_static_access_key.mr_prober_key.access_key
  secret_key = yandex_iam_service_account_static_access_key.mr_prober_key.secret_key
}

locals {
  // https://st.yandex-team.ru/CLOUD-51142
  // https://racktables.yandex-team.ru/index.php?page=services&tab=projects&project_name=_CLOUD_MR_PROBER_PROD_NETS_
  cloud_mr_prober_prod_nets = 63564  // == 0xf84c
}

module "mr_prober" {
  source = "../../modules/monitoring/mr_prober/"

  // VLA - http://netbox.cloud.yandex.net/ipam/prefixes/388/
  // SAS - http://netbox.cloud.yandex.net/ipam/prefixes/389/
  // MYT - http://netbox.cloud.yandex.net/ipam/prefixes/390/
  control_network_ipv6_cidrs = {
    ru-central1-a = cidrsubnet("2a02:6b8:c0e:500::/64", 32, local.cloud_mr_prober_prod_nets)
    ru-central1-b = cidrsubnet("2a02:6b8:c02:900::/64", 32, local.cloud_mr_prober_prod_nets)
    ru-central1-c = cidrsubnet("2a02:6b8:c03:500::/64", 32, local.cloud_mr_prober_prod_nets)
  }

  # Probably, we'll be able to decrease these values after https://st.yandex-team.ru/CLOUD-97970
  api_vm_memory = 8
  api_vm_cores = 4
  api_vm_fraction = 100

  # We need a lot of CPU cores here for "terraform plan" in meeseeks cluster
  # due to this terraform issue: https://github.com/hashicorp/terraform/issues/26355.
  # If (whenever) this issue is solved, see Creator CPU comsumption and review these numbers.
  run_creator = true
  creator_vm_memory = 8
  creator_vm_cores = 8
  creator_vm_fraction = 100
  creator_disk_size = 150

  database_resource_preset = "s2.large"

  api_domain = "api.prober.cloud.yandex.net"
  dns_zone = "prober.cloud.yandex.net"
  dns_zone_id = "dnscgvui2qjcsadq7lim"

  ycp_profile = var.ycp_profile
  yc_endpoint = var.yc_endpoint

  environment = var.environment
  mr_prober_environment = var.environment

  hc_network_ipv6 = var.hc_network_ipv6

  grpc_iam_api_endpoint = "ts.private-api.cloud.yandex.net:4282"
  grpc_compute_api_endpoint = "compute-api.cloud.yandex.net:9051"
  // Should be consistent with https://bb.yandex-team.ru/projects/CLOUD/repos/salt-formula/browse/pillar/common/prod/e2e-tests.sls#33
  meeseeks_compute_node_prefixes_cli_param = "--compute-node-prefix vla --compute-node-prefix sas --compute-node-prefix myt"
}

locals {
  // https://st.yandex-team.ru/CLOUD-88436
  // https://racktables.yandex-team.ru/index.php?page=services&tab=projects&project_name=_CLOUD_MR_PROBER_EXTERNAL_STANDS_PROD_NETS_
  cloud_mr_prober_external_stands_prod_nets = 63645  // == 0xf89d

  control_network_ipv6_cidrs_testing_suffix = 1
  control_network_ipv6_cidrs_hwlabs_suffix  = 2
}

module "mr_prober_testing" {
  source = "../../modules/monitoring/mr_prober/"

  folder_id = "yc.vpc.mr-prober.testing"

  // Prefixes are the same as for PROD.
  // First, we add Project ID to the prefix. Second, we add special TESTING suffix
  // to distinguish TESTING and other stands.
  control_network_ipv6_cidrs = {
    for zone, ipv6_prefix in {
      ru-central1-a = "2a02:6b8:c0e:500::/64",
      ru-central1-b = "2a02:6b8:c02:900::/64",
      ru-central1-c = "2a02:6b8:c03:500::/64"
    }: zone => cidrsubnet(
      cidrsubnet(
        ipv6_prefix, 32, local.cloud_mr_prober_external_stands_prod_nets
      ),
      16,
      local.control_network_ipv6_cidrs_testing_suffix
    )
  }

  run_creator = true

  api_domain = "api.testing.prober.cloud.yandex.net"
  dns_zone = "testing.prober.cloud.yandex.net"
  dns_zone_id = "dns3b13ap0kqhkroqp0t"

  ycp_profile = var.ycp_profile
  yc_endpoint = var.yc_endpoint

  environment = var.environment
  mr_prober_environment = "testing"

  mr_prober_sa_name = "mr-prober-testing-sa"

  hc_network_ipv6 = var.hc_network_ipv6

  grpc_iam_api_endpoint = "ts.private-api.cloud-testing.yandex.net:4282"
  grpc_compute_api_endpoint = "compute-api.cloud-testing.yandex.net:9051"
  // Should be consistent with https://bb.yandex-team.ru/projects/CLOUD/repos/salt-formula/browse/pillar/common/testing/e2e-tests.sls
  meeseeks_compute_node_prefixes_cli_param = "--compute-node-prefix vla --compute-node-prefix sas --compute-node-prefix myt"
}

module "mr_prober_hwlabs" {
  source = "../../modules/monitoring/mr_prober/"

  folder_id = "yc.vpc.mr-prober.hwlabs"

  // Prefixes are the same as for PROD.
  // First, we add Project ID to the prefix. Second, we add special HWLABS suffix
  // to distinguish HWLABS and other stands.
  control_network_ipv6_cidrs = {
    for zone, ipv6_prefix in {
      ru-central1-a = "2a02:6b8:c0e:500::/64",
      ru-central1-b = "2a02:6b8:c02:900::/64",
      ru-central1-c = "2a02:6b8:c03:500::/64"
    }: zone => cidrsubnet(
      cidrsubnet(
        ipv6_prefix, 32, local.cloud_mr_prober_external_stands_prod_nets
      ),
      16,
      local.control_network_ipv6_cidrs_hwlabs_suffix
    )
  }

  run_creator = true

  api_domain = "api.hwlabs.prober.cloud.yandex.net"
  dns_zone = "hwlabs.prober.cloud.yandex.net"
  dns_zone_id = "dnsd35pobn3r07mrbpsi"

  ycp_profile = var.ycp_profile
  yc_endpoint = var.yc_endpoint

  environment = var.environment
  mr_prober_environment = "hwlabs"

  mr_prober_sa_name = "mr-prober-hwlabs-sa"

  hc_network_ipv6 = var.hc_network_ipv6

  # Following parameters are needed only for dynamic update of Meeseeks cluster (see meeseeks-updater).
  # We don't use meeseeks-updater on HW-LABs, so left these parameters empty.
  grpc_iam_api_endpoint = ""
  grpc_compute_api_endpoint = ""
  meeseeks_compute_node_prefixes_cli_param = ""
}

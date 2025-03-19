locals {
  // https://st.yandex-team.ru/CLOUD-51142
  // https://racktables.yandex-team.ru/index.php?page=services&tab=projects&project_name=_CLOUD_MR_PROBER_PREPROD_NETS_
  cloud_mr_prober_preprod_nets = 64556  // == 0xfc2c
}

module "mr_prober" {
  source = "../../modules/monitoring/mr_prober/"

  // VLA - http://netbox.cloud.yandex.net/ipam/prefixes/391/
  // SAS - http://netbox.cloud.yandex.net/ipam/prefixes/392/
  // MYT - http://netbox.cloud.yandex.net/ipam/prefixes/393/
  control_network_ipv6_cidrs = {
    ru-central1-a = cidrsubnet("2a02:6b8:c0e:501::/64", 32, local.cloud_mr_prober_preprod_nets)
    ru-central1-b = cidrsubnet("2a02:6b8:c02:901::/64", 32, local.cloud_mr_prober_preprod_nets)
    ru-central1-c = cidrsubnet("2a02:6b8:c03:501::/64", 32, local.cloud_mr_prober_preprod_nets)
  }

  run_creator = true

  api_domain = "api.prober.cloud-preprod.yandex.net"
  dns_zone = "prober.cloud-preprod.yandex.net"
  dns_zone_id = "aet1g3f9elp1dgustn3p"

  ycp_profile = var.ycp_profile
  yc_endpoint = var.yc_endpoint

  environment = var.environment
  mr_prober_environment = var.environment

  hc_network_ipv6 = var.hc_network_ipv6

  grpc_iam_api_endpoint = "ts.private-api.cloud-preprod.yandex.net:4282"
  grpc_compute_api_endpoint = "compute-api.cloud-preprod.yandex.net:9051"
  // Should be consistent with https://bb.yandex-team.ru/projects/CLOUD/repos/salt-formula/browse/pillar/common/pre-prod/e2e-tests.sls#35
  meeseeks_compute_node_prefixes_cli_param = "--compute-node-prefix vla --compute-node-prefix sas --compute-node-prefix myt"
}

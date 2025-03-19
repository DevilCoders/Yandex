module "vpc_load_test" {
  source = "../../modules/vpc_load_test/"

  // https://st.yandex-team.ru/CLOUD-94464
  // https://racktables.yandex-team.ru/index.php?page=services&tab=projects&project_name=_CLOUD_VPC_OVERLAY_PROD_NETS_
  vpc_overlay_net = 63651  // == 0xf8a3

  // VLA - http://netbox.cloud.yandex.net/ipam/prefixes/388/
  // SAS - http://netbox.cloud.yandex.net/ipam/prefixes/389/
  // MYT - http://netbox.cloud.yandex.net/ipam/prefixes/390/
  vpc_load_test_network_ipv6_cidrs = {
    ru-central1-a = "2a02:6b8:c0e:500::/64"
    ru-central1-b = "2a02:6b8:c02:900::/64"
    ru-central1-c = "2a02:6b8:c03:500::/64"
  }

  ycp_profile = var.ycp_profile
  yc_endpoint = var.yc_endpoint
  environment = var.environment
  jump_image  = var.image_id

  // https://st.yandex-team.ru/CLOUD-105606
  dns_zone = "vpc-load-test.cloud.yandex.net"
  dns_zone_id = "dns7qkk5k6mckpucvfqu"
}

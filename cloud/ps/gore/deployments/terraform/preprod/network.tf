module "network" {
  source = "../modules/network/"

  // https://st.yandex-team.ru/CLOUD-54442
  // https://racktables.yandex-team.ru/index.php?page=services&tab=projects&project_name=_CLOUD_GORE_PREPROD_NETS_
  project_id = 64563  // == 0xfc33

  // VLA - http://netbox.cloud.yandex.net/ipam/prefixes/391/
  // SAS - http://netbox.cloud.yandex.net/ipam/prefixes/392/
  // MYT - http://netbox.cloud.yandex.net/ipam/prefixes/393/
  ipv6_cidrs = {
    ru-central1-a = "2a02:6b8:c0e:501::/64"
    ru-central1-b = "2a02:6b8:c02:901::/64"
    ru-central1-c = "2a02:6b8:c03:501::/64"
  }
}

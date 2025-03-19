module "network" {
  source = "../modules/network/"

  // https://st.yandex-team.ru/CLOUD-54442
  // https://racktables.yandex-team.ru/index.php?page=services&tab=projects&project_name=_CLOUD_GORE_PROD_NETS_
  project_id = 63568 // == 0xf850

  // VLA - http://netbox.cloud.yandex.net/ipam/prefixes/388/
  // SAS - http://netbox.cloud.yandex.net/ipam/prefixes/389/
  // MYT - http://netbox.cloud.yandex.net/ipam/prefixes/390/
  ipv6_cidrs = {
    ru-central1-a = "2a02:6b8:c0e:500::/64"
    ru-central1-b = "2a02:6b8:c02:900::/64"
    ru-central1-c = "2a02:6b8:c03:500::/64"
  }
}


data "yandex_dns_zone" "oncall-dns-zone" {
  dns_zone_id = "dns2t4fobecr3kg2evoq"
}


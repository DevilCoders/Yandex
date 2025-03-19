locals {
  installation = {
    instance_env_suffix = ""
    dns_zone            = "gpn.yandexcloud.net"
    region_id           = var.region_id
    known_zones = {
      "zone_spb99" = {
        "shortname" = "gpn-spb99"
        "letter"    = "k"
        "subnet_id" = data.ycp_vpc_subnet.mdb-control-plane-nets-ru-gpn-spb99.subnet_id
        "id"        = "ru-gpn-spb99"
      }
    }
    security_groups = []
  }
}

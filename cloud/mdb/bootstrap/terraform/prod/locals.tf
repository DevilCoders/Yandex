locals {
  zones = {
    "zone_a" = {
      "shortname" = "rc1a"
      "letter"    = "k"
      "subnet_id" = ycp_vpc_subnet.dbaasexternalnets-ru-central1-a.id
      "id"        = "ru-central1-a"
    }
    "zone_b" = {
      "shortname" = "rc1b"
      "letter"    = "h"
      "subnet_id" = ycp_vpc_subnet.dbaasexternalnets-ru-central1-b.id
      "id"        = "ru-central1-b"
    }
    "zone_c" = {
      "shortname" = "rc1c"
      "letter"    = "f"
      "subnet_id" = ycp_vpc_subnet.dbaasexternalnets-ru-central1-c.id
      "id"        = "ru-central1-c"
    }
  }

  installation = {
    instance_env_suffix = ""
    dns_zone            = "yandexcloud.net"
    region_id           = var.region_id
    known_zones         = local.zones
    security_groups     = [yandex_vpc_security_group.control-plane-sg.id]
  }

  deploy_userdata = "deploy_version: '2'\nmdb_deploy_api_host: mdb-deploy-api.private-api.yandexcloud.net"
}

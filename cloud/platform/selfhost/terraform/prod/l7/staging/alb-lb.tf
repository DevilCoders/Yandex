# ycp_platform_alb_load_balancer.this:
resource "ycp_platform_alb_load_balancer" "this" {
  # id = "ds71ud0i3bnte12upp74"
  folder_id = "b1gvgqhc57450av0d77p"

  name        = "staging"
  description = "LB for tests"

  internal       = true
  traffic_scheme = "TRAFFIC_SCHEME_UNSPECIFIED"

  deletion_protection = false

  labels = {
    "internal-conductor-group" = "l7-staging"
    "internal-instance-name"   = "staging-ig"

    "internal-enable-remote-als" = "yyy"
    "internal-disable-local-als" = "yyy"
  }

  security_group_ids = []

  juggler_host         = "cloud_prod_l7-staging"
  solomon_cluster_name = "cloud_prod_staging"

  listener {
    name = "tls"

    endpoint {
      ports = [443]

      address {
        external_ipv6_address {}
      }
      address {
        external_ipv4_address {}
      }
    }

    tls {
      default_handler {
        certificate_ids = ["fpqvgu8adrqgmvv2qo00"]

        http_handler {
          allow_http10   = false
          http_router_id = "ds7a2pu8erpjvdnb6kvs"
          is_edge        = true
        }
      }
    }
  }

  allocation_policy {
    location {
      disable_traffic = false
      subnet_id       = "e9bi9drqb7rddf4od5jk"
      zone_id         = "ru-central1-a"
    }
    location {
      disable_traffic = false
      subnet_id       = "e2lo6auqjjfru2tfnorm"
      zone_id         = "ru-central1-b"
    }
    location {
      disable_traffic = false
      subnet_id       = "b0c3id3uje06qohkgmlg"
      zone_id         = "ru-central1-c"
    }
  }
}

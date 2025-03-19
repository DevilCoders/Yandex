resource "ycp_vpc_address" "auth_l7_external_ipv6" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id = local.openid_folder.id
  name      = "auth-l7-ipv6"

  ipv6_address_spec {
    requirements {
      hints = [] # или ["yandex-only"] если надо адрес, доступный только из сетей яндекса.
    }
  }
  reserved = true # убирает лишний diff в terraform plan
}

resource "ycp_vpc_address" "auth_l7_external_ipv4" {
  lifecycle {
    prevent_destroy = true
  }

  folder_id = local.openid_folder.id
  name        = "auth-l7-ipv4"

  external_ipv4_address_spec {
    region_id = local.region_id

    requirements {
      hints = []
    }
  }

  reserved = true # убирает лишний diff в terraform plan
}

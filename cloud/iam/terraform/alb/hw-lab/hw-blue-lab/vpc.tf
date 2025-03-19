resource "ycp_vpc_address" "auth-l7" {
  lifecycle {
    prevent_destroy = true
  }

  folder_id = local.openid_folder.id
  name      = "auth-l7-ipv6"

  ipv6_address_spec {
    requirements {
      hints = ["yandex-only"] # или ["yandex-only"] если надо адрес, доступный только из сетей яндекса.
    }
  }
  reserved = true
}

locals {
  fqdn = "${var.domain}.${data.yandex_dns_zone.this.zone}"
}

data "yandex_dns_zone" "this" {
  dns_zone_id = var.dns_zone_id
}

resource "yandex_dns_recordset" "a" {
  count = length(var.ingress_ips_v4) > 0 ? 1 : 0

  zone_id = var.dns_zone_id
  name    = local.fqdn
  type    = "A"
  ttl     = 600

  data = var.ingress_ips_v4
}

resource "yandex_dns_recordset" "aaaa" {
  count = length(var.ingress_ips_v6) > 0 ? 1 : 0

  zone_id = var.dns_zone_id
  name    = local.fqdn
  type    = "AAAA"
  ttl     = 600

  data = var.ingress_ips_v6
}

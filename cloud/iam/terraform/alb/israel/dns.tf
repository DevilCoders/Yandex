resource "ycp_dns_dns_record_set" "auth_AAAA" {
  lifecycle {
    prevent_destroy = true
  }

  zone_id = local.dns.zone_id
  name    = local.auth_dns_record_name
  type    = "AAAA"
  ttl     = "3600"
  data    = flatten([for a in ycp_vpc_address.auth_l7_external_ipv6.ipv6_address : a.address])
}

resource "ycp_dns_dns_record_set" "auth_A" {
  lifecycle {
    prevent_destroy = true
  }

  zone_id = local.dns.zone_id
  name    = local.auth_dns_record_name
  type    = "A"
  ttl     = "3600"
  data    = flatten([for a in ycp_vpc_address.auth_l7_external_ipv4.external_ipv4_address : a.address])
}

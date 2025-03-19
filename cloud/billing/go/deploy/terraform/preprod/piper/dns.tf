resource "yandex_dns_recordset" "all_pipers" {
  zone_id = local.dns_zone_id
  name    = "piper-all"
  ttl     = 200
  type    = "AAAA"
  data    = [for k, v in module.piper : v.overlay_ipv6]
}

output "dns_record" {
  value = {
    id   = yandex_dns_recordset.all_pipers.id
    type = yandex_dns_recordset.all_pipers.type
    name = yandex_dns_recordset.all_pipers.name
    data = yandex_dns_recordset.all_pipers.data
  }
}

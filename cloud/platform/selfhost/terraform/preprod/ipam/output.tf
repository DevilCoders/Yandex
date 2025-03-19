output "service_list" {
  value = local.service_map
}

output "subnets_map" {
  value = {
    "ru-central1-a" = data.yandex_vpc_subnet.a.id
    "ru-central1-b" = data.yandex_vpc_subnet.b.id
    "ru-central1-c" = data.yandex_vpc_subnet.c.id
  }
}

output "service-name-a_v4_address" {
  value = data.template_file.service-api-router-v4.*.rendered
}

output "service-name-a_v6_address" {
  value = data.template_file.service-api-router-v6.*.rendered
}

output "service-name-atlantis_v4_address" {
  value = data.template_file.service-atlantis-v4.*.rendered
}

output "service-name-atlantis_v6_address" {
  value = data.template_file.service-atlantis-v6.*.rendered
}


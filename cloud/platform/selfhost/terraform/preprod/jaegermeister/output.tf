output "jaegermeister_instances_addresses" {
  value = yandex_compute_instance.jaegermeister.*.network_interface.0.ipv6_address
}

output "jaegermeister_instances" {
  value = formatlist(
    "instance [%v] has ipv6 %v and name %v",
    yandex_compute_instance.jaegermeister.*.id,
    yandex_compute_instance.jaegermeister.*.network_interface.0.ipv6_address,
    yandex_compute_instance.jaegermeister.*.name,
  )
}


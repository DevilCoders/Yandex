output "all_instance_ipv6_addresses" {
  //noinspection HILUnresolvedReference
  value = [
    "${yandex_compute_instance.backend.*.network_interface.0.ipv6_address}",
  ]
}

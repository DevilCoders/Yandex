output "external_ip" {
  value = "${yandex_compute_instance.foobar.network_interface.0.nat_ip_address}"
}

output "address" {
  value = yandex_compute_instance.default.network_interface.0.nat_ip_address
}

output "instance_id" {
  value = yandex_compute_instance.default.id
}

output "admin_pass" {
  value = var.admin_pass
  sensitive = true
}

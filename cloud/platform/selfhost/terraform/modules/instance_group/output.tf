output "all_instance_group_instance_ids" {
  value = "${yandex_compute_instance.node.*.id}"
}

output "all_instance_group_fqdns" {
  value = [
    "${yandex_compute_instance.node.*.fqdn}"]
}

output "all_instance_ipv6_addresses" {
  value = [
    "${yandex_compute_instance.node.*.network_interface.0.ipv6_address}"]
}

output "all_instance_ipv4_addresses" {
  value = [
    "${yandex_compute_instance.node.*.network_interface.0.ip_address}"]
}

output "full_info" {
  value = [
    "${formatlist("id:%s fqdn:%s", yandex_compute_instance.node.*.id, yandex_compute_instance.node.*.fqdn)}"]
}

output "instances_fqdn_ipv6" {
  value = [
    "${formatlist("%s -- %s", yandex_compute_instance.node.*.fqdn, yandex_compute_instance.node.*.network_interface.0.ipv6_address)}"
  ]
}
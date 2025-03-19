output "all_instance_group_instance_ids" {
  value = "${ycp_compute_instance.node.*.id}"
}

output "all_instance_group_fqdns" {
  value = [
    "${ycp_compute_instance.node.*.fqdn}",
  ]
}

output "all_instance_ipv6_addresses" {
  value = [
    for node in ycp_compute_instance.node.*:
      length(coalesce(node.network_interface, [])) > 0 ? node.network_interface.0.primary_v6_address.0.address : "underlay"
  ]
}

output "full_info" {
  value = [
    "${formatlist("id:%s fqdn:%s", ycp_compute_instance.node.*.id, ycp_compute_instance.node.*.fqdn)}",
  ]
}

output "instances_fqdn_ipv6" {
  value = [
    for node in ycp_compute_instance.node.*:
      formatlist("%s -- %s", node.fqdn,
      length(coalesce(node.network_interface, [])) > 0 ? node.network_interface.0.primary_v6_address.0.address : "underlay")
  ]
}

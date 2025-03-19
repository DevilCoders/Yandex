output "all_instance_group_instance_ids" {
  value = "${ycunderlay_compute_instance.node.*.id}"
}

output "all_instance_group_fqdns" {
  value = [
    "${ycunderlay_compute_instance.node.*.fqdn}",
  ]
}

output "all_instance_ipv6_addresses" {
  value = [
    for node in ycunderlay_compute_instance.node.*:
      length(node.network_interface) > 0 ? node.network_interface.0.ipv6_address : "underlay"
  ]
}

output "full_info" {
  value = [
    "${formatlist("id:%s fqdn:%s", ycunderlay_compute_instance.node.*.id, ycunderlay_compute_instance.node.*.fqdn)}",
  ]
}

output "instances_fqdn_ipv6" {
  value = [
    for node in ycunderlay_compute_instance.node.*:
      formatlist("%s -- %s", node.fqdn,
      length(node.network_interface) > 0 ? node.network_interface.0.ipv6_address : "underlay")
  ]
}

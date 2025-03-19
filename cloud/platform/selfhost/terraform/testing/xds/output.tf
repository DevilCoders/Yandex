output "xds_instances_addresses" {
  value = ycp_compute_instance.xds.*.network_interface.0.primary_v6_address
}

output "xds_instances" {
  value = formatlist(
    "instance [%v] has ipv6 %v and name %v",
    ycp_compute_instance.xds.*.id,
    ycp_compute_instance.xds.*.network_interface.0.primary_v6_address,
    ycp_compute_instance.xds.*.name,
  )
}
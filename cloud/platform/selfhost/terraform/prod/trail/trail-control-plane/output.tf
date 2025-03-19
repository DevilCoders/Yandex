output "trail_control_plane_instances" {
  value = "${module.control-plane-instance-group.instances_fqdn_ipv6}"
}

output "trail_control_plane_instance_addresses" {
  value = "${module.control-plane-instance-group.all_instance_ipv6_addresses}"
}

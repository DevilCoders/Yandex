output "lockbox_data_plane_instances" {
  value = "${module.lockbox-data-plane-instance-group.instances_fqdn_ipv6}"
}

output "lockbox_data_plane_instance_addresses" {
  value = "${module.lockbox-data-plane-instance-group.all_instance_ipv6_addresses}"
}

output "trail_preparer_instances" {
  value = "${module.preparer-instance-group.instances_fqdn_ipv6}"
}

output "trail_preparer_instance_addresses" {
  value = "${module.preparer-instance-group.all_instance_ipv6_addresses}"
}

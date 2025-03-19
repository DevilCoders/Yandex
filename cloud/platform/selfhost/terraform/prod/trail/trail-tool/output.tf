output "trail_tool_instances" {
  value = "${module.tool-instance-group.instances_fqdn_ipv6}"
}

output "trail_tool_instance_addresses" {
  value = "${module.tool-instance-group.all_instance_ipv6_addresses}"
}

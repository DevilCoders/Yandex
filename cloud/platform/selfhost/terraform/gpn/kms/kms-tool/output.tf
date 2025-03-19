output "kms_tool_instances" {
  value = "${module.kms-tool-instance-group.instances_fqdn_ipv6}"
}

output "kms_tool_instance_addresses" {
  value = "${module.kms-tool-instance-group.all_instance_ipv6_addresses}"
}

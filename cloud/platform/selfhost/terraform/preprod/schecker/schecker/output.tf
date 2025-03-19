output "schecker_instances" {
  value = module.schecker-instance-group.instances_fqdn_ipv6
}

output "schecker_instance_addresses" {
  value = module.schecker-instance-group.all_instance_ipv6_addresses
}

output "schecker_l7_address" {
  value = ycp_vpc_address.schecker-l7-ipv6.ipv6_address[0].address
}

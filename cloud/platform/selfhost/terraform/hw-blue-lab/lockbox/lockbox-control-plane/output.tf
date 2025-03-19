output "lockbox_cpl_instances" {
  value = module.lockbox-cpl-instance-group.instances_fqdn_ipv6
}

output "lockbox_cpl_instance_addresses" {
  value = module.lockbox-cpl-instance-group.all_instance_ipv6_addresses
}

output "lockbox_cpl_balancer_address" {
  value = ycp_vpc_inner_address.cpl-load-balancer-address.address
}

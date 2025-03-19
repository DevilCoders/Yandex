output "lockbox_dpl_instances" {
  value = module.lockbox-dpl-instance-group.instances_fqdn_ipv6
}

output "lockbox_dpl_instance_addresses" {
  value = module.lockbox-dpl-instance-group.all_instance_ipv6_addresses
}

output "lockbox_dpl_balancer_address" {
  value = ycp_vpc_inner_address.dpl-load-balancer-address.address
}

output "kms_cpl_instances" {
  value = module.kms-cpl-instance-group.instances_fqdn_ipv6
}

output "kms_cpl_instance_addresses" {
  value = module.kms-cpl-instance-group.all_instance_ipv6_addresses
}

output "kms_cpl_balancer_address" {
  value = ycp_vpc_inner_address.cpl-load-balancer-address.address
}

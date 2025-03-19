output "kms_dpl_instances" {
  value = module.kms-dpl-instance-group.instances_fqdn_ipv6
}

output "kms_dpl_instance_addresses" {
  value = module.kms-dpl-instance-group.all_instance_ipv6_addresses
}

output "kms_dpl_balancer_address" {
  value = ycp_vpc_inner_address.dpl-load-balancer-address.address
}

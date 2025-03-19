output "certificate_manager_cpl_instances" {
  value = module.certificate-manager-cpl-instance-group.instances_fqdn_ipv6
}

output "certificate_manager_cpl_instance_addresses" {
  value = module.certificate-manager-cpl-instance-group.all_instance_ipv6_addresses
}

output "certificate_manager_cpl_balancer_address" {
  value = ycp_vpc_inner_address.cpl-load-balancer-address.address
}

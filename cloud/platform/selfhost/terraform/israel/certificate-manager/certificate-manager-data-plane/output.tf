output "certificate_manager_dpl_instances" {
  value = module.certificate-manager-dpl-instance-group.instances_fqdn_ipv6
}

output "certificate_manager_dpl_instance_addresses" {
  value = module.certificate-manager-dpl-instance-group.all_instance_ipv6_addresses
}

output "certificate_manager_dpl_balancer_address" {
  value = ycp_vpc_inner_address.dpl-load-balancer-address.address
}

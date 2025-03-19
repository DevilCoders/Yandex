output "certificate_manager_validation_server_instances" {
  value = module.validation-server-instance-group.instances_fqdn_ipv6
}

output "certificate_manager_validation_server_instance_addresses" {
  value = module.validation-server-instance-group.all_instance_ipv6_addresses
}

output "certificate_manager_validation_server_ipv6_load_balancer_id" {
  value = module.ipv6-load-balancer.id
}

output "certificate_manager_validation_server_ipv6_load_balancer_spec" {
  value = module.ipv6-load-balancer.spec
}

output "certificate_manager_validation_server_ipv6_load_balancer_tg_id" {
  value = module.ipv6-load-balancer.target_group_id
}

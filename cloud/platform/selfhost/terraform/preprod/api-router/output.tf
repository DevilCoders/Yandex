output "api_router_instances" {
  value = module.api-router-instance-group.instances_fqdn_ipv6
}

output "api_router_instance_addresses" {
  value = module.api-router-instance-group.all_instance_ipv6_addresses
}

output "api_router_instance_addresses_ipv4" {
  value = module.api-router-instance-group.all_instance_ipv4_addresses
}

output "api_router_instances_full_info" {
  value = module.api-router-instance-group.full_info
}


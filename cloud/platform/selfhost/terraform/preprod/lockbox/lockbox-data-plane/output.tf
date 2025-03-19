output "lockbox_data_plane_instances" {
  value = module.lockbox-data-plane-instance-group.instances_fqdn_ipv6
}

output "lockbox_data_plane_instance_addresses" {
  value = module.lockbox-data-plane-instance-group.all_instance_ipv6_addresses
}

output "lockbox_data_plane_ipv6_load_balancer_id" {
  value = module.private-ipv6-load-balancer.id
}

output "lockbox_data_plane_ipv6_load_balancer_spec" {
  value = module.private-ipv6-load-balancer.spec
}

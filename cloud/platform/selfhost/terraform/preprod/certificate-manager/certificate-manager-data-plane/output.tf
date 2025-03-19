output "certificate_manager_data_plane_instances" {
  value = module.data-plane-instance-group.instances_fqdn_ipv6
}

output "certificate_manager_data_plane_instance_addresses" {
  value = module.data-plane-instance-group.all_instance_ipv6_addresses
}

/*output "certificate_manager_data_plane_ipv4_load_balancer_id" {
  value = "${module.ipv4-load-balancer.id}"
}

output "certificate_manager_data_plane_ipv4_load_balancer_spec" {
  value = "${module.ipv4-load-balancer.spec}"
}

output "certificate_manager_data_plane_ipv4_load_balancer_tg_id" {
  value = "${module.ipv4-load-balancer.target_group_id}"
}*/

output "certificate_manager_data_plane_ipv6_load_balancer_id" {
  value = module.ipv6-load-balancer.id
}

output "certificate_manager_data_plane_ipv6_load_balancer_spec" {
  value = module.ipv6-load-balancer.spec
}

output "certificate_manager_data_plane_ipv6_load_balancer_tg_id" {
  value = module.ipv6-load-balancer.target_group_id
}
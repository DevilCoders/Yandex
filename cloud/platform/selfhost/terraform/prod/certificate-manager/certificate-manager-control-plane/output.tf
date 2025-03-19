output "certificate_manager_control_plane_instances" {
  value = module.control-plane-instance-group.instances_fqdn_ipv6
}

output "certificate_manager_control_plane_instance_addresses" {
  value = module.control-plane-instance-group.all_instance_ipv6_addresses
}

output "certificate_manager_control_plane_ipv4_target_group_id" {
  value = module.ipv4-target-group.id
}

output "certificate_manager_control_plane_ipv6_target_group_id" {
  value = module.ipv6-target-group.id
}

output "certificate_manager_control_plane_ipv4_load_balancer_id" {
  value = module.public-ipv4-load-balancer.id
}

output "certificate_manager_control_plane_public_ipv4_load_balancer_spec" {
  value = module.public-ipv4-load-balancer.spec
}

output "certificate_manager_control_plane_public_ipv6_load_balancer_id" {
  value = module.public-ipv6-load-balancer.id
}

output "certificate_manager_control_plane_public_ipv6_load_balancer_spec" {
  value = module.public-ipv6-load-balancer.spec
}

output "certificate_manager_control_plane_yandex_only_ipv6_load_balancer_id" {
  value = module.ipv6-load-balancer.id
}

output "certificate_manager_control_plane_yandex_only_ipv6_load_balancer_spec" {
  value = module.ipv6-load-balancer.spec
}

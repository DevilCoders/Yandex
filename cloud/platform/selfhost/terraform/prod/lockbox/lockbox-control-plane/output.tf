output "lockbox_control_plane_instances" {
  value = "${module.lockbox-control-plane-instance-group.instances_fqdn_ipv6}"
}

output "lockbox_control_plane_instance_addresses" {
  value = "${module.lockbox-control-plane-instance-group.all_instance_ipv6_addresses}"
}

output "lockbox_control_plane_ipv6_load_balancer_id" {
  value = "${module.private-ipv6-load-balancer.id}"
}

output "lockbox_control_plane_ipv6_load_balancer_spec" {
  value = "${module.private-ipv6-load-balancer.spec}"
}

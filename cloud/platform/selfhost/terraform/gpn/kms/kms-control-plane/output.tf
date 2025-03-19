output "kms_control_plane_instances" {
  value = "${module.kms-control-plane-instance-group.instances_fqdn_ipv6}"
}

output "kms_control_plane_instance_addresses" {
  value = "${module.kms-control-plane-instance-group.all_instance_ipv6_addresses}"
}

output "kms_load_balancer_address" {
  value = "${ycp_load_balancer_network_load_balancer.load-balancer.listener_spec.*.external_address_spec}"
}

output "kms_load_balancer_ports" {
  value = "${ycp_load_balancer_network_load_balancer.load-balancer.listener_spec.*.port}"
}

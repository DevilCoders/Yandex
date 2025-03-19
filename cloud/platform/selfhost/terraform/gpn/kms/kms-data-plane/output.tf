output "kms_data_plane_instances" {
  value = "${module.kms-data-plane-instance-group.instances_fqdn_ipv6}"
}

output "kms_data_plane_instance_addresses" {
  value = "${module.kms-data-plane-instance-group.all_instance_ipv6_addresses}"
}

output "kms_load_balancer_address" {
  value = "${ycp_load_balancer_network_load_balancer.load-balancer.listener_spec.*.external_address_spec}"
}

output "kms_load_balancer_address_v4" {
  value = "${ycp_load_balancer_network_load_balancer.load-balancer-v4.listener_spec.*.external_address_spec}"
}

output "kms_load_balancer_ports" {
  value = "${ycp_load_balancer_network_load_balancer.load-balancer.listener_spec.*.port}"
}

output "kms_load_balancer_ports_v4" {
  value = "${ycp_load_balancer_network_load_balancer.load-balancer-v4.listener_spec.*.port}"
}

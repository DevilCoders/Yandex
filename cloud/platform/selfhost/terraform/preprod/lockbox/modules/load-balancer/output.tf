output "id" {
  value = ycp_load_balancer_network_load_balancer.load-balancer.id
}

output "spec" {
  value = ycp_load_balancer_network_load_balancer.load-balancer.listener_spec.*.external_address_spec
}


locals {
  all_instance_group_fqdns = [
  for count in range(var.instance_group_size) :
  "${var.hostname_prefix}-${lookup(var.host_suffix_for_zone, element(var.zones, count))}-${floor(count / length(var.zones)) + count % length(var.zones) - index(var.zones, element(var.zones, count)) + 1}.${var.hostname_suffix}"
  ]

  all_envoy_hostnames = concat(["localhost"], local.all_instance_group_fqdns)

  envoy_endpoints = [
  for i, hostname in local.all_envoy_hostnames :
  "{ priority: ${i}, lb_endpoints: { endpoint: { address: { socket_address: { address: ${hostname}, port_value: ${var.api_port} } } } } }"
  ]

  envoy_endpoints_string = "[${join(", ", local.envoy_endpoints)}]"
}
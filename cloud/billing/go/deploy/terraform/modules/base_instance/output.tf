output "instance_id" {
  value = ycp_compute_instance.inst.id
}

output "fqdn" {
  value = ycp_compute_instance.inst.fqdn
}

output "underlay_ipv6" {
  value = var.skip_underlay ? "" : ycp_compute_instance.inst.underlay_network.0.ipv6_address
}

output "overlay_ipv6" {
  value = ycp_compute_instance.inst.network_interface[0].primary_v6_address[0].address
}

output "mk8s_billcollector" {
  value = formatlist(
    "[%v] %v - %v",
    ycp_compute_instance.mk8s-billcollector.*.id,
    data.template_file.mk8s-billcollector-fqdn.*.rendered,
    ycp_compute_instance.mk8s-billcollector.*.network_interface.0.primary_v6_address.0.address,
  )
}

output "mk8s_contoller_blue" {
  value = formatlist(
    "[%v] %v - %v",
    ycp_compute_instance.mk8s-controller-blue.*.id,
    data.template_file.mk8s-controller-blue-fqdn.*.rendered,
    ycp_compute_instance.mk8s-controller-blue.*.network_interface.0.primary_v6_address.0.address,
  )
}

output "mk8s_contoller_green" {
  value = formatlist(
    "[%v] %v - %v",
    ycp_compute_instance.mk8s-controller-green.*.id,
    data.template_file.mk8s-controller-green-fqdn.*.rendered,
    ycp_compute_instance.mk8s-controller-green.*.network_interface.0.primary_v6_address.0.address,
  )
}

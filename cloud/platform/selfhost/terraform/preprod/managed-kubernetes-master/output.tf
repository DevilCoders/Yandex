output "mk8s_master_instances" {
  value = formatlist(
    "[%v] %v - %v",
    ycp_compute_instance.mk8s-master.*.id,
    data.template_file.mk8s-master-name.*.rendered,
    ycp_compute_instance.mk8s-master.*.network_interface.0.primary_v6_address.0.address,
  )
}

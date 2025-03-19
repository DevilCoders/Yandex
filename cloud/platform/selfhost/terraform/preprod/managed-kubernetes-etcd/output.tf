output "mk8s_etcd_instances" {
  value = formatlist(
    "[%v] %v - %v",
    ycp_compute_instance.mk8s-etcd.*.id,
    data.template_file.mk8s-etcd-name.*.rendered,
    ycp_compute_instance.mk8s-etcd.*.network_interface.0.primary_v6_address.0.address,
  )
}
